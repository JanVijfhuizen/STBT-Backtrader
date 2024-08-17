#include "pch.h"

#include "BackTrader.h"
#include "JLib/ArrayUtils.h"
#include "JLib/Math.h"
#include "JLib/Queue.h"

void StockAlgorithm(jv::Arena& tempArena, const jv::bt::World& world, const jv::bt::Portfolio& portfolio,
	jv::Vector<jv::bt::Call>& calls, const uint32_t offset, void* userPtr)
{
	jv::bt::Call call{};

	const auto& stock = world.timeSeries[0];

	if(stock.open[offset] > stock.close[offset])
	{
		if (portfolio.liquidity - 100 > stock.close[offset])
		{
			call.amount = (portfolio.liquidity - 100) / stock.close[offset];
			call.type = jv::bt::CallType::Buy;
			call.symbolId = 0;
			calls.Add() = call;
		}
	}
	else
	{
		if (portfolio.liquidity > 10 && portfolio.stocks[0] > 0)
		{
			call.amount = portfolio.stocks[0];
			call.type = jv::bt::CallType::Sell;
			call.symbolId = 0;
			calls.Add() = call;
		}
	}
	return;

	const float ma10 = jv::bt::GetMA(stock.close, offset, 10);
	const float ma100 = jv::bt::GetMA(stock.close, offset, 100);

	float momentum = 0;
	for (uint32_t i = 0; i < 3; ++i)
	{
		const auto ceiling = jv::Max<float>(stock.close[offset + i], stock.open[offset + i]);
		const auto floor = jv::Min<float>(stock.close[offset + i], stock.open[offset + i]);

		momentum += stock.high[offset + i] - ceiling;
		momentum -= floor - stock.low[offset + i];
		//momentum += stock.close[offset + i] - stock.open[offset + i];
	}

	if(momentum > 0)
	{
		if (portfolio.liquidity - 100 > stock.close[offset])
		{
			call.amount = (portfolio.liquidity - 100) / stock.close[offset];
			call.type = jv::bt::CallType::Buy;
			call.symbolId = 0;
			calls.Add() = call;
		}
	}
	else
	{
		if (portfolio.liquidity > 10 && portfolio.stocks[0] > 0)
		{
			call.amount = portfolio.stocks[0];
			call.type = jv::bt::CallType::Sell;
			call.symbolId = 0;
			calls.Add() = call;
		}
	}

	return;

	// buy
	if(ma10 < ma100 * .98)
	{
		if (portfolio.liquidity - 100 > stock.close[offset])
		{
			call.amount = (portfolio.liquidity - 100) / stock.close[offset];
			call.type = jv::bt::CallType::Buy;
			call.symbolId = 0;
			calls.Add() = call;
		}
	}
	// sell
	if (ma10 > ma100 * 1.02)
	{
		if (portfolio.liquidity > 10 && portfolio.stocks[0] > 0)
		{
			call.amount = portfolio.stocks[0];
			call.type = jv::bt::CallType::Sell;
			call.symbolId = 0;
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

	jv::bt::TimeSeries timeSeries = bte.backTrader.world.timeSeries[0];
	//jv::bt::Tracker::Debug(timeSeries.close, 30, true);
	jv::bt::Tracker::DebugCandles(timeSeries, 0, 400);
	
	jv::bt::TestInfo testInfo{};
	testInfo.bot = StockAlgorithm;
	
	const auto ret = bte.backTrader.RunTestEpochs(bte.arena, bte.tempArena, testInfo);
	std::cout << ret * 100 << "%" << std::endl;

	bte.backTrader.PrintAdvice(bte.arena, bte.tempArena, StockAlgorithm, "jan", true);
	return 0;
}
