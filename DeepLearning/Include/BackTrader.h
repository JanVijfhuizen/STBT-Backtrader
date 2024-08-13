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

	typedef void(*Bot)(const World& world, const Portfolio& portfolio, Vector<Call>& calls, uint32_t offset, void* userPtr);

	struct RunInfo final
	{
		Bot bot;
		void* userPtr = nullptr;
		uint32_t offset = 0;
		uint32_t length = 1;
		bool debug = false;
	};

	struct TestInfo final
	{
		Bot bot;
		void* userPtr = nullptr;
		uint32_t epochs = 1000;
		uint32_t length = 30;
		uint32_t maxOffset = 2000;
		float liquidity = 1000;
	};

	struct BackTrader final
	{
		uint64_t scope;
		World world;
		Tracker tracker;

		[[nodiscard]] float RunTestEpochs(Arena& arena, Arena& tempArena, const TestInfo& testInfo) const;
		[[nodiscard]] Portfolio Run(Arena& arena, Arena& tempArena, const Portfolio& portfolio, Log& outLog, const RunInfo& runInfo) const;
		[[modiscard]] float GetLiquidity(const Portfolio& portfolio, uint32_t offset) const;

		void PrintAdvice(Arena& arena, Arena& tempArena, Bot bot, const Array<const char*>& symbols, const char* portfolioName, bool apply) const;
	};

	[[nodiscard]] Portfolio CreatePortfolio(Arena& arena, const BackTrader& backTrader);
	[[nodiscard]] Portfolio LoadPortfolio(Arena& arena, const BackTrader& backTrader, const char* name);
	void DestroyPortfolio(Arena& arena, const Portfolio& portfolio);
	void SavePortfolio(const char* name, const Portfolio& portfolio);

	[[nodiscard]] BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, const Array<const char*>& symbols, float fee);
	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena);
}
