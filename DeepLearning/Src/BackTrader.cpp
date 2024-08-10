#include "pch.h"
#include "BackTrader.h"

#include "JLib/Arena.h"
#include "JLib/ArrayUtils.h"
#include "JLib/VectorUtils.h"

namespace jv::bt
{
	void Portfolio::Copy(const Portfolio& other)
	{
		liquidity = other.liquidity;
		memcpy(stocks.ptr, other.stocks.ptr, sizeof(uint32_t) * stocks.length);
	}

	Portfolio BackTrader::Run(Arena& arena, Arena& tempArena, const Portfolio& portfolio, Array<Array<Call>>& outLog, const RunInfo& runInfo) const
	{
		auto cpyPortfolio = CreatePortfolio(arena, *this);
		cpyPortfolio.Copy(portfolio);

		auto calls = CreateVector<Call>(tempArena, portfolio.stocks.length);
		outLog = CreateArray<Array<Call>>(arena, runInfo.length);

		for (uint32_t i = 0; i < runInfo.length; ++i)
		{
			const uint32_t index = runInfo.offset - i;
			calls.Clear();
			runInfo.func(world, cpyPortfolio, calls, index, runInfo.userPtr);
			const auto arr = CreateArray<Call>(arena, calls.count);
			memcpy(arr.ptr, calls.ptr, sizeof(Call) * calls.count);
			outLog[i] = arr;
		}
		
		return cpyPortfolio;
	}

	float BackTrader::GetLiquidity(const Portfolio& portfolio, const uint32_t offset) const
	{
		float liquidity = portfolio.liquidity;
		for (uint32_t i = 0; i < portfolio.stocks.length; ++i)
			if(portfolio.stocks[i] > 0)
				liquidity += world.timeSeries[i].close[offset] * portfolio.stocks[i];
		return liquidity;
	}

	Portfolio CreatePortfolio(Arena& arena, const BackTrader& backTrader)
	{
		Portfolio portfolio{};
		portfolio.stocks = CreateArray<uint32_t>(arena, backTrader.world.timeSeries.length);
		return portfolio;
	}

	std::string GetPortfolioPath(const char* name)
	{
		const std::string postfix = ".port";
		return name + postfix;
	}

	Portfolio LoadPortfolio(Arena& arena, const BackTrader& backTrader, const char* name)
	{
		const std::string path = GetPortfolioPath(name);
		std::ifstream fin(path);
		std::string line;
		assert(fin.good());

		auto portfolio = CreatePortfolio(arena, backTrader);

		getline(fin, line);
		portfolio.liquidity = std::stof(line);

		uint32_t i = 0;
		while(getline(fin, line))
			portfolio.stocks[i++] = std::stoi(line);

		return portfolio;
	}

	void DestroyPortfolio(Arena& arena, const Portfolio& portfolio)
	{
		DestroyArray<uint32_t>(arena, portfolio.stocks);
	}

	void SavePortfolio(const char* name, const Portfolio& portfolio)
	{
		const std::string path = GetPortfolioPath(name);
		std::ofstream fout(path);

		fout << portfolio.liquidity << std::endl;
		for (const auto& stock : portfolio.stocks)
			fout << stock << std::endl;
	}

	BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, const Array<const char*> symbols, const float fee)
	{
		BackTrader backTrader{};
		backTrader.scope = arena.CreateScope();
		backTrader.world = {};
		backTrader.world.timeSeries = CreateArray<TimeSeries>(arena, symbols.length);
		backTrader.world.fee = fee;
		backTrader.tracker = {};

		auto& tracker = backTrader.tracker;

		for (uint32_t i = 0; i < symbols.length; ++i)
		{
			const auto tempScope = tempArena.CreateScope();
			const auto str = tracker.GetData(tempArena, symbols[i]);
			const auto timeSeries = tracker.ConvertDataToTimeSeries(arena, str);
			backTrader.world.timeSeries[i] = timeSeries;
			tempArena.DestroyScope(tempScope);
		}
		
		return backTrader;
	}

	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena)
	{
		arena.DestroyScope(backTrader.scope);
	}
}
