#pragma once

namespace jv::bt
{
	struct PortfolioStock final
	{
		const char* symbol;
		uint32_t count;
	};

	struct Portfolio final
	{
		uint64_t scope;
		Array<PortfolioStock> stocks;
		float liquidity;

		[[nodiscard]] static Portfolio Create(Arena& arena, const char** symbols, uint32_t length);
		static void Destroy(Arena& arena, const Portfolio& portfolio);
	};
}