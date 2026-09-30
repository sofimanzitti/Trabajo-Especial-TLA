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
extern CompilationStatus FractionLexemeAction();
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus NumberLexemeAction();
extern CompilationStatus StringLexemeAction();
extern CompilationStatus UnitLexemeAction(UnitKind unit);
extern CompilationStatus UnknownLexemeAction();
extern CompilationStatus UnterminatedStringLexemeAction();

%}

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
digit				[[:digit:]]+
utf8_character		[\xC2-\xDF][\x80-\xBF]|[\xE0-\xEF][\x80-\xBF]{2}|[\xF0-\xF4][\x80-\xBF]{3}
letter				[A-Za-z_]|{utf8_character}
identifier			({letter})({letter}|[0-9])*
number				{digit}(\.{digit})?
fraction			{digit}"/"{digit}
mixed_number		{digit}[ \t]+{fraction}
string				\"[^\"\n]*\"
unterminated_string	\"[^\"\n]*
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
"minute"							{ return KeywordLexemeAction(MINUTES); }
"minutes"							{ return KeywordLexemeAction(MINUTES); }
"hour"								{ return KeywordLexemeAction(HOURS); }
"hours"								{ return KeywordLexemeAction(HOURS); }
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

"gram"								{ return UnitLexemeAction(UNIT_GRAMS); }
"grams"								{ return UnitLexemeAction(UNIT_GRAMS); }
"kilogram"							{ return UnitLexemeAction(UNIT_KILOGRAMS); }
"kilograms"							{ return UnitLexemeAction(UNIT_KILOGRAMS); }
"milliliter"						{ return UnitLexemeAction(UNIT_MILLILITERS); }
"milliliters"						{ return UnitLexemeAction(UNIT_MILLILITERS); }
"liter"								{ return UnitLexemeAction(UNIT_LITERS); }
"liters"							{ return UnitLexemeAction(UNIT_LITERS); }
"pinch"								{ return UnitLexemeAction(UNIT_PINCHES); }
"pinches"							{ return UnitLexemeAction(UNIT_PINCHES); }
"cup"								{ return UnitLexemeAction(UNIT_CUPS); }
"cups"								{ return UnitLexemeAction(UNIT_CUPS); }
"tablespoon"						{ return UnitLexemeAction(UNIT_TABLESPOONS); }
"tablespoons"						{ return UnitLexemeAction(UNIT_TABLESPOONS); }
"teaspoon"							{ return UnitLexemeAction(UNIT_TEASPOONS); }
"teaspoons"							{ return UnitLexemeAction(UNIT_TEASPOONS); }
"unit"								{ return UnitLexemeAction(UNIT_UNITS); }
"units"								{ return UnitLexemeAction(UNIT_UNITS); }

","									{ return KeywordLexemeAction(COMMA); }
"{"									{ return KeywordLexemeAction(OPEN_BRACE); }
"}"									{ return KeywordLexemeAction(CLOSE_BRACE); }

{number}							{ return NumberLexemeAction(); }
{fraction}							{ return FractionLexemeAction(); }
{mixed_number}						{ return FractionLexemeAction(); }
{string}							{ return StringLexemeAction(); }
{unterminated_string}				{ return UnterminatedStringLexemeAction(); }
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
