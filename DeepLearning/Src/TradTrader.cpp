#include "pch.h"
#include "Traders/TradTrader.h"
#include "TraderUtils.h"

namespace jv
{
	const uint32_t MA_LEN = 30;
	const uint32_t DEV_LEN = 20;
	const uint32_t T_LEN = MA_LEN + DEV_LEN;

	bool TradTraderInit(const bt::STBTBotInfo& info)
	{
		if (info.buffer < T_LEN)
		{
			auto str = "Buffer needs to be this at least " + std::to_string(T_LEN) + ".";
			info.output->Add() = bt::OutputMsg::Create(str.c_str(), bt::OutputMsg::error);
			return false;
		}

		auto& trader = *reinterpret_cast<TradTrader*>(info.userPtr);
		trader.runScope = trader.arena->CreateScope();

		const uint32_t l = info.scope->GetTimeSeriesCount();
		trader.start = info.start;
		trader.end = info.end;
		trader.buffer = l;
		trader.mas30 = trader.arena->New<float*>(l);

		const auto maLen = Min<uint32_t>(MA_LEN, info.buffer);
		for (uint32_t i = 0; i < l; i++)
		{
			auto close = info.scope->GetTimeSeries(i).close;
			trader.mas30[i] = TraderUtils::CreateMA(*trader.arena, info.start + DEV_LEN, info.end, maLen, close);
		}

		return true;
	}

	bool TradTraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto& trader = *reinterpret_cast<TradTrader*>(info.userPtr);
		const auto tempScope = trader.tempArena->CreateScope();
		const uint32_t l = info.scope->GetTimeSeriesCount();

		const uint32_t ind = trader.end + info.current;

		// Collect all stocks that are outside of their bollinger bands.
		auto bad = CreateVector<uint32_t>(*trader.tempArena, l);
		auto good = CreateVector<uint32_t>(*trader.tempArena, l);

		for (uint32_t i = 0; i < l; i++)
		{
			const float close = info.scope->GetTimeSeries(i).close[info.current];
			float& ma30 = trader.mas30[i][ind];
			const auto dev = TraderUtils::GetStandardDeviation(&ma30, DEV_LEN);
			const float topBand = ma30 + dev;
			const float floorBand = ma30 - dev;

			if (close > floorBand && close < topBand)
				continue;

			if (close < floorBand)
				good.Add() = i;
			else
				bad.Add() = i;
		}
		
		// Super simple, buy if below bands, sell if above.
		for (auto& i : bad)
			info.trades[i].change = -1e9;
		for (auto& i : good)
			info.trades[i].change = 1e9;

		trader.tempArena->DestroyScope(tempScope);
		return true;
	}

	void TradTraderCleanup(const bt::STBTBotInfo& info)
	{
		auto& trader = *reinterpret_cast<TradTrader*>(info.userPtr);
		trader.arena->DestroyScope(trader.runScope );
	}

	TradTrader TradTrader::Create(Arena& arena, Arena& tempArena)
	{
		TradTrader trader{};
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		return trader;
	}

	jv::bt::STBTBot TradTrader::GetBot()
	{
		jv::bt::STBTBot bot;
		bot.name = "Traditional Trader";
		bot.description = "No deep AI used.";
		bot.author = "jannie";
		bot.init = jv::TradTraderInit;
		bot.update = jv::TradTraderUpdate;
		bot.cleanup = jv::TradTraderCleanup;
		bot.userPtr = this;
		return bot;
	}
}