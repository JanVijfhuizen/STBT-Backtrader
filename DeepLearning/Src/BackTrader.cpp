#include "pch.h"
#include "BackTrader.h"

#include "JLib/Arena.h"
#include "JLib/ArrayUtils.h"

namespace jv::bt
{
	BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, const Array<const char*> symbols, const float fee)
	{
		BackTrader backTrader{};
		backTrader.scope = arena.CreateScope();
		backTrader.world = {};
		backTrader.world.timeSeries = CreateArray<TimeSeries>(arena, symbols.length);
		backTrader.world.fee = fee;
		backTrader.tracker = {};

		auto& tracker = backTrader.tracker;

		for (uint32_t i = 0; i < symbols.length; ++i)
		{
			const auto tempScope = tempArena.CreateScope();
			const auto str = tracker.GetData(tempArena, symbols[i]);
			const auto timeSeries = tracker.ConvertDataToTimeSeries(arena, str);
			backTrader.world.timeSeries[i] = timeSeries;
			tempArena.DestroyScope(tempScope);
		}
		
		return backTrader;
	}

	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena)
	{
		arena.DestroyScope(backTrader.scope);
	}
}
