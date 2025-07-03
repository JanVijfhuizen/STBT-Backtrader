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
		info.outputCount = 3;
		auto nnet = jv::ai::DynNNet::Create(arena, tempArena, info);
		auto constructed = jv::ai::DynCInstance::Create(arena, tempArena, nnet, nnet.generation[0]);
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
