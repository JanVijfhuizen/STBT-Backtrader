#pragma once
#include "TimeSeries.h"
#include "Tracker.h"
#include "JLib/Array.h"

namespace jv::bt
{
	struct Portfolio final
	{
		Array<uint32_t> stocks;
		float liquidity;
	};

	struct World final
	{
		Array<TimeSeries> timeSeries;
		float fee;
	};

	struct BackTrader final
	{
		uint64_t scope;
		World world;
		Tracker tracker;
	};

	[[nodiscard]] BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, Array<const char*> symbols, float fee);
	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena);
}
