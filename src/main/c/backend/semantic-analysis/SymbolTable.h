#ifndef SYMBOL_TABLE_HEADER
#define SYMBOL_TABLE_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include <stdbool.h>
#include <stddef.h>

/** A growable list of (borrowed) pointers to AST statements. */
typedef struct {
	Statement ** items;
	size_t count;
	size_t capacity;
} StatementList;

/**
 * Declarations seen so far. The nodes are owned by the AST, so the table must
 * not outlive it. Ingredients and recipes share a single namespace (both can
 * be requested by a "requires"); menus have their own.
 */
typedef struct {
	StatementList ingredients;
	StatementList recipes;
	StatementList menus;
	StatementList substitutes;
} SymbolTable;

/** How an ingredient or recipe name was resolved by a quantity. */
typedef enum {
	ITEM_OK,
	ITEM_UNDECLARED,
	ITEM_RECIPE_WITHOUT_YIELDS,
	ITEM_INCOMPATIBLE_UNIT,
	ITEM_NEEDS_DENSITY
} ItemResolution;

SymbolTable * createSymbolTable();
void destroySymbolTable(SymbolTable * table);

void addToStatementList(StatementList * list, Statement * statement);

Statement * findIngredient(const SymbolTable * table, const char * name);
Statement * findRecipe(const SymbolTable * table, const char * name);
Statement * findMenu(const SymbolTable * table, const char * name);

/**
 * Expresses a quantity of an ingredient (or of a recipe used as ingredient) in
 * its native unit: the one of the ingredient, or the one that the recipe
 * yields. When the quantity has no unit, it is already in the native one.
 */
ItemResolution resolveQuantity(const SymbolTable * table, const char * name, const double amount, const bool hasUnit, const UnitKind unit, double * nativeAmount);

/** The unit in which an ingredient or recipe is measured; false if it isn't declared. */
bool nativeUnitOf(const SymbolTable * table, const char * name, UnitKind * unit);

#endif
