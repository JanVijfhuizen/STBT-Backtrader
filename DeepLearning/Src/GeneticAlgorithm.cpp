#include "pch.h"
#include "Algorithms/GeneticAlgorithm.h"

namespace jv
{
	void GeneticAlgorithm::RandInit()
	{
		const uint32_t l = width * length;
		for (uint32_t i = 0; i < l; i++)
			generation[i] = RandF(-1, 1);
		for (uint32_t i = 0; i < length; i++)
			result[i] = RandF(-1, 1);
	}

	GeneticAlgorithm GeneticAlgorithm::Create(Arena& arena, GeneticAlgorithmCreateInfo& info)
	{
		GeneticAlgorithm ga{};
		ga.length = info.length;
		ga.width = info.width;
		ga.scope = arena.CreateScope();
		ga.generation = arena.New<float>(info.width * info.length);
		ga.result = arena.New<float>(info.width);
		ga.rating = FLT_MAX;
		ga.genId = 0;
		return ga;
	}

	void GeneticAlgorithm::Destroy(Arena& arena, GeneticAlgorithm& ga)
	{
		arena.DestroyScope(ga.scope);
	}
}