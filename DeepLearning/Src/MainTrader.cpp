#include "pch.h"
#include "Traders/MainTrader.h"
#include "TraderUtils.h"
#include "Jlib/VectorUtils.h"
#include <Traders/Modules/ModMA.h>

namespace jv
{
	bool MainTraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);

		mt->modMA = {};
		mt->manager = tmm::Manager::Create(*mt->arena, 1);
		mt->manager.Set(0, &mt->modMA);

		tmm::Info info{};
		info.start = start;
		info.end = end;
		info.runIndex = runIndex;
		info.nRuns = nRuns;
		info.buffer = buffer;
		return mt->manager.Init(*mt->arena, info, scope, output);
	}

	bool MainTraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		
		return true;
	}

	void MainTraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		tmm::Manager::Destroy(*mt->arena, mt->manager);
	}

	MainTrader MainTrader::Create(Arena& arena, Arena& tempArena)
	{
		MainTrader mt{};
		mt.arena = &arena;
		mt.tempArena = &tempArena;
		mt.scope = arena.CreateScope();
		return mt;
	}
	void MainTrader::Destroy(Arena& arena, MainTrader& trader)
	{
		arena.DestroyScope(trader.scope);
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