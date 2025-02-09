#include "pch.h"
#include "GeneticAlgorithm.h"
#include <JLib/LinearSort.h>
#include <NNetUtils.h>
#include <Shader.h>
#include <Mesh.h>
#include <Renderer.h>
#include <Jlib/VectorUtils.h>

namespace jv::ai
{
	struct EpochDebugData final
	{
		float score;
		float filtered;
		float bestScore;
	};

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
		gr::Renderer renderer;

		if (info.debug)
		{
			gr::RendererCreateInfo createInfo{};
			createInfo.title = "GA Progress";
			renderer = gr::CreateRenderer(createInfo);
		}

		const auto tempScope = tempArena.CreateScope();
		float* ratings = tempArena.New<float>(info.width);
		float* compabilities = tempArena.New<float>(info.width);
		uint32_t* indices = tempArena.New<uint32_t>(info.width);
		
		float bestNNetRating = -1;
		NNet bestNNet{};
		Arena arenas[2];

		// Allocate memory from temp arena with predetermined minimal size.
		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = Alloc;
		arenaCreateInfo.free = Free;
		arenaCreateInfo.memorySize = info.initMemSize / 2;
		arenaCreateInfo.memory = tempArena.Alloc(info.initMemSize / 2);
		arenas[0] = Arena::Create(arenaCreateInfo);
		arenaCreateInfo.memory = tempArena.Alloc(info.initMemSize / 2);
		arenas[1] = Arena::Create(arenaCreateInfo);

		jv::ai::Mutations currentMutations = info.mutations;

		NNet* generations[2];
		for (uint32_t i = 0; i < 2; i++)
			generations[i] = tempArena.New<NNet>(info.width);

		uint32_t mutationId = 0;

		jv::ai::NNetCreateInfo nnetCreateInfo{};
		nnetCreateInfo.inputSize = info.inputSize;
		nnetCreateInfo.neuronCapacity = info.inputSize + info.outputSize + 1;
		nnetCreateInfo.weightCapacity = info.inputSize * info.outputSize + 3;
		nnetCreateInfo.outputSize = info.outputSize;

		// Add mutation space.
		nnetCreateInfo.neuronCapacity += info.arrivalMutationCount;
		nnetCreateInfo.weightCapacity += info.arrivalMutationCount * 3;

		// Set up first generation of random instances.
		for (uint32_t i = 0; i < info.width; i++)
		{
			NNet& nnet = generations[0][i];
			nnet = CreateNNet(nnetCreateInfo, arenas[0]);
			Init(nnet, InitType::random, mutationId);
			ConnectIO(nnet, jv::ai::InitType::random, mutationId);

			for (uint32_t j = 0; j < info.arrivalMutationCount; j++)
				Mutate(nnet, currentMutations, mutationId);
		}

		// Scope used to store best nnet in.
		auto retScope = arena.CreateScope();

		uint32_t stagnateStreak = 0;
		float survivorRating = 0;
		float previousSurvivorRating;

		auto epochDebugData = CreateVector<EpochDebugData>(tempArena, info.epochs);

