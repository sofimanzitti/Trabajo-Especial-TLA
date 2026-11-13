#ifndef STEP_GRAPH_HEADER
#define STEP_GRAPH_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include <stddef.h>

/**
 * Orders the steps of a recipe so every step comes after the ones it depends
 * on (ties are broken by declaration order). Dependencies on unknown steps are
 * ignored. Returns the number of steps placed in "ordered", which is less than
 * the total when there is a dependency cycle (the remaining steps are those
 * that are part of, or depend on, a cycle). The caller must free "ordered".
 */
size_t sortSteps(const RecipeDeclaration * recipe, size_t * total, StepDeclaration *** ordered);

/**
 * Length in minutes of the longest chain of dependent steps, i.e. the minimum
 * time to finish the recipe if independent steps run in parallel.
 */
double criticalPathMinutes(const RecipeDeclaration * recipe);

#endif
