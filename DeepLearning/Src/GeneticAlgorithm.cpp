#include "pch.h"
#include "Algorithms/GeneticAlgorithm.h"
#include <JLib/LinearSort.h>

namespace jv
{
	bool Comparer(float& a, float& b)
	{
		return a > b;
	}

	void GeneticAlgorithm::RandInit()
	{
		for (uint32_t i = 0; i < length; i++)
			for (uint32_t j = 0; j < width; j++)
				generation[i][j] = RandF(-1, 1);
		for (uint32_t i = 0; i < length; i++)
			result[i] = RandF(-1, 1);
	}

	float* GeneticAlgorithm::GetTrainee()
	{
		return generation[trainId];
	}

	void GeneticAlgorithm::Rate(Arena& tempArena, const float rating)
	{
		// Finish generation and start new one if applicable.
		genRatings[trainId++] = rating;
		if (trainId >= length)
		{
			trainId = 0;
			++genId;

			auto tempScope = tempArena.CreateScope();

			auto indices = tempArena.New<uint32_t>(length);
			jv::CreateSortableIndices(indices, length);
			jv::ExtLinearSort(genRatings, indices, length, Comparer);
			jv::ApplyExtLinearSort(tempArena, generation, indices, length);

			// Copy new best instance to result if applicable.
			auto bestRating = genRatings[indices[0]];
			if (bestRating > this->rating)
			{
				this->rating = bestRating;
				memcpy(result, generation[indices[0]], sizeof(float) * width);
			}

			std::cout << genRatings[indices[0]] << std::endl;
			tempArena.DestroyScope(tempScope);
		}
	}

	GeneticAlgorithm GeneticAlgorithm::Create(Arena& arena, GeneticAlgorithmCreateInfo& info)
	{
		GeneticAlgorithm ga{};
		ga.length = info.length;
		ga.width = info.width;
		ga.scope = arena.CreateScope();
		ga.generation = arena.New<float*>(info.length);
		for (uint32_t i = 0; i < info.length; i++)
			ga.generation[i] = arena.New<float>(info.width);
		ga.result = arena.New<float>(info.width);
		ga.genRatings = arena.New<float>(info.length);
		ga.rating = FLT_MAX;
		ga.genId = 0;
		ga.trainId = 0;
		return ga;
	}

	void GeneticAlgorithm::Destroy(Arena& arena, GeneticAlgorithm& ga)
	{
		arena.DestroyScope(ga.scope);
	}
}