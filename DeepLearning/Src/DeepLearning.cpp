#include "pch.h"

#include <random>
#include "BackTrader.h"
#include "WebSocket.h"
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

	{
		jv::bt::Date date{};
		date.SetToToday();
		date.Adjust(-120);

		jv::bt::WebSocket webSocket{};
		webSocket.Init();

		std::cout << webSocket.GetData(tempArena, "AAPL", date) << std::endl;
		return 0;
	}

	jv::bt::Date date{};
	date.SetToToday();
	date.Adjust(-120);

	jv::bt::Init();
	const auto gspc = jv::bt::AddQuote("^GSPC");
	Explore(gspc, date, 120);
	
	auto timeline = jv::bt::Timeline::Create(arena, 40);
	timeline.Fill(tempArena, date, gspc);
	for (int i = 0; i < 20; ++i)
		timeline.Next(tempArena, gspc);

	timeline.Draw();

	std::cin.get();

	jv::bt::Shutdown();
	return 0;
}
