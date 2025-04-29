#include "pch.h"
#include "Traders/MainTrader.h"
#include "TraderUtils.h"
#include "Jlib/VectorUtils.h"

namespace jv
{
	bool MainTraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		mt->runScope = mt->arena->CreateScope();

		mt->start = start;
		mt->end = end;

		const uint32_t count = scope.GetTimeSeriesCount();
		mt->mas1 = mt->arena->New<float*>(count);
		mt->mas2 = mt->arena->New<float*>(count);

		for (uint32_t i = 0; i < count; i++)
		{
			auto close = scope.GetTimeSeries(i).close;
			mt->mas1[i] = TraderUtils::CreateMA(*mt->arena, start, end, mt->mas1Len, close);
			mt->mas2[i] = TraderUtils::CreateMA(*mt->arena, start, end, mt->mas2Len, close);
		}

		mt->threshPos = mt->arena->New<float>(count);
		mt->threshNeg = mt->arena->New<float>(count);

		return true;
	}

	bool MainTraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto tempScope = mt->tempArena->CreateScope();

		const uint32_t count = scope.GetTimeSeriesCount();
		auto buys = CreateVector<uint32_t>(*mt->tempArena, count);
		auto sells = CreateVector<uint32_t>(*mt->tempArena, count);

		const uint32_t index = mt->start - current;

		for (uint32_t i = 0; i < count; i++)
		{
			const float diff = mt->mas1[i][current] - mt->mas2[i][current];
			if (diff > mt->threshPos[i])
				buys.Add() = i;
			else if (diff < -mt->threshNeg[i])
				sells.Add() = i;
		}

		// Buy with an even distribution.
		const auto liq = scope.GetLiquidity();
		float stackPrice = 0;

		for (auto& buy : buys)
			stackPrice += scope.GetTimeSeries(buy).close[current];

		const uint32_t stackAmount = Max<float>(liq / stackPrice, 1);
		for (auto& buy : buys)
			trades[buy].change = stackAmount;

		for (auto& sell : sells)
			trades[sell].change = -1e5;

		mt->tempArena->DestroyScope(tempScope);
		return true;
	}

	void MainTraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		mt->arena->DestroyScope(mt->runScope);
	}

	MainTrader MainTrader::Create(Arena& arena, Arena& tempArena)
	{
		MainTrader mt{};
		mt.arena = &arena;
		mt.tempArena = &tempArena;
		return mt;
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