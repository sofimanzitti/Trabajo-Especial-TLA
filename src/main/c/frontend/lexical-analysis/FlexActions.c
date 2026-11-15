#include "FlexActions.h"
#include <math.h>

/* MODULE INTERNAL STATE */

static bool _logIgnoredLexemes = true;
static CompilerState * _compilerState = NULL;
static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownFlexActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: FlexActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
	_lexicalAnalyzer = NULL;
}

ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer, CompilerState * compilerState) {
	_compilerState = compilerState;
	_lexicalAnalyzer = lexicalAnalyzer;
	_logger = createLogger("FlexActions");
	_logIgnoredLexemes = getBooleanOrDefault("LOG_IGNORED_LEXEMES", _logIgnoredLexemes);
	return _shutdownFlexActionsModule;
}

/* PRIVATE FUNCTIONS */

static void _logTokenAction(const char * actionName, Token * token);
static CompilationStatus _rejectToken(Token * token, const char * reason);

/**
 * Logs a lexical-analyzer action over a token in DEBUGGING level.
 */
static void _logTokenAction(const char * actionName, Token * token) {
	char * _lexeme = escape(token->lexeme);
	logDebugging(_logger, WARNING_COLOR "%s" DEFAULT_COLOR ": Token(context=%d, label=%d, length=%d, lexeme=%s\"%s\"%s, line=%d, semanticValue=%p)",
		actionName,
		token->context,
		token->label,
		token->length,
		INFORMATION_COLOR, _lexeme, DEFAULT_COLOR,
		token->line,
		token->semanticValue);
	free(_lexeme);
	_lexeme = NULL;
}

/**
 * Reports a lexical error with its line, and pushes an EOF so the parser can
 * release its stack. The token is destroyed.
 */
static CompilationStatus _rejectToken(Token * token, const char * reason) {
	char * lexeme = escape(token->lexeme);
	logError(_logger, "Line %d: %s \"%s\".", token->line, reason, lexeme);
	free(lexeme);
	destroyToken(token);
	_compilerState->hasLexicalError = true;
	Token * eof = createToken(_lexicalAnalyzer, 0);
	pushToken(_lexicalAnalyzer, eof);
	destroyToken(eof);
	return FAILED;
}

/** Largest accepted numeric literal; anything above would overflow later computations. */
#define MAX_NUMBER 1e9

/** Longest accepted identifier; names are used to build output file names. */
#define MAX_IDENTIFIER_LENGTH 100

/* PUBLIC FUNCTIONS */

CompilationStatus EOFLexemeAction() {
	CompilationStatus status = IN_PROGRESS;
	Token * token = createToken(_lexicalAnalyzer, 0);
	_logTokenAction(__FUNCTION__, token);
	status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

/**
 * Accepts a fraction ("1/2") or a mixed number ("1 1/2"), and pushes its
 * value as a NUMBER token. A zero denominator rejects the input program.
 */
CompilationStatus FractionLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, NUMBER);
	double whole = 0;
	double numerator = 0;
	double denominator = 0;
	if (strchr(token->lexeme, ' ') != NULL || strchr(token->lexeme, '\t') != NULL) {
		sscanf(token->lexeme, "%lf %lf/%lf", &whole, &numerator, &denominator);
	}
	else {
		sscanf(token->lexeme, "%lf/%lf", &numerator, &denominator);
	}
	_logTokenAction(__FUNCTION__, token);
	if (denominator == 0) {
		return _rejectToken(token, "fraction with zero denominator");
	}
	token->semanticValue->number = whole + numerator / denominator;
	if (!isfinite(token->semanticValue->number) || token->semanticValue->number > MAX_NUMBER) {
		return _rejectToken(token, "number too large");
	}
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus IdentifierLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, IDENT);
	// Length in characters: UTF-8 continuation bytes (10xxxxxx) do not count.
	unsigned int characters = 0;
	for (const char * c = token->lexeme; *c != '\0'; ++c) {
		characters += (((unsigned char) *c) & 0xC0) != 0x80;
	}
	if (characters > MAX_IDENTIFIER_LENGTH) {
		return _rejectToken(token, "identifier too long");
	}
	token->semanticValue->string = strdup(token->lexeme);
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus IgnoredLexemeAction() {
	if (_logIgnoredLexemes) {
		Token * token = createToken(_lexicalAnalyzer, IGNORED);
		_logTokenAction(__FUNCTION__, token);
		destroyToken(token);
	}
	return IN_PROGRESS;
}

CompilationStatus KeywordLexemeAction(TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

/** A number glued to letters ("300Harina"): a separator is missing. */
CompilationStatus MalformedNumberLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	return _rejectToken(token, "number immediately followed by letters (a space is missing)");
}

CompilationStatus NumberLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, NUMBER);
	token->semanticValue->number = atof(token->lexeme);
	if (!isfinite(token->semanticValue->number) || token->semanticValue->number > MAX_NUMBER) {
		return _rejectToken(token, "number too large");
	}
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus StringLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, STRING);
	const unsigned int contentLength = token->length - 2;
	char * content = (char *) calloc(contentLength + 1, sizeof(char));
	strncpy(content, token->lexeme + 1, contentLength);
	token->semanticValue->string = content;
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus UnitLexemeAction(UnitKind unit) {
	Token * token = createToken(_lexicalAnalyzer, UNIT);
	token->semanticValue->unit = unit;
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus UnknownLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	return _rejectToken(token, "unknown character");
}

CompilationStatus UnterminatedStringLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	return _rejectToken(token, "unterminated description (missing closing quote)");
}
