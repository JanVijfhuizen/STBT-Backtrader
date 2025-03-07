#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	class MI_Symbols final : public MI_Main
	{
	public:
		Array<TimeSeries> timeSeries;
		Array<std::string> names;
		Array<bool> enabled;
		char nameBuffer[6];
		uint32_t symbolIndex = -1;
		bool normalizeGraph = true;

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

		static void DrawTopRightWindow(const char* name, bool large = false, bool transparent = false);
		static void DrawBottomRightWindow(const char* name, bool popup = false);
	};
}