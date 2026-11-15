#include "SemanticAnalysis.h"
#include "StepGraph.h"
#include "Units.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;
static int _errors = 0;

/** Shutdown module's internal state. */
void _shutdownSemanticAnalysisModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: SemanticAnalysis...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeSemanticAnalysisModule() {
	_logger = createLogger("SemanticAnalysis");
	return _shutdownSemanticAnalysisModule;
}

/* PRIVATE FUNCTIONS */

#define TOLERANCE 1e-9

#define REPORT(line, ...) \
	do { \
		++_errors; \
		char _message[512]; \
		snprintf(_message, sizeof(_message), __VA_ARGS__); \
		logError(_logger, "Line %d: %s.", (line), _message); \
	} while (0)

static bool _isDeclaredName(const SymbolTable * table, const char * name) {
	return findIngredient(table, name) != NULL || findRecipe(table, name) != NULL;
}

/** Reports a failed resolution of a quantity; returns true if it was valid. */
static bool _checkResolution(const ItemResolution resolution, const int line, const char * context, const char * name) {
	switch (resolution) {
		case ITEM_OK:
			return true;
		case ITEM_UNDECLARED:
			REPORT(line, "%s: '%s' is not a declared ingredient or recipe", context, name);
			break;
		case ITEM_RECIPE_WITHOUT_YIELDS:
			REPORT(line, "%s: recipe '%s' is used as an ingredient, so it must declare 'yields'", context, name);
			break;
		case ITEM_INCOMPATIBLE_UNIT:
			REPORT(line, "%s: the unit is not compatible with the unit of '%s'", context, name);
			break;
		case ITEM_NEEDS_DENSITY:
			REPORT(line, "%s: converting between mass and volume requires '%s' to declare a density", context, name);
			break;
	}
	return false;
}

static void _analyzeIngredient(SymbolTable * table, Statement * statement) {
	const IngredientDeclaration * ingredient = &statement->ingredient;
	if (_isDeclaredName(table, ingredient->name)) {
		REPORT(statement->line, "The name '%s' is already used by another ingredient or recipe", ingredient->name);
	}
	if (ingredient->hasDensity && ingredient->density <= 0) {
		REPORT(statement->line, "The density of '%s' must be greater than zero", ingredient->name);
	}
	if (ingredient->hasDensity && unitDimension(ingredient->unit) == DIMENSION_COUNT) {
		logWarning(_logger, "Line %d: ingredient '%s' is measured in units, so its density is meaningless and will be ignored.", statement->line, ingredient->name);
	}
	addToStatementList(&table->ingredients, statement);
}

/** Position of an ingredient among the "requires" of a recipe, or -1. */
static int _requiresIndex(const RecipeDeclaration * recipe, const char * name) {
	int index = 0;
	for (const RequiresItem * item = recipe->requires; item != NULL; item = item->next, ++index) {
		if (strcmp(item->ingredientName, name) == 0) {
			return index;
		}
	}
	return -1;
}

static void _analyzeStepGraph(const RecipeDeclaration * recipe) {
	size_t total = 0;
	StepDeclaration ** ordered = NULL;
	const size_t placed = sortSteps(recipe, &total, &ordered);
	if (placed < total) {
		char names[384] = "";
		int line = 0;
		for (const StepDeclaration * step = recipe->steps; step != NULL; step = step->next) {
			bool isPlaced = false;
			for (size_t k = 0; k < placed; ++k) {
				isPlaced = isPlaced || ordered[k] == step;
			}
			if (!isPlaced) {
				if (line == 0) {
					line = step->line;
				}
				if (strlen(names) + strlen(step->name) + 4 < sizeof(names)) {
					strcat(names, names[0] == '\0' ? "" : ", ");
					strcat(names, step->name);
				}
			}
		}
		REPORT(line, "Recipe '%s' has a dependency cycle among its steps (%s)", recipe->name, names);
	}
	free(ordered);
}

