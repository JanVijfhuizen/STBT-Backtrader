#pragma once
#include <cstdint>

namespace jv
{
	struct Arena;

	namespace bt
	{
		struct Date final
		{
			uint32_t day;
			uint32_t month;
		};

		struct TimeSeries final
		{
			std::time_t date;

			float* open;
			float* high;
			float* low;
			float* close;
			uint32_t* volume;
			Date* dates;

			uint64_t scope;
			uint32_t length;
		};

		TimeSeries CreateTimeSeries(Arena& arena, uint32_t length);
		void DestroyTimeSeries(const TimeSeries& timeSeries, Arena& arena);
	}
}