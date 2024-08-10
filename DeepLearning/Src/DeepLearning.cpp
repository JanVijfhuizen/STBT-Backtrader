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
		if(portfolio.liquidity - 10 > stock.open[offset])
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

	jv::ArenaCreateInfo arenaCreateInfo{};
	arenaCreateInfo.alloc = Alloc;
	arenaCreateInfo.free = Free;
	auto arena = jv::Arena::Create(arenaCreateInfo);
	auto tempArena = jv::Arena::Create(arenaCreateInfo);
	
	const auto symbols = jv::CreateArray<const char*>(arena, 4);
	symbols[0] = "AAPL";
	symbols[1] = "AMZN";
	symbols[2] = "TSLA";
	symbols[3] = "EA";

	const auto backTrader = jv::bt::CreateBackTrader(arena, tempArena, symbols, .001f);
	
	jv::bt::TestInfo testInfo{};
	testInfo.bot = StockAlgorithm;
	
	const auto ret = backTrader.RunTestEpochs(arena, tempArena, testInfo);
	std::cout << ret << std::endl;
	return 0;
}
