%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison parser.
 *
 * @todo Add location to the grammar and "pushToken" API function.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Tracking-Locations.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */

	char * string;
	double number;
	UnitKind unit;
	TokenLabel token;

	/** Non-terminals. */

	AssignmentOption * assignmentOption;
	IdentifierList * identifierList;
	IncludeItem * includeItem;
	NumberOption * numberOption;
	Program * program;
	RequiresItem * requiresItem;
	Statement * statement;
	StepDeclaration * step;
	UnitOption * unitOption;
	UseItem * useItem;
	YieldsOption * yieldsOption;
}

/**
 * Destructors. This functions are executed after the parsing ends, so if the
 * AST must be used in the following phases of the compiler you shouldn't used
 * this approach for the AST root node ("program" non-terminal, in this
 * grammar), or it will drop the entire tree even if the parsing succeeds.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { destroyAssignmentOption($$); } <assignmentOption>
%destructor { destroyIdentifierList($$); } <identifierList>
%destructor { destroyIncludeItem($$); } <includeItem>
%destructor { destroyNumberOption($$); } <numberOption>
%destructor { destroyRequiresItem($$); } <requiresItem>
%destructor { destroyStatement($$); } <statement>
%destructor { destroyStepDeclaration($$); } <step>
%destructor { destroyUnitOption($$); } <unitOption>
%destructor { destroyUseItem($$); } <useItem>
%destructor { destroyYieldsOption($$); } <yieldsOption>
%destructor { free($$); } <string>

/** Terminals. */
%token <string> IDENT
%token <string> STRING
%token <number> NUMBER
%token <unit> UNIT

%token <token> COMMA
%token <token> DENSITY
%token <token> DEPENDS
%token <token> EQUALS
%token <token> FOR
%token <token> GENERATE
%token <token> IN
%token <token> INCLUDE
%token <token> INGREDIENT
%token <token> LIST
%token <token> MENU
%token <token> MINUTES
%token <token> ON
%token <token> RATIO
%token <token> RECIPE
%token <token> REQUIRES
%token <token> SCALE
%token <token> SCALED
%token <token> SERVES
%token <token> SERVINGS
%token <token> SHOPPING
%token <token> STEP
%token <token> SUBSTITUTE
%token <token> TAKES
%token <token> TO
%token <token> USES
%token <token> WITH
%token <token> YIELDS

%token <token> CLOSE_BRACE
%token <token> OPEN_BRACE

%token <token> IGNORED
%token <token> UNKNOWN

/** Non-terminals. */
%type <assignmentOption> assignment_option
%type <identifierList> depends_option identifier_list
%type <includeItem> include_item include_item_list
%type <numberOption> density_option serves_option takes_option
%type <program> program
%type <requiresItem> requires_item requires_item_list
%type <statement> generate_statement ingredient_declaration menu_declaration recipe_declaration scale_statement statement statement_list substitute_declaration
%type <step> step step_list
%type <unitOption> unit_option
%type <useItem> uses_item_list uses_option
%type <yieldsOption> yields_option

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

program: statement_list											{ $$ = ProgramSemanticAction($1); }
	;

statement_list: %empty											{ $$ = NULL; }
	| statement_list statement									{ $$ = AppendStatementSemanticAction($1, $2); }
	;

statement: ingredient_declaration								{ $$ = $1; }
	| recipe_declaration										{ $$ = $1; }
	| substitute_declaration									{ $$ = $1; }
	| scale_statement											{ $$ = $1; }
	| menu_declaration											{ $$ = $1; }
	| generate_statement										{ $$ = $1; }
	;

ingredient_declaration: INGREDIENT IDENT IN UNIT density_option	{ $$ = IngredientDeclarationSemanticAction($2, $4, $5); }
	;

density_option: %empty											{ $$ = NoNumberOptionSemanticAction(); }
	| DENSITY NUMBER											{ $$ = NumberOptionSemanticAction($2); }
	;

recipe_declaration: RECIPE IDENT serves_option yields_option OPEN_BRACE requires_item_list step_list CLOSE_BRACE
																{ $$ = RecipeDeclarationSemanticAction($2, $3, $4, $6, $7); }
	;

serves_option: %empty											{ $$ = NoNumberOptionSemanticAction(); }
	| SERVES NUMBER												{ $$ = NumberOptionSemanticAction($2); }
	;

yields_option: %empty											{ $$ = NoYieldsOptionSemanticAction(); }
	| YIELDS NUMBER UNIT										{ $$ = YieldsOptionSemanticAction($2, $3); }
	;

requires_item_list: %empty										{ $$ = NULL; }
	| requires_item_list requires_item							{ $$ = AppendRequiresItemSemanticAction($1, $2); }
	;

requires_item: REQUIRES NUMBER unit_option IDENT				{ $$ = RequiresItemSemanticAction($2, $3, $4); }
	;

unit_option: %empty											{ $$ = NoUnitOptionSemanticAction(); }
	| UNIT														{ $$ = UnitOptionSemanticAction($1); }
	;

step_list: %empty												{ $$ = NULL; }
	| step_list step											{ $$ = AppendStepDeclarationSemanticAction($1, $2); }
	;

step: STEP IDENT uses_option depends_option takes_option OPEN_BRACE STRING CLOSE_BRACE
																{ $$ = StepDeclarationSemanticAction($2, $3, $4, $5, $7); }
	;

uses_option: %empty											{ $$ = NULL; }
	| USES uses_item_list										{ $$ = $2; }
	;

uses_item_list: NUMBER unit_option IDENT						{ $$ = UseItemSemanticAction($1, $2, $3); }
	| uses_item_list COMMA NUMBER unit_option IDENT			{ $$ = AppendUseItemSemanticAction($1, UseItemSemanticAction($3, $4, $5)); }
	;

depends_option: %empty											{ $$ = NULL; }
	| DEPENDS ON identifier_list								{ $$ = $3; }
	;

identifier_list: IDENT											{ $$ = IdentifierListSemanticAction($1); }
	| identifier_list COMMA IDENT								{ $$ = AppendIdentifierSemanticAction($1, $3); }
	;

takes_option: %empty											{ $$ = NoNumberOptionSemanticAction(); }
	| TAKES NUMBER MINUTES										{ $$ = NumberOptionSemanticAction($2); }
	;

substitute_declaration: SUBSTITUTE IDENT WITH IDENT RATIO NUMBER
																{ $$ = SubstituteDeclarationSemanticAction($2, $4, $6); }
	;

scale_statement: assignment_option SCALE IDENT TO NUMBER SERVINGS
																{ $$ = ScaleStatementSemanticAction($1, $3, $5); }
	;

assignment_option: %empty										{ $$ = NoAssignmentOptionSemanticAction(); }
	| IDENT EQUALS												{ $$ = AssignmentOptionSemanticAction($1); }
	;

menu_declaration: MENU IDENT OPEN_BRACE include_item_list CLOSE_BRACE
																{ $$ = MenuDeclarationSemanticAction($2, $4); }
	;

include_item_list: %empty										{ $$ = NULL; }
	| include_item_list include_item							{ $$ = AppendIncludeItemSemanticAction($1, $2); }
	;

include_item: INCLUDE IDENT SCALED TO NUMBER SERVINGS			{ $$ = IncludeItemSemanticAction($2, $5); }
	;

generate_statement: GENERATE SHOPPING LIST FOR IDENT			{ $$ = GenerateStatementSemanticAction($5); }
	;

%%
