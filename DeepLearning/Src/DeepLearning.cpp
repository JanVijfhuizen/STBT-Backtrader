#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>
#include <Traders/TradTrader.h>
#include <Traders/MainTrader.h>

void* MAlloc(const uint32_t size)
{
	return malloc(size);
}

void MFree(void* ptr)
{
	return free(ptr);
}

float kmDist(glm::vec3& a, glm::vec3& b)
{
	return distance(a, b);
}

void kmAdd(glm::vec3& a, glm::vec3& b)
{
	a += b;
}

void kmDiv(glm::vec3& a, uint32_t n)
{
	a /= n;
}

void kmClear(glm::vec3& a)
{
	a = {};
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
	/*
	{
		for (uint32_t i = 0; i < 100; i++)
		{
			auto arr = CreateArray<glm::vec3>(arena, 1000);
			for (auto& a : arr)
				a = { jv::RandF(-1, 1), jv::RandF(-1, 1), jv::RandF(-1, 1) };

			uint32_t c = 0;

			jv::KMeansInfo<glm::vec3> kmInfo{};
			kmInfo.arena = &arena;
			kmInfo.tempArena = &tempArena;
			kmInfo.cycles = 50;
			kmInfo.instances = arr.ptr;
			kmInfo.instanceCount = arr.length;
			kmInfo.pointCount = 20;
			kmInfo.add = kmAdd;
			kmInfo.dist = kmDist;
			kmInfo.div = kmDiv;
			kmInfo.clear = kmClear;
			kmInfo.outCycleCount = &c;
			auto res = ApplyKMeans(kmInfo);
			for (auto& a : res)
			{
				//std::cout << a << std::endl;
			}
			std::cout << c << std::endl;
		}
		
	}
	// END TEMP
	*/

	auto tradTrader = jv::TradTrader::Create(arena, tempArena);
	auto gaTrader = jv::GATrader::Create(arena, tempArena);
	auto mainTrader = jv::MainTrader::Create(arena, tempArena);
	mainTrader.InitGA();

	const char* bName = "Training";
	jv::bt::STBTBot bots[3];
	bots[0] = gaTrader.GetBot();
	bots[1] = tradTrader.GetBot();
	bots[2] = mainTrader.GetBot();

	auto stbt = jv::bt::CreateSTBT(bots, sizeof(bots) / sizeof(jv::bt::STBTBot));
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