static void _analyzeRecipe(SymbolTable * table, Statement * statement) {
	const RecipeDeclaration * recipe = &statement->recipe;
	const int line = statement->line;
	if (_isDeclaredName(table, recipe->name)) {
		REPORT(line, "The name '%s' is already used by another ingredient or recipe", recipe->name);
	}
	if (recipe->hasServes && recipe->serves <= 0) {
		REPORT(line, "Recipe '%s': 'serves' must be greater than zero", recipe->name);
	}
	if (recipe->hasYields && recipe->yieldsAmount <= 0) {
		REPORT(line, "Recipe '%s': 'yields' must be greater than zero", recipe->name);
	}

	// Requirements: declared ingredients or recipes, valid units, no repetitions.
	int requiredCount = 0;
	for (const RequiresItem * item = recipe->requires; item != NULL; item = item->next) {
		++requiredCount;
		char context[256];
		snprintf(context, sizeof(context), "requires in recipe '%s'", recipe->name);
		if (strcmp(item->ingredientName, recipe->name) == 0) {
			REPORT(item->line, "Recipe '%s' cannot require itself", recipe->name);
			continue;
		}
		if (item->amount <= 0) {
			REPORT(item->line, "%s: the amount of '%s' must be greater than zero", context, item->ingredientName);
		}
		double ignored;
		_checkResolution(resolveQuantity(table, item->ingredientName, item->amount, item->hasUnit, item->unit, &ignored), item->line, context, item->ingredientName);
		if (_requiresIndex(recipe, item->ingredientName) != requiredCount - 1) {
			REPORT(item->line, "%s: '%s' is required more than once", context, item->ingredientName);
		}
	}

	double * required = (double *) calloc(requiredCount == 0 ? 1 : requiredCount, sizeof(double));
	double * used = (double *) calloc(requiredCount == 0 ? 1 : requiredCount, sizeof(double));
	int index = 0;
	for (const RequiresItem * item = recipe->requires; item != NULL; item = item->next, ++index) {
		double amount = 0;
		if (resolveQuantity(table, item->ingredientName, item->amount, item->hasUnit, item->unit, &amount) == ITEM_OK) {
			required[index] += amount;
		}
	}

	// Steps: unique names, declared dependencies, uses bounded by the requirements.
	for (const StepDeclaration * step = recipe->steps; step != NULL; step = step->next) {
		for (const StepDeclaration * previous = recipe->steps; previous != step; previous = previous->next) {
			if (strcmp(previous->name, step->name) == 0) {
				REPORT(step->line, "Recipe '%s' declares the step '%s' more than once", recipe->name, step->name);
				break;
			}
		}
		if (step->hasTakes && step->minutes <= 0) {
			REPORT(step->line, "Step '%s': 'takes' must be greater than zero", step->name);
		}
		for (const IdentifierList * dependency = step->dependsOn; dependency != NULL; dependency = dependency->next) {
			bool found = false;
			for (const StepDeclaration * other = recipe->steps; other != NULL; other = other->next) {
				found = found || strcmp(other->name, dependency->value) == 0;
			}
			if (strcmp(dependency->value, step->name) == 0) {
				REPORT(step->line, "Step '%s' cannot depend on itself", step->name);
			}
			else if (!found) {
				REPORT(step->line, "Step '%s' depends on '%s', which is not a step of recipe '%s'", step->name, dependency->value, recipe->name);
			}
		}
		for (const UseItem * use = step->uses; use != NULL; use = use->next) {
			char context[256];
			snprintf(context, sizeof(context), "uses in step '%s'", step->name);
			if (use->amount <= 0) {
				REPORT(step->line, "%s: the amount of '%s' must be greater than zero", context, use->ingredientName);
			}
			const int position = _requiresIndex(recipe, use->ingredientName);
			if (position < 0) {
				REPORT(step->line, "%s: '%s' is not required by recipe '%s'", context, use->ingredientName, recipe->name);
				continue;
			}
			double amount = 0;
			if (_checkResolution(resolveQuantity(table, use->ingredientName, use->amount, use->hasUnit, use->unit, &amount), step->line, context, use->ingredientName)) {
				used[position] += amount;
			}
		}
	}
	_analyzeStepGraph(recipe);

	index = 0;
	bool anyStep = recipe->steps != NULL;
	for (const RequiresItem * item = recipe->requires; item != NULL; item = item->next, ++index) {
		if (used[index] > required[index] * (1 + TOLERANCE) + TOLERANCE) {
			REPORT(item->line, "Recipe '%s': the steps use %g of '%s' (in its own unit) but the recipe only requires %g", recipe->name, used[index], item->ingredientName, required[index]);
		}
		else if (anyStep && used[index] == 0 && required[index] > 0) {
			logWarning(_logger, "Line %d: recipe '%s' requires '%s' but no step uses it.", item->line, recipe->name, item->ingredientName);
		}
	}
	free(required);
	free(used);
	addToStatementList(&table->recipes, statement);
}

/** True if following substitution rules from "start" reaches "target". */
static bool _substitutionReaches(const SymbolTable * table, const char * start, const char * target, const int depth) {
	if (strcmp(start, target) == 0) {
		return true;
	}
	if (depth > (int) table->substitutes.count) {
		return false;
	}
	for (size_t k = 0; k < table->substitutes.count; ++k) {
		const SubstituteDeclaration * rule = &table->substitutes.items[k]->substitute;
		if (strcmp(rule->fromIngredient, start) == 0 && _substitutionReaches(table, rule->toIngredient, target, depth + 1)) {
			return true;
		}
	}
	return false;
}

