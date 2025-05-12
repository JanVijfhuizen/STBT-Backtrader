#include "pch.h"
#include "Algorithms/GeneticAlgorithm.h"
#include <JLib/LinearSort.h>

namespace jv
{
	bool Comparer(float& a, float& b)
	{
		return a > b;
	}

	void* GeneticAlgorithm::GetTrainee()
	{
		return generation[trainId];
	}

	void GeneticAlgorithm::Rate(Arena& arena, Arena& tempArena, const float rating, Queue<bt::OutputMsg>& output)
	{
		// Finish generation and start new one if applicable.
		genRatings[trainId++] = rating;
		if (trainId >= info.length)
		{
			trainId = 0;
			++genId;

			auto tempScope = tempArena.CreateScope();

			// Copy all instances to temp arena.
			void** cpyGen = tempArena.New<void*>(info.length);
			for (uint32_t i = 0; i < info.length; i++)
				cpyGen[i] = info.copy(tempArena, generation[i], info.userPtr);

			// Reset to start.
			arena.DestroyScope(genScope);

			auto indices = tempArena.New<uint32_t>(info.length);
			jv::CreateSortableIndices(indices, info.length);
			jv::ExtLinearSort(genRatings, indices, info.length, Comparer);
			jv::ApplyExtLinearSort(tempArena, cpyGen, indices, info.length);

			// Copy new best instance to result if applicable.
			auto bestRating = genRatings[indices[0]];
			if (bestRating > this->rating)
			{
				arena.DestroyScope(resScope);
				this->rating = bestRating;
				result = info.copy(arena, cpyGen[0], info.userPtr);
				genScope = arena.CreateScope();

				if (debug)
				{
					auto str = "GAR: " + std::to_string(bestRating);
					output.Add() = bt::OutputMsg::Create(str.c_str());
				}
			}

			tempArena.DestroyScope(tempScope);

			assert(info.surviverPct > 0 && info.surviverPct <= 1);
			const uint32_t start = (float)info.length * info.surviverPct;
			const uint32_t end = info.length - (float)info.length * info.arrivalsPct;
			assert(end < info.length && end > start);

			// Copy all successful instances.
			for (uint32_t i = 0; i < start; i++)
				generation[i] = info.copy(arena, cpyGen[i], info.userPtr);

			// Breed successfull instances.
			for (uint32_t i = start; i < end; i++)
			{
				uint32_t a = rand() % start;
				uint32_t b = rand() % end;
				generation[i] = info.breed(arena, cpyGen[a], cpyGen[b], info.userPtr);
			}
			// Create new instances.
			for (uint32_t i = end; i < info.length; i++)
				info.create(arena, info.userPtr);
		}
	}

	GeneticAlgorithm GeneticAlgorithm::Create(Arena& arena, GeneticAlgorithmCreateInfo& info)
	{
		GeneticAlgorithm ga{};
		ga.info = info;
		ga.scope = arena.CreateScope();

		ga.generation = arena.New<void*>(info.length);
		ga.genRatings = arena.New<float>(info.length);

		ga.resScope = arena.CreateScope();
		ga.genScope = arena.CreateScope();
		
		for (uint32_t i = 0; i < info.length; i++)
			ga.generation[i] = info.create(arena, info.userPtr);
		ga.result = info.create(arena, info.userPtr);

		ga.rating = FLT_MIN;
		ga.genId = 0;
		ga.trainId = 0;
		return ga;
	}

	void GeneticAlgorithm::Destroy(Arena& arena, GeneticAlgorithm& ga)
	{
		arena.DestroyScope(ga.scope);
	}
}