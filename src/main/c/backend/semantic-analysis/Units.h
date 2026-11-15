#ifndef UNITS_HEADER
#define UNITS_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * Physical magnitude measured by a unit. Units of the same dimension are
 * always convertible between them; mass and volume are convertible only when
 * a density (in g/ml) is known; counts are never convertible to anything else.
 */
typedef enum {
	DIMENSION_COUNT,
	DIMENSION_MASS,
	DIMENSION_VOLUME
} UnitDimension;

UnitDimension unitDimension(const UnitKind unit);

/** Factor that converts one unit into the base one of its dimension (g, ml, unit). */
double unitFactor(const UnitKind unit);

/**
 * Formats an amount with up to 2 decimals, without trailing zeros. Amounts
 * smaller than 0.1 get extra decimals so that they never collapse into "0".
 * It is the only way amounts are printed, including in output file names.
 */
void formatAmount(const double value, char * buffer, const size_t size);

/** True if the amount, rounded to 2 decimals as it is printed, is exactly 1. */
bool isSingularAmount(const double amount);

/** Spanish display name (e.g. "taza" or "tazas"), singular only when the printed amount is 1. */
const char * unitDisplayName(const UnitKind unit, const double amount);

/**
 * Converts an amount between units. Density is expressed in g/ml and is
 * needed only when the units have different dimension (mass vs. volume).
 * Returns false if the conversion is not possible.
 */
bool convertQuantity(const double amount, const UnitKind from, const UnitKind to, const bool hasDensity, const double density, double * result);

/**
 * Converts an amount from the unit of an ingredient to the unit of another
 * one, using the density of the ingredient that has it (preferring the one
 * of the destination). Returns false if the conversion is not possible.
 */
bool convertBetweenIngredients(const double amount, const IngredientDeclaration * from, const IngredientDeclaration * to, double * result);

#endif
