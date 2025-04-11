#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>
#include <Traders/TradTrader.h>

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

	auto tradTrader = jv::TradTrader::Create(arena, tempArena);
	auto gaTrader = jv::GATrader::Create(arena, tempArena);

	const char* bName = "Training";
	jv::bt::STBTBot bots[2];
	bots[0] = gaTrader.GetBot();
	bots[1] = tradTrader.GetBot();

	auto stbt = jv::bt::CreateSTBT(bots, 2);
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
