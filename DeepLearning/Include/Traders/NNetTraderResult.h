#pragma once
#include <Traders/NNetTrader.h>

namespace jv
{
	struct NNetTraderResult final : NNetTrader
	{
		[[nodiscard]] static NNetTraderResult Create(Arena& arena, Arena& tempArena, const NNetTraderCreateInfo& info);
		static void Destroy(Arena& arena, NNetTraderResult& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}

