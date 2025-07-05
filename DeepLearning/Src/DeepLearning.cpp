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
		info.outputCount = 1;
		info.generationSize = 50;
		auto nnet = jv::ai::DynNNet::Create(arena, tempArena, info);
		nnet.alpha = 1;

		float iv[4]{ 0.2, 0.3, .1, -.5 };
		jv::Array<float> input{};
		input.ptr = iv;
		input.length = 4;

		bool requiredOutput[]{ true, true, false };

		bool out[2];
		jv::Array<bool> output{};
		output.ptr = out;
		output.length = 2;

		for (uint32_t i = 0; i < 1e4; i++)
		{
			auto current = nnet.GetCurrent();
			nnet.Construct(arena, tempArena, current);
			nnet.CreateParameters(arena);

			float highestRating = 0;

			for (uint32_t j = 0; j < 400; j++)
			{
				nnet.ConstructParameters(current, nnet.GetCurrentParameters());
				nnet.Flush(current);

				jv::FPFNTester tester{};
				float rating = 0;

				for (uint32_t k = 0; k < 25; k++)
				{
					nnet.Propagate(tempArena, input, output);
					if (k < 0)
						continue;

					// Arbitrary input is supposed to find sine.
					//const bool wanted = sin(.2 * k) > 0;
					const bool wanted = k % 3 == 0;

					rating += float(wanted == out[0]) / 3;
					rating += float(!wanted == out[1]) / 3;
					rating += float(out[0] != out[1]) / 3;
					tester.AddResult(wanted, out[0]);
					tester.AddResult(!wanted, out[1]);
					tester.AddResult(!out[0], out[1]);
				}

				highestRating = jv::Max(rating, highestRating);
				nnet.RateParameters(arena, tempArena, tester.GetRating());
			}

			nnet.DestroyParameters(arena);
			nnet.Deconstruct(arena, current);
			nnet.Rate(arena, tempArena);

			if (nnet.currentId == 0)
			{
				std::cout << "gen " << nnet.generationId << ": " << highestRating << " / " << nnet.rating << std::endl;
				std::cout << "n: " << nnet.result.neurons.length << " w:" << nnet.result.weights.length << std::endl;
				std::cout << nnet.neurons.count << " \ " << nnet.weights.count << std::endl;

				for (uint32_t j = 0; j < 10; j++)
				{
					std::cout << "n: " << nnet.generation[j].neurons.length << " w:" << nnet.generation[j].weights.length << std::endl;
				}
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
