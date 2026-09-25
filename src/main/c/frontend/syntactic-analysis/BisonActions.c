#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

AssignmentOption * AssignmentOptionSemanticAction(char * variableName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	AssignmentOption * assignmentOption = calloc(1, sizeof(AssignmentOption));
	assignmentOption->present = true;
	assignmentOption->variableName = variableName;
	return assignmentOption;
}

AssignmentOption * NoAssignmentOptionSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	AssignmentOption * assignmentOption = calloc(1, sizeof(AssignmentOption));
	assignmentOption->present = false;
	assignmentOption->variableName = NULL;
	return assignmentOption;
}

IdentifierList * IdentifierListSemanticAction(char * identifier) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	IdentifierList * identifierList = calloc(1, sizeof(IdentifierList));
	identifierList->value = identifier;
	identifierList->next = NULL;
	return identifierList;
}

IdentifierList * AppendIdentifierSemanticAction(IdentifierList * list, char * identifier) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	IdentifierList * item = IdentifierListSemanticAction(identifier);
	if (list == NULL) {
		return item;
	}
	IdentifierList * last = list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = item;
	return list;
}

IncludeItem * IncludeItemSemanticAction(char * recipeName, const double toServings) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	IncludeItem * includeItem = calloc(1, sizeof(IncludeItem));
	includeItem->recipeName = recipeName;
	includeItem->toServings = toServings;
	includeItem->next = NULL;
	return includeItem;
}

IncludeItem * AppendIncludeItemSemanticAction(IncludeItem * list, IncludeItem * item) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (list == NULL) {
		return item;
	}
	IncludeItem * last = list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = item;
	return list;
}

NumberOption * NoNumberOptionSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	NumberOption * numberOption = calloc(1, sizeof(NumberOption));
	numberOption->present = false;
	numberOption->value = 0;
	return numberOption;
}

NumberOption * NumberOptionSemanticAction(const double value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	NumberOption * numberOption = calloc(1, sizeof(NumberOption));
	numberOption->present = true;
	numberOption->value = value;
	return numberOption;
}

RequiresItem * RequiresItemSemanticAction(const double amount, UnitOption * unit, char * ingredientName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	RequiresItem * requiresItem = calloc(1, sizeof(RequiresItem));
	requiresItem->amount = amount;
	requiresItem->hasUnit = unit->present;
	requiresItem->unit = unit->unit;
	requiresItem->ingredientName = ingredientName;
	requiresItem->next = NULL;
	destroyUnitOption(unit);
	return requiresItem;
}

RequiresItem * AppendRequiresItemSemanticAction(RequiresItem * list, RequiresItem * item) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (list == NULL) {
		return item;
	}
	RequiresItem * last = list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = item;
	return list;
}

StepDeclaration * StepDeclarationSemanticAction(char * name, UseItem * uses, IdentifierList * dependsOn, NumberOption * takes, char * description) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StepDeclaration * stepDeclaration = calloc(1, sizeof(StepDeclaration));
	stepDeclaration->name = name;
	stepDeclaration->uses = uses;
	stepDeclaration->dependsOn = dependsOn;
	stepDeclaration->hasTakes = takes->present;
	stepDeclaration->minutes = takes->value;
	stepDeclaration->description = description;
	stepDeclaration->next = NULL;
	destroyNumberOption(takes);
	return stepDeclaration;
}

StepDeclaration * AppendStepDeclarationSemanticAction(StepDeclaration * list, StepDeclaration * item) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (list == NULL) {
		return item;
	}
	StepDeclaration * last = list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = item;
	return list;
}

UnitOption * NoUnitOptionSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UnitOption * unitOption = calloc(1, sizeof(UnitOption));
	unitOption->present = false;
	return unitOption;
}

UnitOption * UnitOptionSemanticAction(const UnitKind unit) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UnitOption * unitOption = calloc(1, sizeof(UnitOption));
	unitOption->present = true;
	unitOption->unit = unit;
	return unitOption;
}

UseItem * UseItemSemanticAction(const double amount, UnitOption * unit, char * ingredientName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UseItem * useItem = calloc(1, sizeof(UseItem));
	useItem->amount = amount;
	useItem->hasUnit = unit->present;
	useItem->unit = unit->unit;
	useItem->ingredientName = ingredientName;
	useItem->next = NULL;
	destroyUnitOption(unit);
	return useItem;
}

UseItem * AppendUseItemSemanticAction(UseItem * list, UseItem * item) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (list == NULL) {
		return item;
	}
	UseItem * last = list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = item;
	return list;
}

YieldsOption * NoYieldsOptionSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	YieldsOption * yieldsOption = calloc(1, sizeof(YieldsOption));
	yieldsOption->present = false;
	return yieldsOption;
}

YieldsOption * YieldsOptionSemanticAction(const double amount, const UnitKind unit) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	YieldsOption * yieldsOption = calloc(1, sizeof(YieldsOption));
	yieldsOption->present = true;
	yieldsOption->amount = amount;
	yieldsOption->unit = unit;
	return yieldsOption;
}

Statement * AppendStatementSemanticAction(Statement * list, Statement * statement) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (list == NULL) {
		return statement;
	}
	Statement * last = list;
	while (last->next != NULL) {
		last = last->next;
	}
	last->next = statement;
	return list;
}

Statement * GenerateStatementSemanticAction(char * menuName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->type = GENERATE_STATEMENT;
	statement->generate.menuName = menuName;
	statement->next = NULL;
	return statement;
}

Statement * IngredientDeclarationSemanticAction(char * name, const UnitKind unit, NumberOption * density) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->type = INGREDIENT_STATEMENT;
	statement->ingredient.name = name;
	statement->ingredient.unit = unit;
	statement->ingredient.hasDensity = density->present;
	statement->ingredient.density = density->value;
	statement->next = NULL;
	destroyNumberOption(density);
	return statement;
}

Statement * MenuDeclarationSemanticAction(char * name, IncludeItem * includes) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->type = MENU_STATEMENT;
	statement->menu.name = name;
	statement->menu.includes = includes;
	statement->next = NULL;
	return statement;
}

Statement * RecipeDeclarationSemanticAction(char * name, NumberOption * serves, YieldsOption * yields, RequiresItem * requires, StepDeclaration * steps) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->type = RECIPE_STATEMENT;
	statement->recipe.name = name;
	statement->recipe.hasServes = serves->present;
	statement->recipe.serves = serves->value;
	statement->recipe.hasYields = yields->present;
	statement->recipe.yieldsAmount = yields->amount;
	statement->recipe.yieldsUnit = yields->unit;
	statement->recipe.requires = requires;
	statement->recipe.steps = steps;
	statement->next = NULL;
	destroyNumberOption(serves);
	destroyYieldsOption(yields);
	return statement;
}

Statement * ScaleStatementSemanticAction(AssignmentOption * assignment, char * recipeName, const double toServings) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->type = SCALE_STATEMENT;
	statement->scale.hasAssignment = assignment->present;
	statement->scale.variableName = assignment->variableName;
	statement->scale.recipeName = recipeName;
	statement->scale.toServings = toServings;
	statement->next = NULL;
	free(assignment);
	return statement;
}

Statement * SubstituteDeclarationSemanticAction(char * fromIngredient, char * toIngredient, const double ratio) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->type = SUBSTITUTE_STATEMENT;
	statement->substitute.fromIngredient = fromIngredient;
	statement->substitute.toIngredient = toIngredient;
	statement->substitute.ratio = ratio;
	statement->next = NULL;
	return statement;
}

Program * ProgramSemanticAction(Statement * statements) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->statements = statements;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}
