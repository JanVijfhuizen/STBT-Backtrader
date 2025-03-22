#pragma once
#include <JLib/Menu.h>
#include "STBT.h"

namespace jv::bt
{
	class MI_Main : public MenuItem<STBT>
	{
	public:
		bool Update(STBT& stbt, uint32_t& index) final override;
		virtual bool DrawMainMenu(STBT& stbt, uint32_t& index) = 0;
		virtual bool DrawSubMenu(STBT& stbt, uint32_t& index);
		virtual bool DrawFree(STBT& stbt, uint32_t& index);
		virtual const char* GetMenuTitle() = 0;
		virtual const char* GetSubMenuTitle();
		virtual const char* GetDescription() = 0;

		static void DrawTopRightWindow(const char* name, bool large = false, bool transparent = false);
		static void DrawBottomRightWindow(const char* name);

		static void TryDrawTutorialText(STBT& stbt, const char* text);
	};
}