#include "pch.h"

#include "BackTrader.h"
#include "JLib/ArrayUtils.h"
#include "JLib/Math.h"
#include "JLib/Queue.h"
#include "NNet.h"
#include <NNetUtils.h>
#include "GeneticAlgorithm.h";
#include "FPFNTester.h"
#include <Renderer.h>
#include <STBT.h>

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

	auto nnet = reinterpret_cast<jv::ai::NNet*>(userPtr);
	float input[3]{ ma, momentum / 10, trend };
	bool output[2];
	Propagate(*nnet, input, output);

	if (output[0])
	{
		if (portfolio.liquidity - 100 > stock.close[offset])
		{
			call.amount = (portfolio.liquidity - 100) / stock.close[offset];
			call.type = jv::bt::CallType::Buy;
			call.symbolId = 0;
			calls.Add() = call;
		}
	}
	else if (output[1])
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

[[nodiscard]] float RatingFunc(jv::ai::NNet& nnet, void* userPtr, jv::Arena& arena, jv::Arena& tempArena)
{
	jv::bt::TestInfo testInfo{};
	testInfo.bot = StockAlgorithm;
	testInfo.userPtr = &nnet;
	testInfo.warmup = 100;
	auto bte = reinterpret_cast<jv::bt::BackTraderEnvironment*>(userPtr);
	float rating = bte->backTrader.RunTestEpochs(arena, tempArena, testInfo);
	return rating;
}

[[nodiscard]] float TestRatingFunc(jv::ai::NNet& nnet, void* userPtr, jv::Arena& arena, jv::Arena& tempArena)
{
	jv::ai::FPFNTester tester{};

	for (uint32_t i = 0; i < 1000; i++)
	{
		float input[10];
		input[0] = (sin(static_cast<float>(i) / 10) + 1) / 2;
		input[1] = (cos(static_cast<float>(i) / 10) + 1) / 2;
		input[2] = abs(sin(static_cast<float>(i) / 14));
		input[3] = i % 2;
		for (uint32_t j = 0; j < 6; j++)
		{
			// Nonsense inputs.
			input[4 + j] = static_cast<float>(rand() % 1000) / 1000;
		}
		bool output;
		Propagate(nnet, input, &output);
		if (i > 500)
			tester.AddResult(output, input[0] < sin(static_cast<float>(i + 5) / 10) &&
				(input[3] > 0.1f ? input[0] : 1.f - input[0]) > cos(static_cast<float>(i + 12) / 7) * input[2]);
	}

	return tester.GetRating();
}

