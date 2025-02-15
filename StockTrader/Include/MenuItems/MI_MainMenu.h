#pragma once
#include "MI_Main.h"

namespace jv::bt
{
	class MI_MainMenu final : public MI_Main
	{
	public:
		bool DrawMainMenu(STBT& stbt, uint32_t& index);
		const char* GetMenuTitle();
		const char* GetDescription();
	};
}
