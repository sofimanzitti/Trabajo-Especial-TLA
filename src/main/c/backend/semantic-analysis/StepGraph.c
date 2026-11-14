#include "StepGraph.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static size_t _countSteps(const RecipeDeclaration * recipe) {
	size_t count = 0;
	for (const StepDeclaration * step = recipe->steps; step != NULL; step = step->next) {
		++count;
	}
	return count;
}

static StepDeclaration ** _collectSteps(const RecipeDeclaration * recipe, const size_t count) {
	StepDeclaration ** steps = (StepDeclaration **) calloc(count == 0 ? 1 : count, sizeof(StepDeclaration *));
	size_t k = 0;
	for (StepDeclaration * step = recipe->steps; step != NULL; step = step->next) {
		steps[k++] = step;
	}
	return steps;
}

static bool _dependsOn(const StepDeclaration * step, const char * name) {
	for (const IdentifierList * dependency = step->dependsOn; dependency != NULL; dependency = dependency->next) {
		if (strcmp(dependency->value, name) == 0) {
			return true;
		}
	}
	return false;
}

size_t sortSteps(const RecipeDeclaration * recipe, size_t * total, StepDeclaration *** ordered) {
	const size_t count = _countSteps(recipe);
	StepDeclaration ** steps = _collectSteps(recipe, count);
	StepDeclaration ** result = (StepDeclaration **) calloc(count == 0 ? 1 : count, sizeof(StepDeclaration *));
	bool * placed = (bool *) calloc(count == 0 ? 1 : count, sizeof(bool));
	size_t placedCount = 0;
	bool progress = true;
	while (progress) {
		progress = false;
		for (size_t k = 0; k < count; ++k) {
			if (placed[k]) {
				continue;
			}
			bool ready = true;
			for (size_t j = 0; j < count && ready; ++j) {
				if (j != k && !placed[j] && _dependsOn(steps[k], steps[j]->name)) {
					ready = false;
				}
			}
			if (ready) {
				placed[k] = true;
				result[placedCount++] = steps[k];
				progress = true;
				break; // restart from the first pending step to keep declaration order
			}
		}
	}
	free(placed);
	free(steps);
	*total = count;
	*ordered = result;
	return placedCount;
}

double criticalPathMinutes(const RecipeDeclaration * recipe) {
	size_t total = 0;
	StepDeclaration ** ordered = NULL;
	const size_t placed = sortSteps(recipe, &total, &ordered);
	double * finish = (double *) calloc(placed == 0 ? 1 : placed, sizeof(double));
	double longest = 0;
	for (size_t k = 0; k < placed; ++k) {
		double start = 0;
		for (size_t j = 0; j < k; ++j) {
			if (_dependsOn(ordered[k], ordered[j]->name) && finish[j] > start) {
				start = finish[j];
			}
		}
		finish[k] = start + (ordered[k]->hasTakes ? ordered[k]->minutes : 0);
		if (finish[k] > longest) {
			longest = finish[k];
		}
	}
	free(finish);
	free(ordered);
	return longest;
}
