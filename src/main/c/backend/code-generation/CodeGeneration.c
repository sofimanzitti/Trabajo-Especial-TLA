#include "CodeGeneration.h"
#include "../semantic-analysis/StepGraph.h"
#include "../semantic-analysis/Units.h"
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownCodeGenerationModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: CodeGeneration...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeCodeGenerationModule() {
	_logger = createLogger("CodeGeneration");
	return _shutdownCodeGenerationModule;
}

/* PRIVATE FUNCTIONS */

#define EPSILON 1e-9

/** The total of an ingredient needed by a set of recipes, in the unit of the ingredient. */
typedef struct {
	const IngredientDeclaration * ingredient;
	double amount;
} Need;

typedef struct {
	Need * items;
	size_t count;
	size_t capacity;
} Needs;

static void _addNeed(Needs * needs, const IngredientDeclaration * ingredient, const double amount) {
	for (size_t k = 0; k < needs->count; ++k) {
		if (needs->items[k].ingredient == ingredient) {
			needs->items[k].amount += amount;
			return;
		}
	}
	if (needs->count == needs->capacity) {
		needs->capacity = (needs->capacity == 0) ? 16 : needs->capacity * 2;
		needs->items = (Need *) realloc(needs->items, needs->capacity * sizeof(Need));
	}
	needs->items[needs->count].ingredient = ingredient;
	needs->items[needs->count].amount = amount;
	++needs->count;
}

/**
 * Consolidation: expands a recipe (multiplied by a factor) into raw ingredients.
 * A recipe used as an ingredient is expanded recursively, in proportion to the
 * amount of it that is required with respect to what it yields.
 */
static void _expandRecipe(const SymbolTable * table, const RecipeDeclaration * recipe, const double factor, Needs * needs) {
	for (const RequiresItem * item = recipe->requires; item != NULL; item = item->next) {
		double amount = 0;
		resolveQuantity(table, item->ingredientName, item->amount, item->hasUnit, item->unit, &amount);
		const Statement * ingredient = findIngredient(table, item->ingredientName);
		if (ingredient != NULL) {
			_addNeed(needs, &ingredient->ingredient, amount * factor);
			continue;
		}
		const Statement * subRecipe = findRecipe(table, item->ingredientName);
		if (subRecipe != NULL) {
			_expandRecipe(table, &subRecipe->recipe, factor * amount / subRecipe->recipe.yieldsAmount, needs);
		}
	}
}

static int _compareNeeds(const void * left, const void * right) {
	return strcmp(((const Need *) left)->ingredient->name, ((const Need *) right)->ingredient->name);
}

/** Formats with up to 2 decimals, without trailing zeros. */
static void _formatNumber(const double value, char * buffer, const size_t size) {
	snprintf(buffer, size, "%.2f", value);
	char * end = buffer + strlen(buffer);
	while (end > buffer && *(end - 1) == '0') {
		*--end = '\0';
	}
	if (end > buffer && *(end - 1) == '.') {
		*(end - 1) = '\0';
	}
	if (strcmp(buffer, "-0") == 0) {
		strcpy(buffer, "0");
	}
}

static void _formatMinutes(const double minutes, char * buffer, const size_t size) {
	const long total = lround(minutes);
	if (total >= 60 && total % 60 == 0) {
		snprintf(buffer, size, "%ld h", total / 60);
	}
	else if (total >= 60) {
		snprintf(buffer, size, "%ld h %ld min", total / 60, total % 60);
	}
	else {
		snprintf(buffer, size, "%ld min", total);
	}
}

static void _formatServings(const double servings, char * buffer, const size_t size) {
	char number[64];
	_formatNumber(servings, number, sizeof(number));
	snprintf(buffer, size, "%s", number);
}

