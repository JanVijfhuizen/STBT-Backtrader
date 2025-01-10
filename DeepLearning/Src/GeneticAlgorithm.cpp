#include "pch.h"
#include "GeneticAlgorithm.h"

namespace jv::ai
{
	void* Alloc(const uint32_t size)
	{
		return malloc(size);
	}
	void Free(void* ptr)
	{
		return free(ptr);
	}

	NNet* RunGeneticAlgorithm(GeneticAlgorithmRunInfo& info, Arena& arena, Arena& tempArena)
	{
		const auto tempScope = tempArena.CreateScope();
		float* ratings = tempArena.New<float>(info.width);
		NNet* bestNNet = nullptr;
		float bestNNetRating = -1;

		Arena arenas[2];

		{
			// Allocate memory from temp arena with predetermined minimal size.
			ArenaCreateInfo createInfo{};
			createInfo.alloc = Alloc;
			createInfo.free = Free;
			createInfo.memory = tempArena.Alloc(info.initMemSize / 2);
			createInfo.memorySize = info.initMemSize / 2;
			arenas[0] = Arena::Create(createInfo);
			createInfo.memory = tempArena.Alloc(info.initMemSize / 2);
			arenas[1] = Arena::Create(createInfo);
		}

		NNet* generations[2];
		for (uint32_t i = 0; i < 2; i++)
			generations[i] = tempArena.New<NNet>(info.width);

		// Set up first generation of random instances.
		for (uint32_t i = 0; i < info.width; i++)
		{
			NNet& nnet = generations[0][i];
			nnet = CreateNNet(info.nnetCreateInfo, arenas[0]);
		}

		for (uint32_t i = 0; i < info.length; i++)
		{

		}

		Arena::Destroy(arenas[1]);
		Arena::Destroy(arenas[0]);
		tempArena.DestroyScope(tempScope);
		return nullptr;
	}
}