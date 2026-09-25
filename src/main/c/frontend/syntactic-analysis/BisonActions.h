#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdlib.h>
#include <string.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule();

/**
 * Bison semantic actions.
 */

AssignmentOption * AssignmentOptionSemanticAction(char * variableName);
AssignmentOption * NoAssignmentOptionSemanticAction();

IdentifierList * AppendIdentifierSemanticAction(IdentifierList * list, char * identifier);
IdentifierList * IdentifierListSemanticAction(char * identifier);

IncludeItem * AppendIncludeItemSemanticAction(IncludeItem * list, IncludeItem * item);
IncludeItem * IncludeItemSemanticAction(char * recipeName, const double toServings);

NumberOption * NoNumberOptionSemanticAction();
NumberOption * NumberOptionSemanticAction(const double value);

RequiresItem * AppendRequiresItemSemanticAction(RequiresItem * list, RequiresItem * item);
RequiresItem * RequiresItemSemanticAction(const double amount, UnitOption * unit, char * ingredientName);

StepDeclaration * AppendStepDeclarationSemanticAction(StepDeclaration * list, StepDeclaration * item);
StepDeclaration * StepDeclarationSemanticAction(char * name, UseItem * uses, IdentifierList * dependsOn, NumberOption * takes, char * description);

UnitOption * NoUnitOptionSemanticAction();
UnitOption * UnitOptionSemanticAction(const UnitKind unit);

UseItem * AppendUseItemSemanticAction(UseItem * list, UseItem * item);
UseItem * UseItemSemanticAction(const double amount, UnitOption * unit, char * ingredientName);

YieldsOption * NoYieldsOptionSemanticAction();
YieldsOption * YieldsOptionSemanticAction(const double amount, const UnitKind unit);

Statement * AppendStatementSemanticAction(Statement * list, Statement * statement);
Statement * GenerateStatementSemanticAction(char * menuName);
Statement * IngredientDeclarationSemanticAction(char * name, const UnitKind unit, NumberOption * density);
Statement * MenuDeclarationSemanticAction(char * name, IncludeItem * includes);
Statement * RecipeDeclarationSemanticAction(char * name, NumberOption * serves, YieldsOption * yields, RequiresItem * requires, StepDeclaration * steps);
Statement * ScaleStatementSemanticAction(AssignmentOption * assignment, char * recipeName, const double toServings);
Statement * SubstituteDeclarationSemanticAction(char * fromIngredient, char * toIngredient, const double ratio);

Program * ProgramSemanticAction(Statement * statements);

#endif