static void _analyzeSubstitute(SymbolTable * table, Statement * statement) {
	const SubstituteDeclaration * rule = &statement->substitute;
	const int line = statement->line;
	const Statement * from = findIngredient(table, rule->fromIngredient);
	const Statement * to = findIngredient(table, rule->toIngredient);
	bool valid = true;
	if (from == NULL) {
		REPORT(line, "substitute: '%s' is not a declared ingredient", rule->fromIngredient);
		valid = false;
	}
	if (to == NULL) {
		REPORT(line, "substitute: '%s' is not a declared ingredient", rule->toIngredient);
		valid = false;
	}
	if (rule->ratio <= 0) {
		REPORT(line, "substitute: the ratio must be greater than zero");
	}
	if (!valid) {
		return;
	}
	if (strcmp(rule->fromIngredient, rule->toIngredient) == 0) {
		REPORT(line, "substitute: '%s' cannot be substituted by itself", rule->fromIngredient);
		return;
	}
	double converted;
	if (!convertBetweenIngredients(1, &from->ingredient, &to->ingredient, &converted)) {
		if (unitDimension(from->ingredient.unit) == DIMENSION_COUNT || unitDimension(to->ingredient.unit) == DIMENSION_COUNT) {
			REPORT(line, "substitute: '%s' and '%s' are measured in incompatible units", rule->fromIngredient, rule->toIngredient);
		}
		else {
			REPORT(line, "substitute: '%s' and '%s' have different dimension (mass vs. volume) and neither declares a density", rule->fromIngredient, rule->toIngredient);
		}
		return;
	}
	for (size_t k = 0; k < table->substitutes.count; ++k) {
		const SubstituteDeclaration * previous = &table->substitutes.items[k]->substitute;
		if (strcmp(previous->fromIngredient, rule->fromIngredient) == 0 && strcmp(previous->toIngredient, rule->toIngredient) == 0) {
			REPORT(line, "substitute: the rule '%s' with '%s' is already defined", rule->fromIngredient, rule->toIngredient);
			return;
		}
	}
	if (_substitutionReaches(table, rule->toIngredient, rule->fromIngredient, 0)) {
		REPORT(line, "substitute: '%s' with '%s' creates a circular substitution", rule->fromIngredient, rule->toIngredient);
		return;
	}
	addToStatementList(&table->substitutes, statement);
}

static const RecipeDeclaration * _scalableRecipe(const SymbolTable * table, const char * name, const double servings, const int line, const char * context) {
	const Statement * recipe = findRecipe(table, name);
	if (recipe == NULL) {
		REPORT(line, "%s: '%s' is not a declared recipe", context, name);
		return NULL;
	}
	if (servings <= 0) {
		REPORT(line, "%s: the number of servings must be greater than zero", context);
	}
	if (!recipe->recipe.hasServes) {
		REPORT(line, "%s: recipe '%s' cannot be scaled because it does not declare 'serves'", context, name);
		return NULL;
	}
	return &recipe->recipe;
}

static void _analyzeScale(SymbolTable * table, const Program * program, Statement * statement) {
	_scalableRecipe(table, statement->scale.recipeName, statement->scale.toServings, statement->line, "scale");
	// Each scale writes "<recipe>-<servings>-servings.html": compare the servings as they are printed.
	char servings[64];
	formatAmount(statement->scale.toServings, servings, sizeof(servings));
	for (const Statement * previous = program->statements; previous != statement; previous = previous->next) {
		if (previous->type != SCALE_STATEMENT || strcmp(previous->scale.recipeName, statement->scale.recipeName) != 0) {
			continue;
		}
		char other[64];
		formatAmount(previous->scale.toServings, other, sizeof(other));
		if (strcmp(servings, other) == 0) {
			REPORT(statement->line, "scale: '%s' is already scaled to %s servings (line %d)", statement->scale.recipeName, servings, previous->line);
			break;
		}
	}
}

static void _analyzeMenu(SymbolTable * table, Statement * statement) {
	const MenuDeclaration * menu = &statement->menu;
	if (findMenu(table, menu->name) != NULL) {
		REPORT(statement->line, "The menu '%s' is already declared", menu->name);
	}
	for (const IncludeItem * include = menu->includes; include != NULL; include = include->next) {
		_scalableRecipe(table, include->recipeName, include->toServings, include->line, "include");
	}
	addToStatementList(&table->menus, statement);
}

static void _analyzeGenerate(SymbolTable * table, Statement * statement) {
	if (findMenu(table, statement->generate.menuName) == NULL) {
		REPORT(statement->line, "generate shopping list: '%s' is not a declared menu", statement->generate.menuName);
	}
}

/* PUBLIC FUNCTIONS */

CompilationStatus executeSemanticAnalysis(const Program * program, SymbolTable * table) {
	logDebugging(_logger, "Analyzing semantics...");
	_errors = 0;
	for (Statement * statement = (program == NULL) ? NULL : program->statements; statement != NULL; statement = statement->next) {
		switch (statement->type) {
			case INGREDIENT_STATEMENT:
				_analyzeIngredient(table, statement);
				break;
			case RECIPE_STATEMENT:
				_analyzeRecipe(table, statement);
				break;
			case SUBSTITUTE_STATEMENT:
				_analyzeSubstitute(table, statement);
				break;
			case SCALE_STATEMENT:
				_analyzeScale(table, program, statement);
				break;
			case MENU_STATEMENT:
				_analyzeMenu(table, statement);
				break;
			case GENERATE_STATEMENT:
				_analyzeGenerate(table, statement);
				break;
		}
	}
	if (_errors > 0) {
		logError(_logger, "The semantic-analysis phase found %d error(s).", _errors);
		return FAILED;
	}
	logDebugging(_logger, "Semantic analysis is done.");
	return SUCCEEDED;
}
