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
	//srand(time(NULL));

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
		info.outputCount = 3;
		info.generationSize = 50;
		auto nnet = jv::ai::DynNNet::Create(arena, tempArena, info);

		float iv[4]{0.2, 0.3, .1, -.5f};
		jv::Array<float> input{};
		input.ptr = iv;
		input.length = 4;

		bool requiredOutput[]{ true, true, false };

		bool out[3]{};
		jv::Array<bool> output{};
		output.ptr = out;
		output.length = 3;

		for (uint32_t i = 0; i < 1e4; i++)
		{
			auto current = nnet.GetCurrent();
			nnet.Construct(arena, tempArena, current);
			nnet.CreateParameters(arena);

			for (uint32_t j = 0; j < 1e4; j++)
			{
				nnet.ConstructParameters(current, nnet.GetCurrentParameters());
				nnet.Propagate(tempArena, input, output);

				uint32_t rating = 0;
				for (uint32_t k = 0; k < 3; k++)
					rating += output[k] == requiredOutput[k];
				nnet.RateParameters(arena, tempArena, rating);
			}

			nnet.DestroyParameters(arena);
			nnet.Deconstruct(arena, current);
			nnet.Rate(arena, tempArena);

			if (nnet.currentId == 0)
				std::cout << "gen " << nnet.generationId << ": " << nnet.rating << std::endl;
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
