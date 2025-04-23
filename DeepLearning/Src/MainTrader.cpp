#include "pch.h"
#include "Traders/MainTrader.h"

namespace jv
{
	bool MainTraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output)
	{
		return true;
	}

	bool MainTraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output)
	{
		return true;
	}

	void MainTraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
	{
		
	}

	MainTrader MainTrader::Create(Arena& arena, Arena& tempArena)
	{
		return MainTrader();
	}
	bt::STBTBot MainTrader::GetBot()
	{
		bt::STBTBot bot{};
		bot.name = "Main trader";
		bot.description = "First Big Attempt.";
		bot.author = "jannie";
		bot.init = MainTraderInit;
		bot.update = MainTraderUpdate;
		bot.cleanup = MainTraderCleanup;
		bot.userPtr = this;
		return bot;
	}
}