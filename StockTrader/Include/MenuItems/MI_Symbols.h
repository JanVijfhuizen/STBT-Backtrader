#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	struct SymbolGraphDrawInfo final
	{
		TimeSeries* timeSeries;
		std::string* names;
		bool* enabled;
		glm::vec4* colors;
		uint32_t length;
		bool normalizeGraph;
		bool reverse = false;

		// Reference.
		uint32_t* symbolIndex;
	};

	struct SymbolsDataDrawInfo final
	{
		TimeSeries* timeSeries;
		std::string* names;
		bool* enabled;
		glm::vec4* colors;
		uint32_t length;

		// References.
		uint32_t* symbolIndex;
		bool* normalizeGraph;
	};

	class MI_Symbols final : public MI_Main
	{
	public:
		// Timeseries of the current loaded in stock.
		Array<TimeSeries> timeSeries;
		// Names of all known symbols in scope.
		Array<std::string> names;
		// Whether or not said symbols are enabled for the backtrader.
		Array<bool> enabled;
		// Graph line colors.
		Array<glm::vec4> colors;
		// Current symbol index.
		uint32_t symbolIndex = -1;
		// Normalize graph on the screen.
		bool normalizeGraph = true;

		char nameBuffer[10];

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
		[[nodiscard]] static Array<gr::GraphPoint> RenderSymbolGraph(STBT& stbt, const SymbolGraphDrawInfo& drawInfo);
		static void RenderSymbolData(STBT& stbt, const SymbolsDataDrawInfo& drawInfo);
	};
}