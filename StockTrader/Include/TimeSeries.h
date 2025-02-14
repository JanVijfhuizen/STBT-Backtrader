#pragma once
#include <cstdint>

namespace jv
{
	struct Arena;

	namespace bt
	{
		struct TimeSeries final
		{
			std::time_t date;

			float* open;
			float* high;
			float* low;
			float* close;
			uint32_t* volume;

			uint64_t scope;
			uint32_t length;
		};

		TimeSeries CreateTimeSeries(Arena& arena, uint32_t length);
		void DestroyTimeSeries(const TimeSeries& timeSeries, Arena& arena);
	}
}