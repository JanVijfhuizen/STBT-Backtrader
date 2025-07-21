#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>
#include <Traders/TradTrader.h>
#include <Traders/MainTrader.h>
#include <Traders/CorrolationTrader.h>
#include <Traders/NNetTrader.h>
#include <Traders/Modules/MBounds.h>
#include <Jlib/ArrayUtils.h>

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

	auto tradTrader = jv::TradTrader::Create(arena, tempArena);
	auto gaTrader = jv::GATrader::Create(arena, tempArena);
	auto mainTrader = jv::MainTrader::Create(arena, tempArena);
	mainTrader.InitGA();
	auto corTrader = jv::CorrolationTrader::Create(arena, tempArena);

	jv::NNetTraderDefaultMod defaultMod;
	jv::NNetTraderModBounds mBounds;
	jv::NNetTrader nnetTrader;
	{
		auto mods = CreateArray<jv::NNetTraderMod>(arena, 2);
		mods[0] = jv::NNetGetDefaultMod(defaultMod);
		mods[1] = jv::NNetGetTraderModBounds(mBounds);

		auto timeFrames = CreateArray<uint32_t>(arena, 1);
		timeFrames[0] = 0;
		//timeFrames[1] = 3;
		//timeFrames[2] = 6;

		jv::NNetTraderCreateInfo info{};
		info.mods = mods;
		info.timeFrames = timeFrames;
		nnetTrader = jv::NNetTrader::Create(arena, tempArena, info);
	}

	jv::bt::STBTBot bots[5];
	bots[0] = gaTrader.GetBot();
	bots[1] = tradTrader.GetBot();
	bots[2] = mainTrader.GetBot();
	bots[3] = corTrader.GetBot();
	bots[4] = nnetTrader.GetBot();

	auto stbt = jv::bt::CreateSTBT(bots, sizeof(bots) / sizeof(jv::bt::STBTBot));
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
