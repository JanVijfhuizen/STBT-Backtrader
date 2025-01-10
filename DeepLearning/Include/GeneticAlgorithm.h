#pragma once
#include "JLib/Arena.h"
#include "NNet.h"

namespace jv::ai 
{
	struct GeneticAlgorithmRunInfo final
	{
		NNetCreateInfo nnetCreateInfo;
		uint32_t width = 1000;
		uint32_t length = 1000;
		size_t initMemSize = 262144;
		float (*ratingFunc)(NNet& nnet, void* userPtr, Arena& arena, Arena& tempArena);
		void* userPtr;
	};

	[[nodiscard]] NNet* RunGeneticAlgorithm(GeneticAlgorithmRunInfo& info, Arena& arena, Arena& tempArena);
}