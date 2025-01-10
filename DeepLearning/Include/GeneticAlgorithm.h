#pragma once
#include "JLib/Arena.h"
#include "NNet.h"

namespace jv::ai 
{
	struct GeneticAlgorithmRunInfo final
	{
		NNetCreateInfo nnetCreateInfo;
		uint32_t width = 1000;
		uint32_t epochs = 1000;
		uint32_t survivors = 100;
		uint32_t arrivals = 100;
		size_t initMemSize = 262144;
		float (*ratingFunc)(NNet& nnet, void* userPtr, Arena& arena, Arena& tempArena);
		void* userPtr;
	};

	[[nodiscard]] NNet RunGeneticAlgorithm(GeneticAlgorithmRunInfo& info, Arena& arena, Arena& tempArena);
}