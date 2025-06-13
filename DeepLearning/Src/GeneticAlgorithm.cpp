#include "pch.h"
#include "Algorithms/GeneticAlgorithm.h"
#include <JLib/LinearSort.h>
#include <Algorithms/KMeans.h>

namespace jv
{
	float* GACreate(Arena& arena, const uint32_t width)
	{
		// Adding dominance variable.
		const uint32_t w = width * 2;
		auto arr = arena.New<float>(w);
		for (uint32_t i = 0; i < width; i++)
			arr[i] = RandF(-1, 1);
		for (uint32_t i = 0; i < width; i++)
			arr[width + i] = RandF(0, 1);
		return arr;
	}

	float* GACopy(Arena& arena, float* instance, const uint32_t width)
	{
		auto arr = arena.New<float>(width * 2);

		for (uint32_t i = 0; i < width * 2; i++)
			arr[i] = instance[i];

		return arr;
	}

	void GAMutate(Arena& arena, float* instance, const GeneticAlgorithm& ga)
	{
		const auto& info = ga.info;

		for (uint32_t i = 0; i < info.width; i++)
		{
			if (RandF(0, 1) > info.mutateChance)
				continue;

			float& f = instance[i];

			const uint32_t type = rand() % 3;
			switch (type)
			{
				// Add/Sub
			case 0:
				f += RandF(-info.mutateAddition, info.mutateAddition);
				break;
				// Mul/Div
			case 1:
				f *= 1.f + RandF(-info.mutateMultiplier, info.mutateMultiplier);
				break;
				// New
			case 2:
				f = RandF(-1, 1);
				break;
			}
		}
	}

	float* GABreed(Arena& arena, float* a, float* b, const GeneticAlgorithm& ga)
	{
		const uint32_t width = ga.info.width;

		auto aArr = reinterpret_cast<float*>(a);
		auto bArr = reinterpret_cast<float*>(b);
		auto c = arena.New<float>(width * 2);

		for (uint32_t i = 0; i < width; i++)
		{
			auto& f = c[i];
			const float aDom = aArr[width + i];
			const float bDom = bArr[width + i];
			const bool choice = RandF(0, aDom + bDom) < aDom;

			// Apply gene based on random dominance factor.
			f = choice ? aArr[i] : bArr[i];
			c[width + i] = choice ? aDom : bDom;
		}
		GAMutate(arena, c, ga);
		return c;
	}

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
			float** cpyGen = tempArena.New<float*>(info.length);
			for (uint32_t i = 0; i < info.length; i++)
				cpyGen[i] = GACopy(tempArena, generation[i], info.width);

			// Reset to start.
			arena.DestroyScope(genScope);

			auto indices = tempArena.New<uint32_t>(info.length);
			jv::CreateSortableIndices(indices, info.length);
			jv::ExtLinearSort(genRatings, indices, info.length, Comparer);
			jv::ApplyExtLinearSort(tempArena, cpyGen, indices, info.length);

			// Copy new best instance to result if applicable.
			auto bestRating = genRatings[indices[0]];
			genRating = bestRating;

			if (bestRating > this->rating)
			{
				arena.DestroyScope(resScope);
				this->rating = bestRating;
				result = GACopy(arena, cpyGen[0], info.width);
				genScope = arena.CreateScope();

				if (debug)
				{
					auto str = "GAR: " + std::to_string(bestRating);
					output.Add() = bt::OutputMsg::Create(str.c_str());
				}
			}

			// Sort again, but now with speciation in mind.
			assert(info.kmPointCount < info.length);
			if (info.kmPointCount > 1)
			{
				uint32_t c;
				jv::KMeansInfo kmInfo{};
				kmInfo.arena = &arena;
				kmInfo.tempArena = &tempArena;
				kmInfo.cycles = 50;
				kmInfo.instances = cpyGen;
				kmInfo.count = info.length;
				kmInfo.width = info.width;
				kmInfo.pointCount = info.kmPointCount;
				kmInfo.outCycleCount = &c;
				auto res = ApplyKMeans(kmInfo);
				auto conv = jv::ConvKMeansRes(arena, tempArena, res, kmInfo.pointCount);
			}

			tempArena.DestroyScope(tempScope);

			assert(info.breedablePct > 0 && info.breedablePct <= 1);
			const uint32_t apexLen = (float)info.length * info.apexPct;
			const uint32_t breedableLen = (float)info.length * info.breedablePct;
			const uint32_t end = info.length - (float)info.length * info.arrivalsPct;
			assert(end < info.length && end > breedableLen);

			// Breed successfull instances.
			for (uint32_t i = 0; i < end; i++)
			{
				uint32_t a = rand() % apexLen;
				uint32_t b = rand() % breedableLen;
				generation[i] = GABreed(arena, cpyGen[a], cpyGen[b], *this);
			}
			// Create new instances.
			for (uint32_t i = end; i < info.length; i++)
				GACreate(arena, info.width);
		}
	}

	GeneticAlgorithm GeneticAlgorithm::Create(Arena& arena, GeneticAlgorithmCreateInfo& info)
	{
		GeneticAlgorithm ga{};
		ga.info = info;
		ga.scope = arena.CreateScope();

		ga.generation = arena.New<float*>(info.length);
		ga.genRatings = arena.New<float>(info.length);

		ga.resScope = arena.CreateScope();
		ga.genScope = arena.CreateScope();
		
		for (uint32_t i = 0; i < info.length; i++)
			ga.generation[i] = GACreate(arena, info.width);
		ga.result = GACreate(arena, info.width);

		ga.rating = FLT_MIN;
		ga.genRating = FLT_MIN;
		ga.genId = 0;
		ga.trainId = 0;
		return ga;
	}

	void GeneticAlgorithm::Destroy(Arena& arena, GeneticAlgorithm& ga)
	{
		arena.DestroyScope(ga.scope);
	}
}