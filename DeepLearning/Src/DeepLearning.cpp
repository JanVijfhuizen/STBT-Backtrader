#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>
#include <Traders/TradTrader.h>
#include <Traders/MainTrader.h>
#include <Traders/CorrolationTrader.h>
#include <Algorithms/KMeans.h>

#include <Algorithms/NNet.h>
#include <Algorithms/HyperNNet.h>
#include <Algorithms/DynNNet.h>

void* MAlloc(const uint32_t size)
{
	return malloc(size);
}

void MFree(void* ptr)
{
	return free(ptr);
}

int main()
{
	srand(time(NULL));

	jv::Arena arena, tempArena;
	{
		jv::ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		arena = jv::Arena::Create(arenaCreateInfo);
		tempArena = jv::Arena::Create(arenaCreateInfo);
	}

	{
		jv::ai::DynNNetCreateInfo info{};
		info.inputCount = 4;
		info.outputCount = 2;
		info.generationSize = 50;
		auto nnet = jv::ai::DynNNet::Create(arena, tempArena, info);
		nnet.alpha = 10;
		nnet.kmPointCount = 3;
		nnet.gaLength = 40;
		nnet.gaKmPointCount = 3;

		float iv[4]{ 0.2, 0.3, .1, -.5 };
		jv::Array<float> input{};
		input.ptr = iv;
		input.length = 4;

		float out[2];
		jv::Array<float> output{};
		output.ptr = out;
		output.length = 2;

		float bestFPFNRating = 0;
		uint32_t cyclesSinceNewBestRating = 0;
		float highestRating = 0;

		for (uint32_t i = 0; i < 1e4; i++)
		{
			auto current = nnet.GetCurrent();
			nnet.Construct(arena, tempArena, current);
			nnet.CreateParameters(arena);
			float currentBestFPFNRating = 0;

			if (i % (nnet.generation.length / 10) == 0)
				std::cout << "-";

			const uint32_t l = nnet.generation.length * 10;
			for (uint32_t j = 0; j < l; j++)
			{
				nnet.ConstructParameters(current, nnet.GetCurrentParameters());
				nnet.Flush(current);

				jv::FPFNTester tester{};
				float rating = 0;

				for (uint32_t k = 0; k < 25; k++)
				{
					input[0] = float(k % 3 == 0);
					//input[1] = sin(.2 * k);
					nnet.Propagate(tempArena, input, output);
					if (k < 0)
						continue;

					// Arbitrary input is supposed to find sine.
					const float si = sin(.2 * k);
					const bool wanted = si > 0;
					//const bool wanted = k % 3 == 0;
					const float fWanted = float(wanted);

					bool o1 = bool(int(round(out[0])));
					bool o2 = bool(int(round(out[1])));

					rating += wanted == o1;
					rating += wanted != o2;
					rating += o1 != o2;

					tester.AddResult(o1, wanted);
					tester.AddResult(o2, !wanted);
					tester.AddResult(!o1, o2);
				}

				const float r = tester.GetRating();
				highestRating = jv::Max(rating, highestRating);
				currentBestFPFNRating = jv::Max(currentBestFPFNRating, r);
				nnet.RateParameters(arena, tempArena, r);
			}	

			nnet.DestroyParameters(arena);
			nnet.Deconstruct(arena, current);
			nnet.Rate(arena, tempArena);

			if (nnet.currentId == 0)
			{
				std::cout << std::endl;
				std::cout << "gen " << nnet.generationId << ": " << highestRating << " / " << nnet.rating << std::endl;
				std::cout << "n: " << nnet.result.neurons.length << " w:" << nnet.result.weights.length << std::endl;
				std::cout << nnet.neurons.count << " \ " << nnet.weights.count << std::endl;

				highestRating = 0;

				const uint32_t l = nnet.generation.length * nnet.apexPct;
				for (uint32_t j = 0; j < l; j++)
				{
					std::cout << "n: " << nnet.generation[j].neurons.length << " w:" << nnet.generation[j].weights.length << std::endl;
				}

				++cyclesSinceNewBestRating;
				if (currentBestFPFNRating > bestFPFNRating) 
				{
					cyclesSinceNewBestRating = 0;
					bestFPFNRating = currentBestFPFNRating;
				}
				if (cyclesSinceNewBestRating > 10)
					break;
			}
				
		}
	}
	
	auto tradTrader = jv::TradTrader::Create(arena, tempArena);
	auto gaTrader = jv::GATrader::Create(arena, tempArena);
	auto mainTrader = jv::MainTrader::Create(arena, tempArena);
	auto corTrader = jv::CorrolationTrader::Create(arena, tempArena);
	mainTrader.InitGA();

	jv::bt::STBTBot bots[4];
	bots[0] = gaTrader.GetBot();
	bots[1] = tradTrader.GetBot();
	bots[2] = mainTrader.GetBot();
	bots[3] = corTrader.GetBot();

	auto stbt = jv::bt::CreateSTBT(bots, sizeof(bots) / sizeof(jv::bt::STBTBot));
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
