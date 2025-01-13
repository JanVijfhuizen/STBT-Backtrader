#include "pch.h"
#include "GeneticAlgorithm.h"
#include <JLib/LinearSort.h>
#include <NNetUtils.h>

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

	bool Comparer(float& a, float& b) 
	{
		return a > b;
	}

	NNet RunGeneticAlgorithm(GeneticAlgorithmRunInfo& info, Arena& arena, Arena& tempArena)
	{
		const auto tempScope = tempArena.CreateScope();
		float* ratings = tempArena.New<float>(info.width);
		uint32_t* indices = tempArena.New<uint32_t>(info.width);
		NNet bestNNet{};
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

		jv::ai::Mutations mutations{};
		mutations.threshold.chance = .2;
		mutations.weight.chance = .2;
		mutations.decay.chance = .2;
		mutations.newNodeChance = .5;
		mutations.newWeightChance = .5;

		jv::ai::Mutations currentMutations = mutations;

		NNet* generations[2];
		for (uint32_t i = 0; i < 2; i++)
			generations[i] = tempArena.New<NNet>(info.width);

		uint32_t mutationId = 0;

		jv::ai::NNetCreateInfo nnetCreateInfo{};
		nnetCreateInfo.inputSize = info.inputSize;
		nnetCreateInfo.neuronCapacity = info.inputSize + info.outputSize + 1;
		nnetCreateInfo.weightCapacity = info.inputSize * info.outputSize + 3;
		nnetCreateInfo.outputSize = info.outputSize;

		// Set up first generation of random instances.
		for (uint32_t i = 0; i < info.width; i++)
		{
			NNet& nnet = generations[0][i];
			nnet = CreateNNet(nnetCreateInfo, arenas[0]);
			Init(nnet, InitType::random, mutationId);
			ConnectIO(nnet, jv::ai::InitType::random, mutationId);
		}

		// Scope used to store best nnet in.
		auto retScope = arena.CreateScope();

		uint32_t stagnateStreak = 0;
		float survivorRating = 0;
		float previousSurvivorRating;

		for (uint32_t i = 0; i < info.epochs; i++)
		{
			previousSurvivorRating = survivorRating;
			survivorRating = 0;
			++stagnateStreak;

			// Old/New generation index.
			uint32_t oInd = i % 2;
			uint32_t nInd = 1 - oInd;

			// Rate every instance of the generation.
			for (uint32_t j = 0; j < info.width; j++)
			{
				NNet& nnet = generations[oInd][j];
				ratings[j] = info.ratingFunc(nnet, info.userPtr, arena, tempArena);
			}

			CreateSortableIndices(indices, info.width);
			ExtLinearSort(ratings, indices, info.width, Comparer);

			const uint32_t hw = info.width / 2;
			// Copy best performing nnets to new generation.
			for (uint32_t j = 0; j < info.survivors; j++)
			{
				Copy(generations[oInd][indices[j]], generations[nInd][j], &arenas[nInd]);
				survivorRating += ratings[indices[j]];
			}

			survivorRating /= info.survivors;
			if (survivorRating > previousSurvivorRating)
				stagnateStreak = 0;

			// Delete old generation by wiping the entire arena.
			arenas[oInd].Clear();

			const auto nGen = generations[nInd];
			uint32_t breededCount = info.width - info.survivors - info.arrivals;

			float bestRating = ratings[indices[0]];
			if (Comparer(bestRating, bestNNetRating))
			{
				auto& nnet = generations[nInd][0];
				float avr = 0;

				const uint32_t VALIDATION_CHECK_AMOUNT = 10;
				for (uint32_t i = 0; i < VALIDATION_CHECK_AMOUNT; i++)
				{
					Clean(nnet);
					avr += info.ratingFunc(nnet, info.userPtr, arena, tempArena);
				}
					
				avr /= VALIDATION_CHECK_AMOUNT;

				if (Comparer(avr, bestNNetRating))
				{
					bestNNetRating = avr;
					arena.DestroyScope(retScope);
					Copy(nnet, bestNNet, &arena);
					retScope = arena.CreateScope();
					if(info.debug)
						std::cout << std::endl << std::endl << bestNNetRating << std::endl << std::endl;
				}
			}

			// Breed new generation.
			for (uint32_t j = 0; j < breededCount; j++)
			{
				// No two parents for now, just copy and mutate.
				// The issue now is that they breed from two entirely different architectures, 
				// effectively doubling the size every time, leaving no room for small improvements.
				/*
				auto& a = nGen[rand() % info.survivors];
				auto& b = nGen[rand() % info.survivors];
				auto& c = nGen[info.survivors + j] = Breed(a, b, arenas[nInd], tempArena);
				Mutate(c, currentMutations, mutationId);
				*/

				auto& parent = nGen[rand() % info.survivors];
				auto& child = nGen[info.survivors + j];
				Copy(parent, child, &arenas[nInd]);
				Mutate(child, currentMutations, mutationId);
			}

			// Add new random arrivals.
			for (uint32_t j = 0; j < info.arrivals; j++)
			{
				auto& nnet = generations[nInd][info.width - j - 1];
				nnet = CreateNNet(nnetCreateInfo, arenas[nInd]);
				Init(nnet, InitType::random, mutationId);
				ConnectIO(nnet, jv::ai::InitType::random, mutationId);
			}

			if(info.debug)
				std::cout << "e" << i << "S_" << survivorRating << "_N" << bestNNet.neuronCount << "W" << bestNNet.weightCount << "...";

			if (bestNNetRating >= info.targetScore && info.targetScore > 0)
				break;

			// If the algorithm is stuck, try micro adjusting the current networks to see if that works.
			if (stagnateStreak == info.stagnateAfter)
			{
				currentMutations.decay.pctAlpha = .1f;
				currentMutations.weight.pctAlpha = .1f;
				currentMutations.threshold.pctAlpha = .1f;

				currentMutations.decay.linAlpha = 0;
				currentMutations.weight.linAlpha = 0;
				currentMutations.threshold.linAlpha = 0;
				currentMutations.decay.canRandomize = false;
				currentMutations.weight.canRandomize = false;
				currentMutations.threshold.canRandomize = false;
				currentMutations.newNodeChance = 0;
				currentMutations.newWeightChance = 0;
			}
			// Slowly stagnate if no success is found.
			if (stagnateStreak >= info.stagnateAfter)
			{
				currentMutations.decay.chance *= info.stagnationMul;
				currentMutations.weight.chance *= info.stagnationMul;
				currentMutations.threshold.chance *= info.stagnationMul;
			}
			else
				currentMutations = mutations;
		}

		Arena::Destroy(arenas[1]);
		Arena::Destroy(arenas[0]);
		tempArena.DestroyScope(tempScope);
		return bestNNet;
	}
}