static bool _makeDirectory(const char * path) {
	char partial[1024];
	snprintf(partial, sizeof(partial), "%s", path);
	for (char * cursor = partial + 1; *cursor != '\0'; ++cursor) {
		if (*cursor == '/') {
			*cursor = '\0';
			if (mkdir(partial, 0755) != 0 && errno != EEXIST) {
				return false;
			}
			*cursor = '/';
		}
	}
	return mkdir(partial, 0755) == 0 || errno == EEXIST;
}

static FILE * _openOutput(const char * directory, const char * fileName, char * fullPath, const size_t size) {
	snprintf(fullPath, size, "%s/%s", directory, fileName);
	FILE * file = fopen(fullPath, "w");
	if (file == NULL) {
		logError(_logger, "Cannot write '%s': %s.", fullPath, strerror(errno));
	}
	return file;
}

static void _writeCsvField(FILE * file, const char * value) {
	if (strpbrk(value, ",\"\n") == NULL) {
		fputs(value, file);
		return;
	}
	fputc('"', file);
	for (const char * c = value; *c != '\0'; ++c) {
		if (*c == '"') {
			fputc('"', file);
		}
		fputc(*c, file);
	}
	fputc('"', file);
}

/** Shopping lists don't ask for fractions of countable items. */
static double _purchasable(const double amount, const UnitKind unit) {
	return (unitDimension(unit) == DIMENSION_COUNT) ? ceil(amount - 1e-6) : amount;
}

static void _writeAlternatives(FILE * file, const SymbolTable * table, const Need * need) {
	char alternatives[1024] = "";
	for (size_t k = 0; k < table->substitutes.count; ++k) {
		const SubstituteDeclaration * rule = &table->substitutes.items[k]->substitute;
		if (strcmp(rule->fromIngredient, need->ingredient->name) != 0) {
			continue;
		}
		const Statement * target = findIngredient(table, rule->toIngredient);
		double amount = 0;
		if (target == NULL || !convertBetweenIngredients(need->amount * rule->ratio, need->ingredient, &target->ingredient, &amount)) {
			continue;
		}
		amount = _purchasable(amount, target->ingredient.unit);
		char number[64];
		_formatNumber(amount, number, sizeof(number));
		char entry[256];
		snprintf(entry, sizeof(entry), "%s%s: %s %s", alternatives[0] == '\0' ? "" : " | ", target->ingredient.name, number, unitDisplayName(target->ingredient.unit, amount));
		if (strlen(alternatives) + strlen(entry) < sizeof(alternatives)) {
			strcat(alternatives, entry);
		}
	}
	_writeCsvField(file, alternatives);
}

static bool _writeShoppingList(const SymbolTable * table, const MenuDeclaration * menu, const char * directory) {
	Needs needs = {0};
	for (const IncludeItem * include = menu->includes; include != NULL; include = include->next) {
		const RecipeDeclaration * recipe = &findRecipe(table, include->recipeName)->recipe;
		_expandRecipe(table, recipe, include->toServings / recipe->serves, &needs);
	}
	qsort(needs.items, needs.count, sizeof(Need), _compareNeeds);

	char fileName[512];
	snprintf(fileName, sizeof(fileName), "%s-shopping-list.csv", menu->name);
	char path[1024];
	FILE * file = _openOutput(directory, fileName, path, sizeof(path));
	if (file == NULL) {
		free(needs.items);
		return false;
	}
	fputs("ingrediente,cantidad,unidad,alternativas\n", file);
	for (size_t k = 0; k < needs.count; ++k) {
		const Need * need = &needs.items[k];
		const double amount = _purchasable(need->amount, need->ingredient->unit);
		char number[64];
		_formatNumber(amount, number, sizeof(number));
		_writeCsvField(file, need->ingredient->name);
		fprintf(file, ",%s,%s,", number, unitDisplayName(need->ingredient->unit, amount));
		_writeAlternatives(file, table, need);
		fputc('\n', file);
	}
	fclose(file);
	logInformation(_logger, "Generated %s", path);
	free(needs.items);
	return true;
}

