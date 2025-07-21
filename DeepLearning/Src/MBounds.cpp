#include "pch.h"
#include "Traders/Modules/MBounds.h"
#include <Jlib/ArrayUtils.h>
#include <TraderUtils.h>

namespace jv
{
	void NNetTraderModBoundsInit(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t warmup, void* userPtr)
	{
		auto ptr = reinterpret_cast<NNetTraderModBounds*>(userPtr);

		auto ts = info.scope->GetTimeSeries(stockId);
		const uint32_t start = info.start + info.buffer - ptr->maLen;
		ptr->ma = TraderUtils::CreateMA(*info.arena, start, info.end, ptr->maLen, ts.open);
	}
	void NNetTraderModBoundsUpdate(const bt::STBTBotInfo& info, 
		const uint32_t stockId, const uint32_t current, float* out, void* userPtr)
	{
		auto ptr = reinterpret_cast<NNetTraderModBounds*>(userPtr);
		const auto ts = info.scope->GetTimeSeries(stockId);

		auto series = info.scope->GetTimeSeries(stockId);
		const float point = series.close[current];
		const float maPoint = ptr->ma[info.start + info.buffer - current];

		const float std = TraderUtils::GetStandardDeviation(&series.close[current], ptr->stdLen);
		const float lower = point - std * ptr->stdMul;
		const float upper = point + std * ptr->stdMul;

		out[0] = lower > point;
		out[1] = upper < point;
	}
	uint32_t NNetTraderModBoundsGetMinBufferSize(const bt::STBTBotInfo& info, uint32_t stockId, void* userPtr)
	{
		auto ptr = reinterpret_cast<NNetTraderModBounds*>(userPtr);
		return ptr->maLen;
	}

	NNetTraderMod NNetGetTraderModBounds(NNetTraderModBounds& out)
	{
		NNetTraderMod mod{};
		out = {};
		mod.init = NNetTraderModBoundsInit;
		mod.update = NNetTraderModBoundsUpdate;
		mod.getMinBufferSize = NNetTraderModBoundsGetMinBufferSize;
		mod.userPtr = &out;
		mod.outputCount = 2;
		return mod;
	}
}