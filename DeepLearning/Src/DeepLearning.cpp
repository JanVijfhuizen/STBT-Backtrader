#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>

struct GATrader final
{
	jv::GeneticAlgorithm ga;
	bool training = true;
};

bool GATraderInit(const jv::bt::STBTScope& scope, void* userPtr, uint32_t runIndex, 
	uint32_t runLength, jv::Queue<const char*>& output)
{
	return true;
}

bool GATraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades, 
	uint32_t current, void* userPtr, jv::Queue<const char*>& output)
{
	for (uint32_t i = 0; i < 4; i++)
	{
		trades[i].change = rand() % 3;
		trades[i].change -= 1;
	}

	return true;
}

void GATraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
{

}

int main()
{
	GATrader trader{};
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
	{
		jv::GeneticAlgorithmCreateInfo info{};
		info.width = 5;
		info.length = 100;
		trader.ga = jv::GeneticAlgorithm::Create(stbt.arena, info);
		trader.ga.RandInit();
	}

	while (!stbt.Update())
		continue;
	return 0;
}
