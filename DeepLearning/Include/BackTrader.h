#pragma once
#include "TimeSeries.h"
#include "Tracker.h"
#include "JLib/Arena.h"
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
		// Stock ID
		uint32_t symbolId;
	};

	// Trade history
	typedef Array<Array<Call>> Log;

	struct Portfolio final
	{
		Array<uint32_t> stocks;
		float liquidity;

		void Copy(const Portfolio& other);
	};

	// Scope of a trade algorithm.
	struct World final
	{
		Array<TimeSeries> timeSeries;
		float fee;
	};

	// Stock trader bot.
	typedef void(*Bot)(Arena& tempArena, const World& world, const Portfolio& portfolio, Vector<Call>& calls, uint32_t offset, void* userPtr);
	// Data preprocessor for a stock trader.
	typedef void(*PreProcessBot)(Arena& tempArena, const World& world, uint32_t offset, uint32_t length, void* userPtr);

	struct RunInfo final
	{
		Bot bot;
		PreProcessBot preProcessBot = nullptr;
		void* userPtr = nullptr;
		uint32_t offset = 0;
		uint32_t length = 1;
	};

	struct TestInfo final
	{
		// Examined stock trainer bot.
		Bot bot;
		PreProcessBot preProcessBot = nullptr;
		void* userPtr = nullptr;
		// Number of train cycles.
		uint32_t epochs = 1000;
		// Days in a single cycle.
		uint32_t length = 30;
		// Offset from the current day.
		uint32_t maxOffset = 2000;
		// Starting cash.
		float liquidity = 1000;
	};

	struct BackTrader final
	{
		uint64_t scope;
		World world;
		Tracker tracker;
		Array<const char*> symbols;

		[[nodiscard]] float RunTestEpochs(Arena& arena, Arena& tempArena, const TestInfo& testInfo) const;
		[[nodiscard]] Portfolio Run(Arena& arena, Arena& tempArena, const Portfolio& portfolio, Log& outLog, const RunInfo& runInfo) const;
		[[nodiscard]] float GetLiquidity(const Portfolio& portfolio, uint32_t offset) const;

		void PrintAdvice(Arena& arena, Arena& tempArena, Bot bot, const char* portfolioName, 
			bool apply, PreProcessBot preProcessBot = nullptr) const;
	};

	struct BackTraderEnvironment final
	{
		Arena arena;
		Arena tempArena;
		BackTrader backTrader;
	};

	// Moving Average.
	[[nodiscard]] float GetMA(const float* data, uint32_t index, uint32_t length);
	[[nodiscard]] void Normalize(const float* src, float* dst, uint32_t index, uint32_t length);

	[[nodiscard]] Portfolio CreatePortfolio(Arena& arena, const BackTrader& backTrader);
	[[nodiscard]] Portfolio LoadPortfolio(Arena& arena, const BackTrader& backTrader, const char* name);
	void DestroyPortfolio(Arena& arena, const Portfolio& portfolio);
	void SavePortfolio(const char* name, const Portfolio& portfolio);

	[[nodiscard]] BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, const Array<const char*>& symbols, float fee);
	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena);

	[[nodiscard]] BackTraderEnvironment CreateBTE(const char** symbols, uint32_t symbolsLength, float fee);
	void DestroyBTE(const BackTraderEnvironment& bte);
}
