#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	class MI_Symbols final : public MI_Main
	{
	public:
		void Load(STBT& stbt) override;
		bool DrawMainMenu(uint32_t& index);
		bool DrawSubMenu(uint32_t& index);
		const char* GetMenuTitle();
		const char* GetSubMenuTitle();
		const char* GetDescription();
		void Unload(STBT& stbt) override;

		static void LoadSymbols(STBT& stbt);
		static void LoadEnabledSymbols(STBT& stbt);
	};
}