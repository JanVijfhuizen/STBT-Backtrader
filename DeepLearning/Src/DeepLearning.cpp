#include "pch.h"

#include "BackTrader.h"
#include "JLib/Arena.h"
#include "JLib/ArrayUtils.h"

void* Alloc(const uint32_t size)
{
	return malloc(size);
}
void Free(void* ptr)
{
	return free(ptr);
}

void StockAlgorithm(const jv::bt::World& world, const jv::bt::Portfolio& portfolio,
	jv::Vector<jv::bt::Call>& calls, const uint32_t offset, void* userPtr)
{
	const auto c = rand() % 2;
	const auto d = rand() % world.timeSeries.length;

	jv::bt::Call call{};

	const auto& stock = world.timeSeries[d];

	// buy
	if(c == 0)
	{
		if(portfolio.liquidity + 10 > stock.open[offset])
		{
			call.amount = 1;
			call.type = jv::bt::CallType::Buy;
			call.symbolId = d;
			calls.Add() = call;
		}
	}
	// sell
	else
	{
		if (portfolio.liquidity > 10 && portfolio.stocks[d] > 0)
		{
			call.amount = 1;
			call.type = jv::bt::CallType::Sell;
			call.symbolId = d;
			calls.Add() = call;
		}
	}
}

int main()
{
	srand(time(nullptr));

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

	const auto symbols = jv::CreateArray<const char*>(arena, 4);
	symbols[0] = "AAPL";
	symbols[1] = "AMZN";
	symbols[2] = "TSLA";
	symbols[3] = "EA";

	const auto backTrader = jv::bt::CreateBackTrader(arena, tempArena, symbols, .01f);

	auto portfolio = CreatePortfolio(arena, backTrader);
	portfolio.liquidity = 2000;
	portfolio.stocks[0] = 0;
	portfolio.stocks[1] = 0;
	portfolio.stocks[2] = 0;
	portfolio.stocks[3] = 0;

	SavePortfolio("jan", portfolio);
	portfolio = LoadPortfolio(arena, backTrader, "jan");

	jv::bt::RunInfo runInfo{};
	runInfo.offset = 30;
	runInfo.length = 30;
	runInfo.func = StockAlgorithm;
	runInfo.debug = true;
	jv::bt::Log log;
	const auto endPortfolio = backTrader.Run(arena, tempArena, portfolio, log, runInfo);
	std::cout << backTrader.GetLiquidity(endPortfolio, 0) - backTrader.GetLiquidity(portfolio, 30);

	return 0;
}
