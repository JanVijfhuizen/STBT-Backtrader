#include "pch.h"
#include "MenuItems/MI_MainMenu.h"

namespace jv::bt
{
	enum MenuIndex
	{
		miMain,
		miSymbols,
		miBacktrader,
		miLicense
	};

	bool MI_MainMenu::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		if (ImGui::Button("Symbols"))
			index = miSymbols;
		if (ImGui::Button("Backtrader"))
			index = miBacktrader;
		if (ImGui::Button("Licensing"))
			index = miLicense;
		if (ImGui::Button("Exit"))
			return true;
		return false;
	}

	const char* MI_MainMenu::GetMenuTitle()
	{
		return "Main Menu";
	}

	const char* MI_MainMenu::GetDescription()
	{
		return "This tool can be used to \ntrain, test and use stock \ntrade algorithms in \nrealtime.";
	}
}