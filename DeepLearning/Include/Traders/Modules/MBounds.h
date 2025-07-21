#pragma once
#include <Traders/NNetTrader.h>

namespace jv
{
	struct NNetTraderModBounds final
	{
		uint32_t maLen = 20;
		uint32_t stdLen = 20;
		float stdMul = 2;

		float* ma;
	};

	[[nodiscard]] NNetTraderMod NNetGetTraderModBounds(NNetTraderModBounds& out);
}
