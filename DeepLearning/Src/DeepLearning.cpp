#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>
#include <Traders/TradTrader.h>
#include <Traders/MainTrader.h>
#include <Algorithms/KMeans.h>

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
	{
		for (uint32_t i = 0; i < 1; i++)
		{
			auto arr = CreateArray<float>(arena, 30000);
			for (auto& a : arr)
				a = jv::RandF(-1, 1);

			uint32_t c = 0;

			jv::KMeansInfo kmInfo{};
			kmInfo.arena = &arena;
			kmInfo.tempArena = &tempArena;
			kmInfo.cycles = 50;
			kmInfo.instances = arr.ptr;
			kmInfo.count = 600;
			kmInfo.width = 50;
			kmInfo.pointCount = 6;
			kmInfo.outCycleCount = &c;
			auto res = ApplyKMeans(kmInfo);
			for (auto& a : res)
			{
				//std::cout << a << std::endl;
			}
			//std::cout << c << "!!!" << std::endl;

			//for (auto& a : arr)
				//std::cout << a << std::endl;
			//std::cout << "!!" << std::endl;
			/*
			auto conv = jv::ConvKMeansRes(arena, tempArena, res, 2);
			for (auto& a : conv)
			{
				for(auto& b : a)
					std::cout << b << std::endl;
				std::cout << "/" << std::endl;
			}
			*/
			std::cout << c << std::endl;
		}
		
	}
	// END TEMP
	
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
