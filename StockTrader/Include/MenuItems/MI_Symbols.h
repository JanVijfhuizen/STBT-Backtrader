#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	class MI_Symbols final : public MI_Main
	{
	public:
		// Timeseries of the current loaded in stock.
		Array<TimeSeries> timeSeries;
		// Names of all known symbols in scope.
		Array<std::string> names;
		// Whether or not said symbols are enabled for the backtrader.
		Array<bool> enabled;
		// Current symbol index.
		uint32_t symbolIndex = -1;
		// Normalize graph on the screen.
		bool normalizeGraph = true;

		char nameBuffer[6];

		void Load(STBT& stbt) override;
		bool DrawMainMenu(STBT& stbt, uint32_t& index);
		bool DrawSubMenu(STBT& stbt, uint32_t& index);
		bool DrawFree(STBT& stbt, uint32_t& index);
		const char* GetMenuTitle();
		const char* GetSubMenuTitle();
		const char* GetDescription();
		void Unload(STBT& stbt) override;

		static void SaveEnabledSymbols(STBT& stbt, const Array<bool>& enabled);
		static void SaveOrCreateEnabledSymbols(STBT& stbt, const Array<std::string>& names, Array<bool>& enabled);

		[[nodiscard]] static Array<std::string> GetSymbolNames(STBT& stbt);
		[[nodiscard]] static TimeSeries LoadSymbol(STBT& stbt, const uint32_t i, const Array<std::string>& names, uint32_t& index);
		[[nodiscard]] static Array<bool> GetEnabled(STBT& stbt, const Array<std::string>& names, Array<bool>& enabled);
		[[nodiscard]] static Array<gr::GraphPoint> RenderSymbolData(STBT& stbt, Array<TimeSeries>& timeSeries, 
			const Array<std::string>& names, const Array<bool>& enabled, uint32_t& symbolIndex, const bool normalizeGraph);
		static void TryRenderSymbol(STBT& stbt, Array<TimeSeries>& timeSeries,
			const Array<std::string>& names, const Array<bool>& enabled, uint32_t& symbolIndex, bool& normalizeGraph);
	};
}