static void _writeHtmlEscaped(FILE * file, const char * text) {
	for (const char * c = text; *c != '\0'; ++c) {
		switch (*c) {
			case '&':
				fputs("&amp;", file);
				break;
			case '<':
				fputs("&lt;", file);
				break;
			case '>':
				fputs("&gt;", file);
				break;
			case '"':
				fputs("&quot;", file);
				break;
			default:
				fputc(*c, file);
		}
	}
}

static void _writeHtmlHead(FILE * file, const char * title) {
	fputs("<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n<meta charset=\"utf-8\">\n<title>", file);
	_writeHtmlEscaped(file, title);
	fputs("</title>\n<style>\n"
		"body { font-family: Georgia, serif; max-width: 46rem; margin: 2rem auto; padding: 0 1rem; line-height: 1.5; color: #222; }\n"
		"h1 { border-bottom: 2px solid #c0392b; padding-bottom: .3rem; }\n"
		"h2 { color: #c0392b; margin-top: 2rem; }\n"
		".meta { color: #666; font-style: italic; }\n"
		"li { margin: .35rem 0; }\n"
		".detail { color: #666; font-size: .9em; }\n"
		"</style>\n</head>\n<body>\n", file);
}

static void _writeQuantity(FILE * file, const double amount, const UnitKind unit) {
	char number[64];
	_formatNumber(amount, number, sizeof(number));
	fprintf(file, "%s %s", number, unitDisplayName(unit, amount));
}

/** Writes one recipe, multiplied by a factor, with its steps in dependency order. */
static void _writeRecipe(FILE * file, const SymbolTable * table, const RecipeDeclaration * recipe, const double servings) {
	const double factor = servings / recipe->serves;
	char number[64];
	fputs("<h2>", file);
	_writeHtmlEscaped(file, recipe->name);
	fputs("</h2>\n<p class=\"meta\">Para ", file);
	_formatServings(servings, number, sizeof(number));
	fprintf(file, "%s %s (la receta original es para ", number, fabs(servings - 1.0) < EPSILON ? "porción" : "porciones");
	_formatServings(recipe->serves, number, sizeof(number));
	fprintf(file, "%s).</p>\n<h3>Ingredientes</h3>\n<ul>\n", number);
	for (const RequiresItem * item = recipe->requires; item != NULL; item = item->next) {
		UnitKind unit = item->unit;
		if (!item->hasUnit) {
			nativeUnitOf(table, item->ingredientName, &unit);
		}
		fputs("<li>", file);
		_writeQuantity(file, item->amount * factor, unit);
		fputs(" de ", file);
		_writeHtmlEscaped(file, item->ingredientName);
		fputs("</li>\n", file);
	}
	fputs("</ul>\n", file);

	size_t total = 0;
	StepDeclaration ** ordered = NULL;
	const size_t placed = sortSteps(recipe, &total, &ordered);
	if (placed > 0) {
		fputs("<h3>Pasos</h3>\n<ol>\n", file);
		double sum = 0;
		for (size_t k = 0; k < placed; ++k) {
			const StepDeclaration * step = ordered[k];
			fputs("<li><strong>", file);
			_writeHtmlEscaped(file, step->name);
			fputs("</strong>: ", file);
			_writeHtmlEscaped(file, step->description);
			if (step->uses != NULL) {
				fputs("<br><span class=\"detail\">Usa: ", file);
				for (const UseItem * use = step->uses; use != NULL; use = use->next) {
					UnitKind unit = use->unit;
					if (!use->hasUnit) {
						nativeUnitOf(table, use->ingredientName, &unit);
					}
					_writeQuantity(file, use->amount * factor, unit);
					fputs(" de ", file);
					_writeHtmlEscaped(file, use->ingredientName);
					fputs(use->next != NULL ? ", " : "", file);
				}
				fputs(".</span>", file);
			}
			if (step->dependsOn != NULL) {
				fputs("<br><span class=\"detail\">Después de: ", file);
				for (const IdentifierList * dependency = step->dependsOn; dependency != NULL; dependency = dependency->next) {
					_writeHtmlEscaped(file, dependency->value);
					fputs(dependency->next != NULL ? ", " : "", file);
				}
				fputs(".</span>", file);
			}
			if (step->hasTakes) {
				char duration[64];
				_formatMinutes(step->minutes, duration, sizeof(duration));
				fprintf(file, "<br><span class=\"detail\">Duración: %s.</span>", duration);
				sum += step->minutes;
			}
			fputs("</li>\n", file);
		}
		fputs("</ol>\n", file);
		if (sum > 0) {
			char work[64];
			char critical[64];
			_formatMinutes(sum, work, sizeof(work));
			_formatMinutes(criticalPathMinutes(recipe), critical, sizeof(critical));
			fprintf(file, "<p class=\"meta\">Tiempo total de trabajo: %s. Tiempo mínimo si los pasos independientes se hacen en paralelo: %s.</p>\n", work, critical);
		}
	}
	free(ordered);
}

