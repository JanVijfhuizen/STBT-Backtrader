#pragma once
#include "JLib/Arena.h"
#include "NNet.h"
#include "NNetUtils.h"

namespace jv::ai 
{
	struct GeneticAlgorithmRunInfo final
	{
		uint32_t inputSize, outputSize;
		// Amount of instances in a generation.
		uint32_t width = 1000;
		// Amount of generational cycles.
		uint32_t epochs = 1000;
		// Remaining nnets that pass unchanged to the next generation.
		uint32_t survivors = 100;
		// New instances added to each new generation.
		uint32_t arrivals = 100;
		// Every new instance will mutate x times to get more random initial starts.
		uint32_t arrivalMutationCount = 0;
		Mutations mutations = {};
		// Stagnate after x epochs without improvements. Will swap to a mode where values are changed in a diminishing way
		// until success if found again.
		uint32_t stagnateAfter = 100;
		// Mutation chances are multiplied by this every unsuccesfull epoch. Resets on success.
		float stagnationMul = .99f;
		float stagnationMaxPctChange = .1f;
		// Amount of times the nnet result is checked extra if it's a new best result.
		uint32_t validationCheckAmount = 10;
		// Memory reserved for the algorithm. 
		// Will increase dynamically if there is no space, but will obviously fragment if that happens.
		size_t initMemSize = 33554432;
		float (*ratingFunc)(NNet& nnet, void* userPtr, Arena& arena, Arena& tempArena);
		// Will stop the algorithm if the target score is met.
		float targetScore = -1;
		void* userPtr;
		// Debug progress in command prompt.
		bool debug = true; 
	};

	[[nodiscard]] NNet RunGeneticAlgorithm(GeneticAlgorithmRunInfo& info, Arena& arena, Arena& tempArena);
}