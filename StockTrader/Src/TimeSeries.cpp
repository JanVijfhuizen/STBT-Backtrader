#include "pch.h"
#include "TimeSeries.h"

#include "JLib/Arena.h"

namespace jv::bt
{
	TimeSeries CreateTimeSeries(Arena& arena, const uint32_t length)
	{
		TimeSeries timeSeries{};
		timeSeries.length = length;
		timeSeries.scope = arena.CreateScope();
		timeSeries.open = static_cast<float*>(arena.Alloc(sizeof(float) * timeSeries.length));
		timeSeries.close = static_cast<float*>(arena.Alloc(sizeof(float) * timeSeries.length));
		timeSeries.high = static_cast<float*>(arena.Alloc(sizeof(float) * timeSeries.length));
		timeSeries.low = static_cast<float*>(arena.Alloc(sizeof(float) * timeSeries.length));
		timeSeries.volume = static_cast<uint32_t*>(arena.Alloc(sizeof(uint32_t) * timeSeries.length));
		timeSeries.dates = static_cast<Date*>(arena.Alloc(sizeof(Date) * timeSeries.length));
		return timeSeries;
	}

	void DestroyTimeSeries(const TimeSeries& timeSeries, Arena& arena)
	{
		arena.DestroyScope(timeSeries.scope);
	}
}
