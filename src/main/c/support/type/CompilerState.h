#ifndef COMPILER_STATE_HEADER
#define COMPILER_STATE_HEADER

#include <stdbool.h>

/**
 * The global state of the compiler. Should transport every data structure
 * needed across the different phases of a compilation.
 */
typedef struct {
	/**
	 * The root node of the AST.
	 */
	void * abstractSyntaxtTree;

	/**
	 * True if the lexical-analyzer already reported an error, so the
	 * syntactic-analyzer doesn't report a second (and misleading) one.
	 */
	bool hasLexicalError;
} CompilerState;

#endif
