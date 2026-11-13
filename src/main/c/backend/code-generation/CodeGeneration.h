#ifndef CODE_GENERATION_HEADER
#define CODE_GENERATION_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/ModuleDestructor.h"
#include "../semantic-analysis/SymbolTable.h"

/** Initialize module's internal state. */
ModuleDestructor initializeCodeGenerationModule();

/**
 * Produces the artifacts of a semantically valid program inside the output
 * directory (created if it doesn't exist):
 *
 *  - "scale <Recipe> to <N> servings"    => <Recipe>-<N>-servings.html
 *  - "generate shopping list for <Menu>" => <Menu>-shopping-list.csv and <Menu>-recipes.html
 */
CompilationStatus executeCodeGeneration(const Program * program, const SymbolTable * table, const char * outputDirectory);

#endif
