#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

typedef enum StatementType StatementType;
typedef enum UnitKind UnitKind;

typedef struct AssignmentOption AssignmentOption;
typedef struct GenerateStatement GenerateStatement;
typedef struct IdentifierList IdentifierList;
typedef struct IncludeItem IncludeItem;
typedef struct IngredientDeclaration IngredientDeclaration;
typedef struct MenuDeclaration MenuDeclaration;
typedef struct NumberOption NumberOption;
typedef struct Program Program;
typedef struct RecipeDeclaration RecipeDeclaration;
typedef struct RequiresItem RequiresItem;
typedef struct ScaleStatement ScaleStatement;
typedef struct Statement Statement;
typedef struct StepDeclaration StepDeclaration;
typedef struct SubstituteDeclaration SubstituteDeclaration;
typedef struct UnitOption UnitOption;
typedef struct UseItem UseItem;
typedef struct YieldsOption YieldsOption;

enum UnitKind {
	UNIT_CUPS,
	UNIT_GRAMS,
	UNIT_KILOGRAMS,
	UNIT_LITERS,
	UNIT_MILLILITERS,
	UNIT_TABLESPOONS,
	UNIT_TEASPOONS,
	UNIT_UNITS
};

enum StatementType {
	GENERATE_STATEMENT,
	INGREDIENT_STATEMENT,
	MENU_STATEMENT,
	RECIPE_STATEMENT,
	SCALE_STATEMENT,
	SUBSTITUTE_STATEMENT
};

struct NumberOption {
	bool present;
	double value;
};

struct UnitOption {
	bool present;
	UnitKind unit;
};

struct YieldsOption {
	bool present;
	double amount;
	UnitKind unit;
};

struct AssignmentOption {
	bool present;
	char * variableName;
};

struct IdentifierList {
	char * value;
	IdentifierList * next;
};

struct RequiresItem {
	double amount;
	bool hasUnit;
	UnitKind unit;
	char * ingredientName;
	RequiresItem * next;
};

struct UseItem {
	double amount;
	bool hasUnit;
	UnitKind unit;
	char * ingredientName;
	UseItem * next;
};

struct StepDeclaration {
	char * name;
	UseItem * uses;
	IdentifierList * dependsOn;
	bool hasTakes;
	double minutes;
	char * description;
	StepDeclaration * next;
};

struct IncludeItem {
	char * recipeName;
	double toServings;
	IncludeItem * next;
};

struct IngredientDeclaration {
	char * name;
	UnitKind unit;
	bool hasDensity;
	double density;
};

struct RecipeDeclaration {
	char * name;
	bool hasServes;
	double serves;
	bool hasYields;
	double yieldsAmount;
	UnitKind yieldsUnit;
	RequiresItem * requires;
	StepDeclaration * steps;
};

struct SubstituteDeclaration {
	char * fromIngredient;
	char * toIngredient;
	double ratio;
};

struct ScaleStatement {
	bool hasAssignment;
	char * variableName;
	char * recipeName;
	double toServings;
};

struct MenuDeclaration {
	char * name;
	IncludeItem * includes;
};

struct GenerateStatement {
	char * menuName;
};

struct Statement {
	union {
		GenerateStatement generate;
		IngredientDeclaration ingredient;
		MenuDeclaration menu;
		RecipeDeclaration recipe;
		ScaleStatement scale;
		SubstituteDeclaration substitute;
	};
	StatementType type;
	Statement * next;
};

struct Program {
	Statement * statements;
};

void destroyAssignmentOption(AssignmentOption * assignmentOption);
void destroyIdentifierList(IdentifierList * identifierList);
void destroyIncludeItem(IncludeItem * includeItem);
void destroyNumberOption(NumberOption * numberOption);
void destroyProgram(Program * program);
void destroyRequiresItem(RequiresItem * requiresItem);
void destroyStatement(Statement * statement);
void destroyStepDeclaration(StepDeclaration * stepDeclaration);
void destroyUnitOption(UnitOption * unitOption);
void destroyUseItem(UseItem * useItem);
void destroyYieldsOption(YieldsOption * yieldsOption);

#endif
