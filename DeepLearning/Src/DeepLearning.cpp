#include "pch.h"

#include <random>
#include "Tracker.h"
#include "JLib/Arena.h"

void* Alloc(const uint32_t size)
{
	return malloc(size);
}
void Free(void* ptr)
{
	return free(ptr);
}

int main()
{
	// BACK TRADER

	// Goal: Make a training ground where you can test various algorithms, including ET-RNNs.
	// Also has to be able to be both used for training, testing and practical

	// start out by basic helper functions, like getting a timeseries for a single stock.
	// Timeseries object, returns everything vectorized (but only things that were requested, enum bitwise). Append to add new data queue wise.

	// Use queues a lot to implement continuious profiling.
	// Basically update old timeseries with new spot

	// Do testing frame based (which is why the queueing is important)
	// simd?

	// include portfolio management
	
	jv::ArenaCreateInfo arenaCreateInfo{};
	arenaCreateInfo.alloc = Alloc;
	arenaCreateInfo.free = Free;
	auto arena = jv::Arena::Create(arenaCreateInfo);
	auto tempArena = jv::Arena::Create(arenaCreateInfo);

	jv::bt::Tracker tracker{};
	tracker.Init();

	const auto str = tracker.GetData(tempArena, "AAPL");
	const auto timeSeries = tracker.ConvertDataToTimeSeries(arena, str);
	const auto subSeries = jv::bt::Tracker::GetTimeSeriesSubSet(arena, timeSeries, 120, 96);
	jv::bt::Tracker::Draw(subSeries);
	return 0;
}
