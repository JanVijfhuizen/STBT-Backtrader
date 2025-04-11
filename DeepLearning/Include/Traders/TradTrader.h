#pragma once

namespace jv
{
	struct TradTrader final
	{
	};

	bool TradTraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output)
	{
		auto& trader = *reinterpret_cast<TradTrader*>(userPtr);
		return true;
	}

	bool TradTraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output)
	{
		auto& trader = *reinterpret_cast<TradTrader*>(userPtr);
		return true;
	}

	void TradTraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
	{
		auto& trader = *reinterpret_cast<TradTrader*>(userPtr);
	}
}


