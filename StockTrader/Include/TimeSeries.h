#pragma once
#include <cstdint>

namespace jv
{
	struct Arena;

	namespace bt
	{
		struct TimeSeries final
		{
			float* open;
			float* high;
			float* low;
			float* close;
			uint32_t* volume;

			uint64_t scope;
			uint32_t length;
		};

		__declspec(dllexport) TimeSeries CreateTimeSeries(Arena& arena, uint32_t length);
		__declspec(dllexport) void DestroyTimeSeries(const TimeSeries& timeSeries, Arena& arena);
	}
}