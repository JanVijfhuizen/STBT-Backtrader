#include "pch.h"

#include <numeric>
#include <random>

#include <quote.hpp>
#include <ctime>

#include "Timeline.h"
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

	Gnuplot gp("\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\"");
	Quote* snp500 = new Quote("^GSPC");

	jv::ArenaCreateInfo arenaCreateInfo{};
	arenaCreateInfo.alloc = Alloc;
	arenaCreateInfo.free = Free;
	auto arena = jv::Arena::Create(arenaCreateInfo);
	auto tempArena = jv::Arena::Create(arenaCreateInfo);

	jv::Date date{};
	date.SetToToday();
	date.Adjust(-120);

	snp500->getHistoricalSpots("2023-01-01", "2024-05-14", "1d");

	auto timeline = CreateTimeline(arena, 40);
	timeline.Fill(tempArena, date, snp500);
	for (int i = 0; i < 20; ++i)
		timeline.Next(tempArena, snp500);

	std::vector<double> v;
	for (auto d : timeline)
	{
		v.push_back(d);
	}
	
	gp << "set title 'SNP 500'\n";
	gp << "plot '-' with lines title 'v'\n";
	//gp.send(v);
	gp.send(v);

	std::cin.get();

	// Free memory
	delete snp500;
	return 0;
}
