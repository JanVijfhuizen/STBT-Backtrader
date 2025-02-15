#pragma once
#include <JLib/Menu.h>
#include "STBT.h"

namespace jv::bt
{
	class MI_Main : public MenuItem<STBT>
	{
	public:
		bool Update(STBT& stbt, uint32_t& index) final override;
		virtual bool DrawMainMenu(uint32_t& index) = 0;
		virtual bool DrawSubMenu(uint32_t& index);
		virtual const char* GetMenuTitle() = 0;
		virtual const char* GetSubMenuTitle() = 0;
		virtual const char* GetDescription() = 0;
	};
}