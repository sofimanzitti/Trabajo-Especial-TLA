#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* PUBLIC FUNCTIONS */

void destroyAssignmentOption(AssignmentOption * assignmentOption) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (assignmentOption != NULL) {
		if (assignmentOption->variableName != NULL) {
			free(assignmentOption->variableName);
		}
		free(assignmentOption);
	}
}

void destroyIdentifierList(IdentifierList * identifierList) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (identifierList != NULL) {
		destroyIdentifierList(identifierList->next);
		if (identifierList->value != NULL) {
			free(identifierList->value);
		}
		free(identifierList);
	}
}

void destroyIncludeItem(IncludeItem * includeItem) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (includeItem != NULL) {
		destroyIncludeItem(includeItem->next);
		if (includeItem->recipeName != NULL) {
			free(includeItem->recipeName);
		}
		free(includeItem);
	}
}

void destroyNumberOption(NumberOption * numberOption) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (numberOption != NULL) {
		free(numberOption);
	}
}

void destroyRequiresItem(RequiresItem * requiresItem) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (requiresItem != NULL) {
		destroyRequiresItem(requiresItem->next);
		if (requiresItem->ingredientName != NULL) {
			free(requiresItem->ingredientName);
		}
		free(requiresItem);
	}
}

void destroyStepDeclaration(StepDeclaration * stepDeclaration) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (stepDeclaration != NULL) {
		destroyStepDeclaration(stepDeclaration->next);
		destroyUseItem(stepDeclaration->uses);
		destroyIdentifierList(stepDeclaration->dependsOn);
		if (stepDeclaration->name != NULL) {
			free(stepDeclaration->name);
		}
		if (stepDeclaration->description != NULL) {
			free(stepDeclaration->description);
		}
		free(stepDeclaration);
	}
}

void destroyUnitOption(UnitOption * unitOption) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (unitOption != NULL) {
		free(unitOption);
	}
}

void destroyUseItem(UseItem * useItem) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (useItem != NULL) {
		destroyUseItem(useItem->next);
		if (useItem->ingredientName != NULL) {
			free(useItem->ingredientName);
		}
		free(useItem);
	}
}

void destroyYieldsOption(YieldsOption * yieldsOption) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (yieldsOption != NULL) {
		free(yieldsOption);
	}
}

void destroyStatement(Statement * statement) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (statement != NULL) {
		destroyStatement(statement->next);
		switch (statement->type) {
			case GENERATE_STATEMENT:
				if (statement->generate.menuName != NULL) {
					free(statement->generate.menuName);
				}
				break;
			case INGREDIENT_STATEMENT:
				if (statement->ingredient.name != NULL) {
					free(statement->ingredient.name);
				}
				break;
			case MENU_STATEMENT:
				if (statement->menu.name != NULL) {
					free(statement->menu.name);
				}
				destroyIncludeItem(statement->menu.includes);
				break;
			case RECIPE_STATEMENT:
				if (statement->recipe.name != NULL) {
					free(statement->recipe.name);
				}
				destroyRequiresItem(statement->recipe.requires);
				destroyStepDeclaration(statement->recipe.steps);
				break;
			case SCALE_STATEMENT:
				if (statement->scale.variableName != NULL) {
					free(statement->scale.variableName);
				}
				if (statement->scale.recipeName != NULL) {
					free(statement->scale.recipeName);
				}
				break;
			case SUBSTITUTE_STATEMENT:
				if (statement->substitute.fromIngredient != NULL) {
					free(statement->substitute.fromIngredient);
				}
				if (statement->substitute.toIngredient != NULL) {
					free(statement->substitute.toIngredient);
				}
				break;
		}
		free(statement);
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		destroyStatement(program->statements);
		free(program);
	}
}
