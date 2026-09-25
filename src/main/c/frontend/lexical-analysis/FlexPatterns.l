%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus NumberLexemeAction();
extern CompilationStatus StringLexemeAction();
extern CompilationStatus UnitLexemeAction(UnitKind unit);
extern CompilationStatus UnknownLexemeAction();

%}

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
digit				[[:digit:]]+
identifier			[A-Za-z_][A-Za-z0-9_]*
number				{digit}(\.{digit})?
string				\"[^\"\n]*\"
whitespace			[[:space:]]+

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"//"[^\n]*							{ return IgnoredLexemeAction(); }

"ingredient"						{ return KeywordLexemeAction(INGREDIENT); }
"in"								{ return KeywordLexemeAction(IN); }
"density"							{ return KeywordLexemeAction(DENSITY); }
"recipe"							{ return KeywordLexemeAction(RECIPE); }
"serves"							{ return KeywordLexemeAction(SERVES); }
"yields"							{ return KeywordLexemeAction(YIELDS); }
"requires"							{ return KeywordLexemeAction(REQUIRES); }
"step"								{ return KeywordLexemeAction(STEP); }
"uses"								{ return KeywordLexemeAction(USES); }
"depends"							{ return KeywordLexemeAction(DEPENDS); }
"on"								{ return KeywordLexemeAction(ON); }
"takes"								{ return KeywordLexemeAction(TAKES); }
"minutes"							{ return KeywordLexemeAction(MINUTES); }
"substitute"						{ return KeywordLexemeAction(SUBSTITUTE); }
"with"								{ return KeywordLexemeAction(WITH); }
"ratio"								{ return KeywordLexemeAction(RATIO); }
"scale"								{ return KeywordLexemeAction(SCALE); }
"to"								{ return KeywordLexemeAction(TO); }
"servings"							{ return KeywordLexemeAction(SERVINGS); }
"menu"								{ return KeywordLexemeAction(MENU); }
"include"							{ return KeywordLexemeAction(INCLUDE); }
"scaled"							{ return KeywordLexemeAction(SCALED); }
"generate"							{ return KeywordLexemeAction(GENERATE); }
"shopping"							{ return KeywordLexemeAction(SHOPPING); }
"list"								{ return KeywordLexemeAction(LIST); }
"for"								{ return KeywordLexemeAction(FOR); }

"grams"								{ return UnitLexemeAction(UNIT_GRAMS); }
"kilograms"							{ return UnitLexemeAction(UNIT_KILOGRAMS); }
"milliliters"						{ return UnitLexemeAction(UNIT_MILLILITERS); }
"liters"							{ return UnitLexemeAction(UNIT_LITERS); }
"cups"								{ return UnitLexemeAction(UNIT_CUPS); }
"tablespoons"						{ return UnitLexemeAction(UNIT_TABLESPOONS); }
"teaspoons"							{ return UnitLexemeAction(UNIT_TEASPOONS); }
"units"								{ return UnitLexemeAction(UNIT_UNITS); }

"="									{ return KeywordLexemeAction(EQUALS); }
","									{ return KeywordLexemeAction(COMMA); }
"{"									{ return KeywordLexemeAction(OPEN_BRACE); }
"}"									{ return KeywordLexemeAction(CLOSE_BRACE); }

{number}							{ return NumberLexemeAction(); }
{string}							{ return StringLexemeAction(); }
{identifier}						{ return IdentifierLexemeAction(); }

{whitespace}						{ return IgnoredLexemeAction(); }
.									{ return UnknownLexemeAction(); }

<<EOF>>								{ return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
