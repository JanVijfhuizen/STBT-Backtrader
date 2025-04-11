#pragma once

namespace jv
{
	// An example of a traditional basic trader without the use of deep AI.
	struct TradTrader final
	{
		Arena* arena;
		Arena* tempArena;

		uint64_t runScope;
		uint32_t start, end, buffer;

		// Moving Averages.
		float** mas30;

		[[nodiscard]] static TradTrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};

	bool TradTraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		uint32_t start, uint32_t end, uint32_t runIndex, uint32_t nRuns, uint32_t buffer);
	bool TradTraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output);
	void TradTraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output);
}


