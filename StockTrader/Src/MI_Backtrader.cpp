#include "pch.h"
#include "MenuItems/MI_Backtrader.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>

namespace jv::bt
{
	void MI_Backtrader::Load(STBT& stbt)
	{
		MI_Symbols::LoadSymbols(stbt);
		uint32_t c = 0;
		for (uint32_t i = 0; i < stbt.loadedSymbols.length; i++)
			c += stbt.enabledSymbols[i];
		stbt.buffArr = CreateArray<char*>(stbt.arena, c + 1);
		for (uint32_t i = 0; i < stbt.buffArr.length; i++)
			stbt.buffArr[i] = stbt.arena.New<char>(16);
		stbt.timeSeriesArr = CreateArray<TimeSeries>(stbt.arena, c);

		uint32_t index = 0;
		for (size_t i = 0; i < stbt.enabledSymbols.length; i++)
		{
			if (!stbt.enabledSymbols[i])
				continue;
			stbt.timeSeriesArr[index++] = LoadSymbol(stbt, i);
		}

		LoadRandColors(stbt);
		stbt.subScope = stbt.arena.CreateScope();
		stbt.subMenuIndex = 0;
		if (c > 0)
			stbt.symbolIndex = 0;

		stbt.portfolio = CreateArray<uint32_t>(stbt.arena, index);
	}

	void MI_Backtrader::Update(STBT& stbt)
	{
	}

	void MI_Backtrader::Unload(STBT& stbt)
	{
	}
}