#include "pch.h"
#include "Traders/CorrolationTrader.h"

namespace jv
{
	bool COTraderInit(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<CorrolationTrader*>(info.userPtr);
		ptr->runScope = ptr->arena->CreateScope();

		// Ahritmic.
		const uint32_t c = info.scope->GetTimeSeriesCount() - 1;
		const uint32_t n = c * (c + 1) / 2;

		ptr->corrolations = ptr->arena->New<float>(n);
		return true;
	}

	bool COTraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto ptr = reinterpret_cast<CorrolationTrader*>(info.userPtr);
		const uint32_t current = info.current;

		const uint32_t c = info.scope->GetTimeSeriesCount();
		uint32_t k = 0;
		for (uint32_t i = 0; i < c; i++)
		{
			const float a = info.scope->GetTimeSeries(i).open[current];
			const float a2 = info.scope->GetTimeSeries(i).open[current + 1];
			const float aDelta = a2 / a;

			for (uint32_t j = i + 1; j < c; j++)
			{
				const float b = info.scope->GetTimeSeries(j).open[current];
				const float b2 = info.scope->GetTimeSeries(j).open[current + 1];
				const float bDelta = b2 / b;

				ptr->corrolations[k++] += aDelta * bDelta; 
			}
		}

		return true;
	}

	void COTraderCleanup(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<CorrolationTrader*>(info.userPtr);

		const uint32_t c = info.scope->GetTimeSeriesCount() - 1;
		const uint32_t n = c * (c + 1) / 2;
		for (uint32_t i = 0; i < n; i++) 
		{
			auto& cor = ptr->corrolations[i];
			cor /= info.start - info.end;

			cor *= 100;
			cor -= 100;

			auto str = std::to_string(cor);
			str += "%";
			info.output->Add() = bt::OutputMsg::Create(str.c_str());
		}

		ptr->arena->DestroyScope(ptr->runScope);
	}

	void COTraderRender(const bt::STBTBotInfo& info, gr::RenderProxy renderer, glm::vec2 center)
	{
	}

	CorrolationTrader CorrolationTrader::Create(Arena& arena, Arena& tempArena)
	{
		auto trader = CorrolationTrader();
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		return trader;
	}

	bt::STBTBot CorrolationTrader::GetBot()
	{
		bt::STBTBot bot{};
		bot.name = "Corrolation Trader";
		bot.author = "jannie";
		bot.description = "calculates the corrolation\nbetween all stocks.";
		bot.init = COTraderInit;
		bot.update = COTraderUpdate;
		bot.cleanup = COTraderCleanup;
		bot.customRender = COTraderRender;
		bot.userPtr = this;
		return bot;
	}
}