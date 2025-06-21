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
	createInfo.maxPropagations = 1;
	auto group = jv::nnet::Group::Create(arena, createInfo);

	uint32_t ni = 0;

	// WIP memory allocation issue.
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

		bool b = 0;
		jv::Array<bool> out{};
		out.ptr = &b;
		out.length = 1;

		auto& instance = group.GetTrainee();

		float f = 0;
		float score = 0;
		for (uint32_t i = 0; i < 100; i++)
		{
			f += 0.1f;
			instance.Propagate(arr, out);
			score += (b == sin(f) > 0);
		}

		jv::Queue<jv::bt::OutputMsg> msgs;
		group.Rate(arena, tempArena, score, msgs);

		if (group.trainId == 0)
			std::cout << group.genRating << std::endl;

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
