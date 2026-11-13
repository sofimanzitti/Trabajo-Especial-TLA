#ifndef SEMANTIC_ANALYSIS_HEADER
#define SEMANTIC_ANALYSIS_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/ModuleDestructor.h"
#include "SymbolTable.h"

/** Initialize module's internal state. */
ModuleDestructor initializeSemanticAnalysisModule();

/**
 * Validates a syntactically correct program against the rules of the domain
 * and fills the symbol table with its declarations. Every error found is
 * reported (with its line); the status is FAILED if there was at least one.
 */
CompilationStatus executeSemanticAnalysis(const Program * program, SymbolTable * table);

#endif
