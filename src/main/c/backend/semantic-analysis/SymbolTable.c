#include "SymbolTable.h"
#include "Units.h"
#include <stdlib.h>
#include <string.h>

SymbolTable * createSymbolTable() {
	return (SymbolTable *) calloc(1, sizeof(SymbolTable));
}

void destroySymbolTable(SymbolTable * table) {
	if (table != NULL) {
		free(table->ingredients.items);
		free(table->recipes.items);
		free(table->menus.items);
		free(table->substitutes.items);
		free(table);
	}
}

void addToStatementList(StatementList * list, Statement * statement) {
	if (list->count == list->capacity) {
		list->capacity = (list->capacity == 0) ? 8 : list->capacity * 2;
		list->items = (Statement **) realloc(list->items, list->capacity * sizeof(Statement *));
	}
	list->items[list->count++] = statement;
}

static Statement * _find(const StatementList * list, const char * name) {
	for (size_t k = 0; k < list->count; ++k) {
		const Statement * statement = list->items[k];
		const char * candidate = NULL;
		switch (statement->type) {
			case INGREDIENT_STATEMENT:
				candidate = statement->ingredient.name;
				break;
			case RECIPE_STATEMENT:
				candidate = statement->recipe.name;
				break;
			case MENU_STATEMENT:
				candidate = statement->menu.name;
				break;
			default:
				break;
		}
		if (candidate != NULL && strcmp(candidate, name) == 0) {
			return list->items[k];
		}
	}
	return NULL;
}

Statement * findIngredient(const SymbolTable * table, const char * name) {
	return _find(&table->ingredients, name);
}

Statement * findRecipe(const SymbolTable * table, const char * name) {
	return _find(&table->recipes, name);
}

Statement * findMenu(const SymbolTable * table, const char * name) {
	return _find(&table->menus, name);
}

bool nativeUnitOf(const SymbolTable * table, const char * name, UnitKind * unit) {
	const Statement * ingredient = findIngredient(table, name);
	if (ingredient != NULL) {
		*unit = ingredient->ingredient.unit;
		return true;
	}
	const Statement * recipe = findRecipe(table, name);
	if (recipe != NULL && recipe->recipe.hasYields) {
		*unit = recipe->recipe.yieldsUnit;
		return true;
	}
	return false;
}

ItemResolution resolveQuantity(const SymbolTable * table, const char * name, const double amount, const bool hasUnit, const UnitKind unit, double * nativeAmount) {
	const Statement * ingredient = findIngredient(table, name);
	if (ingredient != NULL) {
		const IngredientDeclaration * declaration = &ingredient->ingredient;
		if (!hasUnit) {
			*nativeAmount = amount;
			return ITEM_OK;
		}
		if (convertQuantity(amount, unit, declaration->unit, declaration->hasDensity, declaration->density, nativeAmount)) {
			return ITEM_OK;
		}
		const bool massVolume = unitDimension(unit) != DIMENSION_COUNT && unitDimension(declaration->unit) != DIMENSION_COUNT;
		return massVolume ? ITEM_NEEDS_DENSITY : ITEM_INCOMPATIBLE_UNIT;
	}
	const Statement * recipe = findRecipe(table, name);
	if (recipe != NULL) {
		if (!recipe->recipe.hasYields) {
			return ITEM_RECIPE_WITHOUT_YIELDS;
		}
		if (!hasUnit) {
			*nativeAmount = amount;
			return ITEM_OK;
		}
		// A recipe has no density, so only units of its own dimension are valid.
		return convertQuantity(amount, unit, recipe->recipe.yieldsUnit, false, 0, nativeAmount) ? ITEM_OK : ITEM_INCOMPATIBLE_UNIT;
	}
	return ITEM_UNDECLARED;
}
