#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>

void* MAlloc(const uint32_t size)
{
	return malloc(size);
}

void MFree(void* ptr)
{
	return free(ptr);
}

struct GATrader final
{
	jv::Arena* arena;
	jv::GeneticAlgorithm ga;
	bool training = true;

	// Train instance info.
	float startV;
	uint32_t endId;
};

bool GATraderInit(const jv::bt::STBTScope& scope, void* userPtr, uint32_t runIndex, 
	uint32_t runLength, jv::Queue<const char*>& output)
{
	auto& gt = *reinterpret_cast<GATrader*>(userPtr);
	gt.startV = scope.GetPortValue(runIndex);
	return true;
}

bool GATraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades, 
	uint32_t current, void* userPtr, jv::Queue<const char*>& output)
{
	auto& gt = *reinterpret_cast<GATrader*>(userPtr);
	float* algo = gt.training ? gt.ga.GetTrainee() : gt.ga.result;

	float v = algo[current];
	trades[0].change = v > 0 ? 1e9 : -1e9;
	gt.endId = current;

	/*
	for (uint32_t i = 0; i < 4; i++)
	{
		trades[i].change = rand() % 3;
		trades[i].change -= 1;
	}
	*/

	return true;
}

void GATraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
{
	auto& gt = *reinterpret_cast<GATrader*>(userPtr);
	if (gt.training)
		gt.ga.Rate(*gt.arena, scope.GetPortValue(gt.endId) - gt.startV);
}

int main()
{
	jv::Arena arena;
	{
		jv::ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		arena = jv::Arena::Create(arenaCreateInfo);
	}

	GATrader trader{};
	{
		jv::GeneticAlgorithmCreateInfo info{};
		info.width = 30;
		info.length = 20;
		trader.arena = &arena;
		trader.ga = jv::GeneticAlgorithm::Create(arena, info);
		trader.ga.RandInit();
	}

	const char* bName = "Training";
	jv::bt::STBTBot bot;
	bot.name = "GA trader";
	bot.description = "Genetic Algorithm Trading.";
	bot.author = "jannie";
	bot.init = GATraderInit;
	bot.update = GATraderUpdate;
	bot.cleanup = GATraderCleanup;
	bot.bools = &trader.training;
	bot.boolsNames = &bName;
	bot.boolsLength = 1;
	bot.userPtr = &trader;

	auto stbt = jv::bt::CreateSTBT(&bot, 1);
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
