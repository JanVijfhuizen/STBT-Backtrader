#include "pch.h"
#include "BackTrader.h"

#include "JLib/Arena.h"
#include "JLib/ArrayUtils.h"

namespace jv::bt
{
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
