#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	class MI_Symbols final : public MI_Main
	{
	public:
		Array<TimeSeries> timeSeries;
		Array<glm::vec4> timeSeriesColors;
		Array<std::string> names;
		Array<bool> enabled;
		char nameBuffer[6];
		uint32_t index;

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
		static void RenderSymbolData(STBT& stbt);
		void TryRenderSymbol(STBT& stbt);
	};
}