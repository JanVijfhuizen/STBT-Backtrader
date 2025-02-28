#pragma once
#include "MI_Main.h"
#include <Portfolio.h>

namespace jv::bt
{
	class MI_Backtrader final : public MI_Main
	{
	public:
		Array<TimeSeries> timeSeries;
		Array<std::string> names;
		Array<bool> enabled;
		Array<char*> buffers;
		Portfolio portfolio;

		uint64_t subScope;
		uint32_t subIndex;
		uint32_t symbolIndex;
		bool normalizeGraph;

		uint32_t algoIndex;
		bool randomizeDate;
		char buffBuffer[8];
		char lengthBuffer[8];
		char feeBuffer[8];
		char runCountBuffer[8];
		bool log;
		bool running;

		STBTTrade* trades;
		STBTScope runScope;
		Portfolio runPortfolio;
		uint32_t runIndex;
		uint32_t runDayIndex;
		uint32_t runOffset;

		void Load(STBT& stbt) override;
		bool DrawMainMenu(STBT& stbt, uint32_t& index);
		bool DrawSubMenu(STBT& stbt, uint32_t& index);
		bool DrawFree(STBT& stbt, uint32_t& index);
		const char* GetMenuTitle();
		const char* GetSubMenuTitle();
		const char* GetDescription();
		void Unload(STBT& stbt) override;
	};
}