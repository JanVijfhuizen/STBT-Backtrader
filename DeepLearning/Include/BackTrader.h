#pragma once
#include "TimeSeries.h"
#include "Tracker.h"
#include "JLib/Array.h"
#include "JLib/Vector.h"

namespace jv::bt
{
	enum class CallType
	{
		Buy,
		Sell
	};

	struct Call final
	{
		CallType type;
		uint32_t amount;
		uint32_t symbolId;
	};

	typedef Array<Array<Call>> Log;

	struct Portfolio final
	{
		Array<uint32_t> stocks;
		float liquidity;

		void Copy(const Portfolio& other);
	};

	struct World final
	{
		Array<TimeSeries> timeSeries;
		float fee;
	};

	struct RunInfo final
	{
		void(*func)(const World& world, const Portfolio& portfolio, Vector<Call>& calls, uint32_t offset, void* userPtr);
		void* userPtr = nullptr;
		uint32_t offset = 0;
		uint32_t length = 1;
	};

	struct BackTrader final
	{
		uint64_t scope;
		World world;
		Tracker tracker;

		[[nodiscard]] Portfolio Run(Arena& arena, Arena& tempArena, const Portfolio& portfolio, Log& outLog, const RunInfo& runInfo) const;
		[[modiscard]] float GetLiquidity(const Portfolio& portfolio, uint32_t offset) const;
	};

	[[nodiscard]] Portfolio CreatePortfolio(Arena& arena, const BackTrader& backTrader);
	[[nodiscard]] Portfolio LoadPortfolio(Arena& arena, const BackTrader& backTrader, const char* name);
	void DestroyPortfolio(Arena& arena, const Portfolio& portfolio);
	void SavePortfolio(const char* name, const Portfolio& portfolio);

	[[nodiscard]] BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, Array<const char*> symbols, float fee);
	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena);
}
