#pragma once

namespace jv
{
	struct MainTrader final
	{
		Arena* arena;
		Arena* tempArena;
		uint64_t runScope;

		uint32_t mas1Len = 30;
		uint32_t mas2Len = 100;

		uint32_t start, end;

		// Moving Averages.
		float** mas1;
		float** mas2;

		// Buy/Sell Thresholds.
		float* threshPos;
		float* threshNeg;

		[[nodiscard]] static MainTrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}