		for (uint32_t i = 0; i < info.epochs; i++)
		{
			if (info.debug)
			{
				renderer.DrawPlane(glm::vec2(0), glm::vec2(1 * renderer.GetAspectRatio(), 1), glm::vec4(1));

				float lineWidth = 2.f / (epochDebugData.count - 1);

				for (uint32_t j = 1; j < epochDebugData.count; j++)
				{
					float xStart = lineWidth * (j - 1) - 1.f;
					float xEnd = xStart + lineWidth;

					const auto& prev = epochDebugData[j - 1];
					const auto& cur = epochDebugData[j];
					float prevScore = prev.score / bestNNetRating;
					float score = cur.score / bestNNetRating;

					renderer.DrawLine(glm::vec2(xStart, prevScore - 1), glm::vec2(xEnd, score - 1), glm::vec4(1, 0, 0, 1));
					renderer.DrawLine(glm::vec2(xStart, prev.bestScore / bestNNetRating - 1), glm::vec2(xEnd, cur.bestScore / bestNNetRating - 1), glm::vec4(0, 1, 0, 1));
					renderer.DrawLine(glm::vec2(xStart, prev.filtered / bestNNetRating - 1), glm::vec2(xEnd, cur.filtered / bestNNetRating - 1), glm::vec4(0, 0, 1, 1));
				}
				
				const bool result = renderer.Render();
				if (result)
					break;
			}

			previousSurvivorRating = survivorRating;
			survivorRating = 0;
			++stagnateStreak;

			// Old/New generation index.
			uint32_t oInd = i % 2;
			uint32_t nInd = 1 - oInd;

			for (uint32_t j = 0; j < info.width; j++)
				compabilities[j] = 0;

			float bestRatingUnfiltered = -1;
			uint32_t bestRatingUnfilteredIndex = -1;

			// Rate every instance of the generation.
			for (uint32_t j = 0; j < info.width; j++)
			{
				NNet& nnet = generations[oInd][j];
				ratings[j] = info.ratingFunc(nnet, info.userPtr, arena, tempArena);

				// Set best current rating if it's the best of this generation.
				if (Comparer(ratings[j], bestRatingUnfiltered))
				{
					bestRatingUnfilteredIndex = j;
					bestRatingUnfiltered = ratings[j];
				}

				for (uint32_t k = j + 1; k < info.width; k++)
				{
					NNet& oNNet = generations[oInd][k];
					const float compability = GetCompability(nnet, oNNet);
					compabilities[j] += compability;
					compabilities[k] += compability;
				}

				auto c = compabilities[j];
				c /= (info.width - 1);
				c = 1.f - c;
				ratings[j] *= c;
			}

			if (survivorRating > previousSurvivorRating)
				stagnateStreak = 0;

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

			if (Comparer(bestRatingUnfiltered, bestNNetRating))
			{
				auto& nnet = generations[oInd][bestRatingUnfilteredIndex];
				float avr = 0;

				for (uint32_t j = 0; j < info.validationCheckAmount; j++)
				{
					Clean(nnet);
					avr += info.ratingFunc(nnet, info.userPtr, arena, tempArena);
				}
					
				avr /= info.validationCheckAmount;

				if (Comparer(avr, bestNNetRating))
				{
					stagnateStreak = 0;
					bestNNetRating = avr;
					arena.DestroyScope(retScope);
					retScope = arena.CreateScope();
					Copy(nnet, bestNNet, &arena);
					if(info.debug)
						std::cout << std::endl << std::endl << bestNNetRating << std::endl << std::endl;
				}
			}

			// Delete old generation by wiping the entire arena.
			arenas[oInd].Clear();

			const auto nGen = generations[nInd];
			uint32_t breededCount = info.width - info.survivors - info.arrivals;

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
				for (uint32_t j = 0; j < info.arrivalMutationCount; j++)
					Mutate(nnet, currentMutations, mutationId);
			}

			if (info.debug)
			{
				EpochDebugData debugData{};
				debugData.score = bestRatingUnfiltered;
				debugData.bestScore = bestNNetRating;
				debugData.filtered = ratings[0];
				epochDebugData.Add() = debugData;
			}

			if(info.debug)
				std::cout << "e" << i << "S_" << bestNNetRating << "_N" << bestNNet.neuronCount << "W" << bestNNet.weightCount << "...";

			if (bestNNetRating >= info.targetScore && info.targetScore > 0)
				break;

			// If the algorithm is stuck, try micro adjusting the current networks to see if that works.
			if (stagnateStreak == info.stagnateAfter)
			{
				currentMutations.decay.pctAlpha = info.stagnationMaxPctChange;
				currentMutations.weight.pctAlpha = info.stagnationMaxPctChange;
				currentMutations.threshold.pctAlpha = info.stagnationMaxPctChange;

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
				currentMutations = info.mutations;
		}

		Arena::Destroy(arenas[1]);
		Arena::Destroy(arenas[0]);
		tempArena.DestroyScope(tempScope);

		if (info.debug)
		{
			gr::DestroyRenderer(renderer);
		}

		return {}; // temp, somehow returning bestnnet fucks with debugging
	}
}