int main()
{
	srand(time(nullptr));

	auto stui = jv::ai::CreateSTBT();

	while (!stui.Update())
		continue;
	return 0;

	jv::bt::BackTraderEnvironment bte;

	{
		const char* symbols[4];
		symbols[0] = "AAPL";
		symbols[1] = "AMZN";
		symbols[2] = "TSLA";
		symbols[3] = "EA";
		bte = jv::bt::CreateBTE(symbols, 4, 1e-3);
	}

	// Sin test.
	/*
	{
		jv::ai::Mutations mutations{};
		mutations.threshold.chance = .2;
		mutations.weight.chance = .2;
		mutations.decay.chance = .2;
		mutations.newNodeChance = .5;
		mutations.newWeightChance = .5;

		jv::ai::GeneticAlgorithmRunInfo runInfo{};
		runInfo.inputSize = 10;
		runInfo.outputSize = 1;
		runInfo.ratingFunc = TestRatingFunc;
		runInfo.mutations = mutations;
		const auto res = jv::ai::RunGeneticAlgorithm(runInfo, bte.arena, bte.tempArena);
		return 0;
	}
	*/

	uint32_t globalInnovationId = 0;
	jv::ai::NNetCreateInfo nnetCreateInfo{};
	nnetCreateInfo.inputSize = 4;
	nnetCreateInfo.neuronCapacity = 512;
	nnetCreateInfo.weightCapacity = 512;
	nnetCreateInfo.outputSize = 2;
	auto nnet = jv::ai::CreateNNet(nnetCreateInfo, bte.arena);
	auto ioLayers = Init(nnet, jv::ai::InitType::random, globalInnovationId);
	ConnectIO(nnet, jv::ai::InitType::random, globalInnovationId);

	auto nnetCpy = jv::ai::CreateNNet(nnetCreateInfo, bte.arena);

	jv::bt::TimeSeries timeSeries = bte.backTrader.world.timeSeries[0];
	//jv::bt::Tracker::Debug(timeSeries.close, 30, true);
	//jv::bt::Tracker::DebugCandles(timeSeries, 0, 400);

	// test
	{
		const uint32_t LENGTH = 20;
		jv::gr::GraphPoint points[LENGTH];
		float t = 0;

		while (!stui.renderer.Render())
		{
			t += .01;

			for (uint32_t i = 0; i < LENGTH; i++)
			{
				points[i].open = timeSeries.open[i + static_cast<int>(t)];
				points[i].close = timeSeries.close[i + static_cast<int>(t)];
				points[i].high = timeSeries.high[i + static_cast<int>(t)];
				points[i].low = timeSeries.low[i + static_cast<int>(t)];
			}
			stui.renderer.DrawGraph({ -.5, 0 }, glm::vec2(stui.renderer.GetAspectRatio(), 1), points, LENGTH, jv::gr::GraphType::line, false, true);
			stui.renderer.DrawGraph({ .5, 0 }, glm::vec2(stui.renderer.GetAspectRatio(), 1), points, LENGTH, jv::gr::GraphType::candle, false, true);
		}

		DestroySTBT(stui);
		return 0;
	}

	jv::bt::TestInfo testInfo{};
	testInfo.bot = StockAlgorithm;
	testInfo.userPtr = &nnetCpy;
	testInfo.warmup = 500;

	jv::ai::Mutations mutations{};
	mutations.threshold.chance = .2;
	mutations.weight.chance = .2;
	mutations.newNodeChance = .5;
	mutations.newWeightChance = .5;

	jv::ai::GeneticAlgorithmRunInfo runInfo{};
	runInfo.inputSize = 4;
	runInfo.outputSize = 2;
	runInfo.userPtr = &bte;
	runInfo.ratingFunc = RatingFunc;
	runInfo.mutations = mutations;

	// temp.
	runInfo.arrivals = 2;
	runInfo.survivors = 8;
	runInfo.width = 20;
	runInfo.epochs = 1000;

	const auto res = jv::ai::RunGeneticAlgorithm(runInfo, bte.arena, bte.tempArena);

	/*
	float highestScore = 0;
	for (size_t i = 0; i < 1000; i++)
	{
		Copy(nnet, nnetCpy);
		Mutate(nnetCpy, mutations, globalInnovationId);

		Clean(nnetCpy);
		const auto ret = bte.backTrader.RunTestEpochs(bte.arena, bte.tempArena, testInfo);

		std::cout << "e" << i << ".";
		if (ret > highestScore)
		{
			highestScore = ret;
			Copy(nnetCpy, nnet);
			std::cout << std::endl << std::endl << highestScore * 100 << "%" << std::endl << std::endl;
		}
	}
	*/

	//bte.backTrader.PrintAdvice(bte.arena, bte.tempArena, StockAlgorithm, "jan", true, &nnet);
	return 0;
}

/*
	static void LoadScripts(MI_Backtrader& bt, STBT& stbt)
	{
		stbt.arena.DestroyScope(bt.subScope);

		std::string path("Scripts/");
		std::string ext(".lua");

		uint32_t length = 0;
		for (auto& p : std::filesystem::recursive_directory_iterator(path))
			if (p.path().extension() == ext)
				++length;

		auto arr = jv::CreateArray<std::string>(stbt.arena, length);

		length = 0;
		for (auto& p : std::filesystem::recursive_directory_iterator(path))
		{
			if (p.path().extension() == ext)
				arr[length++] = p.path().stem().string();
		}

		bt.scripts = arr;
	}
*/