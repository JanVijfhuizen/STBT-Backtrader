#include "pch.h"

#include "BackTrader.h"
#include "JLib/ArrayUtils.h"

void StockAlgorithm(jv::Arena& tempArena, const jv::bt::World& world, const jv::bt::Portfolio& portfolio,
	jv::Vector<jv::bt::Call>& calls, const uint32_t offset, void* userPtr)
{
	const auto c = rand() % 2;
	const auto d = rand() % world.timeSeries.length;

	jv::bt::Call call{};

	const auto& stock = world.timeSeries[d];

	// buy
	if(c == 0)
	{
		if(portfolio.liquidity - 10 > stock.close[offset])
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

	jv::bt::BackTraderEnvironment bte;

	{
		const char* symbols[4];
		symbols[0] = "AAPL";
		symbols[1] = "AMZN";
		symbols[2] = "TSLA";
		symbols[3] = "EA";
		bte = jv::bt::CreateBTE(symbols, 4, 1e-3f);
	}
	
	jv::bt::TestInfo testInfo{};
	testInfo.bot = StockAlgorithm;
	
	const auto ret = bte.backTrader.RunTestEpochs(bte.arena, bte.tempArena, testInfo);
	std::cout << ret << std::endl;

	bte.backTrader.PrintAdvice(bte.arena, bte.tempArena, StockAlgorithm, "jan", true);
	return 0;
}
