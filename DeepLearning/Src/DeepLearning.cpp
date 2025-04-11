#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>

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

	auto gaTrader = jv::GATrader::Create(arena, tempArena);

	const char* bName = "Training";
	jv::bt::STBTBot bots[1];
	bots[0] = gaTrader.GetBot();

	auto stbt = jv::bt::CreateSTBT(bots, 1);
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
