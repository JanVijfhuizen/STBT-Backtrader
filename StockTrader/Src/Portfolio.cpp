#include "pch.h"
#include "Portfolio.h"
#include <Jlib/ArrayUtils.h>

namespace jv::bt
{
	Portfolio Portfolio::Create(Arena& arena, const char** symbols, const uint32_t length)
	{
		Portfolio portfolio{};
		portfolio.stocks = CreateArray<PortfolioStock>(arena, length);
		for (uint32_t i = 0; i < length; i++)
		{
			auto& stock = portfolio.stocks[i];
			stock.symbol = symbols[i];
			stock.count = 0;
		}
		return portfolio;
	}

	void Portfolio::Destroy(Arena& arena, const Portfolio& portfolio)
	{
		arena.DestroyScope(portfolio.scope);
	}
}