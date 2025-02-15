#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	class MI_Backtrader final : public MI_Main
	{
	public:
		void Load(STBT& stbt) override;
		bool DrawMainMenu(STBT& stbt, uint32_t& index);
		bool DrawSubMenu(STBT& stbt, uint32_t& index);
		const char* GetMenuTitle();
		const char* GetSubMenuTitle();
		const char* GetDescription();
		void Unload(STBT& stbt) override;
	};
}