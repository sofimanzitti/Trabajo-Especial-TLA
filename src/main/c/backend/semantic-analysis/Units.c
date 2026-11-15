#include "Units.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

UnitDimension unitDimension(const UnitKind unit) {
	switch (unit) {
		case UNIT_GRAMS:
		case UNIT_KILOGRAMS:
			return DIMENSION_MASS;
		case UNIT_CUPS:
		case UNIT_LITERS:
		case UNIT_MILLILITERS:
		case UNIT_PINCHES:
		case UNIT_TABLESPOONS:
		case UNIT_TEASPOONS:
			return DIMENSION_VOLUME;
		case UNIT_UNITS:
		default:
			return DIMENSION_COUNT;
	}
}

double unitFactor(const UnitKind unit) {
	switch (unit) {
		case UNIT_CUPS:
			return 240.0;
		case UNIT_KILOGRAMS:
		case UNIT_LITERS:
			return 1000.0;
		case UNIT_PINCHES:
			return 0.3125;
		case UNIT_TABLESPOONS:
			return 15.0;
		case UNIT_TEASPOONS:
			return 5.0;
		case UNIT_GRAMS:
		case UNIT_MILLILITERS:
		case UNIT_UNITS:
		default:
			return 1.0;
	}
}

void formatAmount(const double value, char * buffer, const size_t size) {
	int decimals = 2;
	const double magnitude = fabs(value);
	while (magnitude > 0 && magnitude * pow(10, decimals) < 10 && decimals < 9) {
		++decimals;
	}
	snprintf(buffer, size, "%.*f", decimals, value);
	if (strchr(buffer, '.') != NULL) {
		char * end = buffer + strlen(buffer);
		while (end > buffer && *(end - 1) == '0') {
			*--end = '\0';
		}
		if (end > buffer && *(end - 1) == '.') {
			*(end - 1) = '\0';
		}
	}
	if (strcmp(buffer, "-0") == 0) {
		strcpy(buffer, "0");
	}
}

bool isSingularAmount(const double amount) {
	// Decided with the same rounding that is used to print the amount.
	return fabs(round(amount * 100.0) / 100.0 - 1.0) < 1e-9;
}

const char * unitDisplayName(const UnitKind unit, const double amount) {
	const bool singular = isSingularAmount(amount);
	switch (unit) {
		case UNIT_CUPS:
			return singular ? "taza" : "tazas";
		case UNIT_GRAMS:
			return singular ? "gramo" : "gramos";
		case UNIT_KILOGRAMS:
			return singular ? "kilogramo" : "kilogramos";
		case UNIT_LITERS:
			return singular ? "litro" : "litros";
		case UNIT_MILLILITERS:
			return singular ? "mililitro" : "mililitros";
		case UNIT_PINCHES:
			return singular ? "pizca" : "pizcas";
		case UNIT_TABLESPOONS:
			return singular ? "cucharada" : "cucharadas";
		case UNIT_TEASPOONS:
			return singular ? "cucharadita" : "cucharaditas";
		case UNIT_UNITS:
		default:
			return singular ? "unidad" : "unidades";
	}
}

bool convertQuantity(const double amount, const UnitKind from, const UnitKind to, const bool hasDensity, const double density, double * result) {
	const UnitDimension fromDimension = unitDimension(from);
	const UnitDimension toDimension = unitDimension(to);
	double base = amount * unitFactor(from);
	if (fromDimension != toDimension) {
		if (fromDimension == DIMENSION_COUNT || toDimension == DIMENSION_COUNT || !hasDensity || density <= 0) {
			return false;
		}
		base = (fromDimension == DIMENSION_MASS) ? base / density : base * density;
	}
	*result = base / unitFactor(to);
	return true;
}

bool convertBetweenIngredients(const double amount, const IngredientDeclaration * from, const IngredientDeclaration * to, double * result) {
	if (unitDimension(from->unit) == unitDimension(to->unit)) {
		return convertQuantity(amount, from->unit, to->unit, false, 0, result);
	}
	if (to->hasDensity) {
		return convertQuantity(amount, from->unit, to->unit, true, to->density, result);
	}
	return convertQuantity(amount, from->unit, to->unit, from->hasDensity, from->density, result);
}
