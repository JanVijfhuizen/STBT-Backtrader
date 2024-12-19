#include "pch.h"

#include "BackTrader.h"
#include "JLib/ArrayUtils.h"
#include "JLib/Math.h"
#include "JLib/Queue.h"
#include "NNet.h"
#include <NNetUtils.h>

[[nodiscard]] float GetMAValue(const jv::bt::TimeSeries& stock, const uint32_t offset, void* userPtr)
{
	const float maShort = jv::bt::GetMA(stock.close, offset, 10);
	const float maLong = jv::bt::GetMA(stock.close, offset, 100);

	return maShort / maLong - 1.f;
}

[[nodiscard]] float GetMomentumValue(const jv::bt::TimeSeries& stock, const uint32_t offset, void* userPtr)
{
	float momentum = 0;
	for (uint32_t i = 0; i < 3; ++i)
	{
		const auto ceiling = jv::Max<float>(stock.close[offset + i], stock.open[offset + i]);
		const auto floor = jv::Min<float>(stock.close[offset + i], stock.open[offset + i]);

		float f = stock.high[offset + i] - ceiling;
		f -= floor - stock.low[offset + i];
		momentum /= abs(stock.close[offset + i] - stock.open[offset + i]);
		momentum += f;
	}
	return momentum / 3;
}

[[nodiscard]] float GetTrendValue(const jv::bt::TimeSeries& stock, const uint32_t offset, void* userPtr)
{
	float trend = 0;
	for (uint32_t i = 0; i < 3; ++i)
		trend += stock.close[offset + i] - stock.open[offset + i];
	return trend / 3;
}

void StockAlgorithm(jv::Arena& tempArena, const jv::bt::World& world, const jv::bt::Portfolio& portfolio,
	jv::Vector<jv::bt::Call>& calls, const uint32_t offset, void* userPtr)
{
	jv::bt::Call call{};

	const auto& stock = world.timeSeries[0];
	const float ma = GetMAValue(stock, offset, userPtr);
	const float momentum = GetMomentumValue(stock, offset, userPtr);
	const float trend = GetTrendValue(stock, offset, userPtr);

	if(ma + momentum + trend > 0)
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
		bte = jv::bt::CreateBTE(symbols, 4, 1e-3);
	}

	jv::ai::NNetCreateInfo nnetCreateInfo{};
	nnetCreateInfo.inputSize = 4;
	nnetCreateInfo.neuronCapacity = 7 + 6 + 4;
	nnetCreateInfo.weightCapacity = 512;
	nnetCreateInfo.outputSize = 3;
	auto nnet = jv::ai::CreateNNet(nnetCreateInfo, bte.arena);
	auto ioLayers = Init(nnet, jv::ai::InitType::random);
	auto midLayer = AddLayer(nnet, 6, jv::ai::InitType::random);
	auto midLayer2 = AddLayer(nnet, 4, jv::ai::InitType::random);
	Connect(nnet, ioLayers.input, midLayer, jv::ai::InitType::random);
	Connect(nnet, midLayer, midLayer2, jv::ai::InitType::random);
	Connect(nnet, midLayer2, ioLayers.output, jv::ai::InitType::random);

	float input[4]{ .1, .2, .3, .4 };
	float output[3]{0, 0, 0};
	Propagate(nnet, input, output);

	jv::ai::Mutations mutations{};
	mutations.threshold.chance = .2;
	mutations.weight.chance = .2;
	auto nnetCpy = jv::ai::CreateNNet(nnetCreateInfo, bte.arena);
	float highestScore = 0;
	for (size_t i = 0; i < 1000; i++)
	{
		Copy(nnet, nnetCpy);
		Mutate(nnetCpy, mutations);
		Propagate(nnetCpy, input, output);
		float score = 0;
		for(auto& f : output)
			score += f;

		if (score > highestScore)
		{
			highestScore = score;
			Copy(nnetCpy, nnet);
			std::cout << score << std::endl;
		}
	}

	return 0;

	jv::bt::TimeSeries timeSeries = bte.backTrader.world.timeSeries[0];
	//jv::bt::Tracker::Debug(timeSeries.close, 30, true);
	//jv::bt::Tracker::DebugCandles(timeSeries, 0, 400);
	
	jv::bt::TestInfo testInfo{};
	testInfo.bot = StockAlgorithm;
	
	const auto ret = bte.backTrader.RunTestEpochs(bte.arena, bte.tempArena, testInfo);
	std::cout << ret * 100 << "%" << std::endl;

	bte.backTrader.PrintAdvice(bte.arena, bte.tempArena, StockAlgorithm, "jan", true);
	return 0;
}
