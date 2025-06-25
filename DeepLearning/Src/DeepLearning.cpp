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
	jv::Arena arena, tempArena;
	{
		jv::ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		arena = jv::Arena::Create(arenaCreateInfo);
		tempArena = jv::Arena::Create(arenaCreateInfo);
	}


	// TEMP

	jv::nnet::GroupCreateInfo createInfo{};
	createInfo.inputCount = 3;
	createInfo.outputCount = 1;
	createInfo.length = 200;
	auto group = jv::nnet::Group::Create(arena, tempArena, createInfo);

	uint32_t ni = 0;

	uint32_t highest = 0;

	while (true)
	{
		float fs[]
		{
			.2f,
			.8f,
			-.45f
		};

		jv::Array<float> arr{};
		arr.ptr = fs;
		arr.length = sizeof(fs) / sizeof(float);

		bool output = 0;
		jv::Array<bool> out{};
		out.ptr = &output;
		out.length = 1;

		auto& instance = group.GetTrainee();

		jv::FPFNTester tester{};

		float f = 0;
		uint32_t correctness = 0;

		for (uint32_t i = 0; i < 500; i++)
		{
			f += 0.01f;
			instance.Propagate(tempArena, arr, out);

			const bool a = sin(f) > 0;
			const bool b = sin(f * .7f) > 0;
			const bool c = i % 4 == 0;

			const bool expected = a && c;

			correctness += output == expected;
			tester.AddResult(output, expected);
		}

		highest = jv::Max(highest, correctness);

		jv::Queue<jv::bt::OutputMsg> msgs;
		group.Rate(arena, tempArena, tester.GetRating(), msgs);

		if (group.trainId == 0)
		{
			std::cout << group.genRating << " // " << highest << std::endl;
		}
			

		++ni;
	}

	// END TEMP
	
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