static bool _writeScaledRecipe(const SymbolTable * table, const ScaleStatement * scale, const char * directory) {
	char servings[64];
	_formatServings(scale->toServings, servings, sizeof(servings));
	char fileName[512];
	snprintf(fileName, sizeof(fileName), "%s-%s-servings.html", scale->recipeName, servings);
	char path[1024];
	FILE * file = _openOutput(directory, fileName, path, sizeof(path));
	if (file == NULL) {
		return false;
	}
	_writeHtmlHead(file, scale->recipeName);
	fputs("<h1>Receta escalada</h1>\n", file);
	_writeRecipe(file, table, &findRecipe(table, scale->recipeName)->recipe, scale->toServings);
	fputs("</body>\n</html>\n", file);
	fclose(file);
	logInformation(_logger, "Generated %s", path);
	return true;
}

static bool _writeMenuRecipes(const SymbolTable * table, const MenuDeclaration * menu, const char * directory) {
	char fileName[512];
	snprintf(fileName, sizeof(fileName), "%s-recipes.html", menu->name);
	char path[1024];
	FILE * file = _openOutput(directory, fileName, path, sizeof(path));
	if (file == NULL) {
		return false;
	}
	_writeHtmlHead(file, menu->name);
	fputs("<h1>Menú ", file);
	_writeHtmlEscaped(file, menu->name);
	fputs("</h1>\n", file);
	for (const IncludeItem * include = menu->includes; include != NULL; include = include->next) {
		_writeRecipe(file, table, &findRecipe(table, include->recipeName)->recipe, include->toServings);
	}
	fputs("</body>\n</html>\n", file);
	fclose(file);
	logInformation(_logger, "Generated %s", path);
	return true;
}

/* PUBLIC FUNCTIONS */

CompilationStatus executeCodeGeneration(const Program * program, const SymbolTable * table, const char * outputDirectory) {
	logDebugging(_logger, "Generating code...");
	if (!_makeDirectory(outputDirectory)) {
		logError(_logger, "Cannot create the output directory '%s': %s.", outputDirectory, strerror(errno));
		return FAILED;
	}
	bool succeeded = true;
	for (const Statement * statement = program->statements; statement != NULL; statement = statement->next) {
		if (statement->type == SCALE_STATEMENT) {
			succeeded = _writeScaledRecipe(table, &statement->scale, outputDirectory) && succeeded;
		}
		else if (statement->type == GENERATE_STATEMENT) {
			const MenuDeclaration * menu = &findMenu(table, statement->generate.menuName)->menu;
			succeeded = _writeShoppingList(table, menu, outputDirectory) && succeeded;
			succeeded = _writeMenuRecipes(table, menu, outputDirectory) && succeeded;
		}
	}
	logDebugging(_logger, "Code generation is done.");
	return succeeded ? SUCCEEDED : FAILED;
}
