#include "pch.h"
#include "Traders/NNetTrader.h"

namespace jv
{
	void ModInit(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t warmup, void* userPtr)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);
		auto modPtr = reinterpret_cast<NNetTraderDefaultMod*>(userPtr);
		const auto ts = info.scope->GetTimeSeries(stockId);
		modPtr->multiplier = 1.f / ts.close[info.start + warmup];
	}
	void ModUpdate(const bt::STBTBotInfo& info, const uint32_t stockId, const uint32_t current, float* out, void* userPtr) 
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);
		auto modPtr = reinterpret_cast<NNetTraderDefaultMod*>(userPtr);
		const auto ts = info.scope->GetTimeSeries(stockId);
		const float mul = modPtr->multiplier;

		out[0] = ts.open[current + 1] * mul;
		out[1] = ts.close[current + 1] * mul;
		out[2] = ts.high[current + 1] * mul;
		out[3] = ts.low[current + 1] * mul;
		out[4] = ts.dates[current + 1].day / 30;
		out[5] = ts.dates[current + 1].month / 12;
	}

	NNetTraderMod NNetGetDefaultMod(NNetTraderDefaultMod& out)
	{
		NNetTraderMod mod{};
		mod.outputCount = 6;
		mod.init = ModInit;
		mod.update = ModUpdate;
		mod.userPtr = &out;
		out = {};
		return mod;
	}
}