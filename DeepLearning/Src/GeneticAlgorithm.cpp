#include "pch.h"
#include "Algorithms/GeneticAlgorithm.h"
#include <JLib/LinearSort.h>

namespace jv
{
	bool Comparer(float& a, float& b)
	{
		return a > b;
	}

	void MakeNew(float* t, const uint32_t length)
	{
		for (uint32_t i = 0; i < length; i++)
			t[i] = RandF(-1, 1);
	}

	void GeneticAlgorithm::RandInit()
	{
		for (uint32_t i = 0; i < length; i++)
			MakeNew(generation[i], width);
		MakeNew(result, width);
	}

	float* GeneticAlgorithm::GetTrainee()
	{
		return generation[trainId];
	}

	void Mutate(const GeneticAlgorithm& ga, float* t)
	{
		for (uint32_t i = 0; i < ga.width; i++)
		{
			if (RandF(0, 1) > ga.mutateChance)
				continue;

			float& f = t[i];

			const uint32_t type = rand() % 3;
			switch (type)
			{
			// Add/Sub
			case 0:
				f += RandF(-ga.mutateAddition, ga.mutateAddition);
				break;
			// Mul/Div
			case 1:
				f *= 1.f + RandF(-ga.mutateMultiplier, ga.mutateMultiplier);
				break;
			// New
			case 2:
				f = RandF(-1, 1);
				break;
			}
		}
	}

	void Breed(const GeneticAlgorithm& ga, float* a, float* b, float* c)
	{
		for (uint32_t i = 0; i < ga.length; i++)
		{
			auto& f = c[i];
			f = rand() % 2 == 0 ? a[i] : b[i];
		}
		Mutate(ga, c);
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

			assert(surviverPct > 0 && surviverPct <= 1);
			const uint32_t start = (float)length * surviverPct;
			const uint32_t end = length - (float)length * arrivalsPct;
			assert(end < length && end > start);

			// Breed successfull instances.
			for (uint32_t i = start; i < end; i++)
			{
				uint32_t a = rand() % start;
				uint32_t b = rand() % end;
				Breed(*this, generation[a], generation[b], generation[i]);
			}
			// Create new instances.
			for (uint32_t i = end; i < length; i++)
				MakeNew(generation[i], length);
		}
	}

	GeneticAlgorithm GeneticAlgorithm::Create(Arena& arena, GeneticAlgorithmCreateInfo& info)
	{
		GeneticAlgorithm ga{};
		ga.length = info.length;
		ga.width = info.width;
		ga.mutateChance = info.mutateChance;
		ga.mutateAddition = info.mutateAddition;
		ga.mutateMultiplier = info.mutateMultiplier;
		ga.surviverPct = info.surviverPct;
		ga.arrivalsPct = info.arrivalsPct;
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