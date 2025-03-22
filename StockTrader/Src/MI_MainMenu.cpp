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
		TryDrawTutorialText(stbt, "Adds tooltips.");
		ImGui::Checkbox("Tutorial Mode", &stbt.enableTutorialMode);
		TryDrawTutorialText(stbt, "Look, load and enable/\ndisable symbol stock data.");
		if (ImGui::Button("Symbols"))
			index = miSymbols;
		TryDrawTutorialText(stbt, "Test and debug stock\ntrading algorithms.");
		if (ImGui::Button("Backtrader"))
			index = miBacktrader;
		TryDrawTutorialText(stbt, "External licensing keys\nrequired to load in\nstock data.");
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