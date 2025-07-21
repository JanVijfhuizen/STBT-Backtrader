#include "pch.h"
#include "MenuItems/Mi_Main.h"

namespace jv::bt
{
	bool MI_Main::Update(STBT& stbt, uint32_t& index)
	{
		ImGui::Begin("Menu", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 0, 0 });
		ImGui::SetWindowSize({ MENU_RESOLUTION_LARGE.x, MENU_RESOLUTION_LARGE.y });

		ImGui::Text(GetMenuTitle());
		ImGui::Text(GetDescription());
		ImGui::Dummy({ 0, 20 });

		bool quit = DrawMainMenu(stbt, index);
		if (quit)
			return true;

		ImGui::End();

		if (auto subMenuTitle = GetSubMenuTitle())
		{
			ImGui::Begin(subMenuTitle, nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ MENU_WIDTH, 0 });
			ImGui::SetWindowSize({ MENU_RESOLUTION_LARGE.x, MENU_RESOLUTION_LARGE.y });
			quit = DrawSubMenu(stbt, index);
			if (quit)
				return true;
			ImGui::End();
		}

		return DrawFree(stbt, index);
	}

	bool MI_Main::DrawSubMenu(STBT& stbt, uint32_t& index)
	{
		return false;
	}

	bool MI_Main::DrawFree(STBT& stbt, uint32_t& index)
	{
		return false;
	}

	const char* MI_Main::GetSubMenuTitle()
	{
		return nullptr;
	}

	void MI_Main::DrawTopRightWindow(const char* name, const bool large, const bool transparent)
	{
		ImGuiWindowFlags FLAGS = 0;
		FLAGS |= ImGuiWindowFlags_NoBackground;
		FLAGS |= ImGuiWindowFlags_NoTitleBar;
		FLAGS |= ImGuiWindowFlags_NoInputs;
		FLAGS = transparent ? FLAGS : 0;

		ImGui::Begin(name, nullptr, WIN_FLAGS | FLAGS);
		ImGui::SetWindowPos({ 400, 0 });
		ImGui::SetWindowSize({ 400, static_cast<float>(large ? 500 : 80) });
	}

	void MI_Main::DrawBottomRightWindow(const char* name)
	{
		const uint32_t d = 10;
		const ImVec2 pos = { 400, 500 - d };
		const ImVec2 size = { 400, MENU_RESOLUTION_SMALL.y + d };

		ImGui::Begin(name, nullptr, WIN_FLAGS);
		ImGui::SetWindowPos(pos);
		ImGui::SetWindowSize(size);
	}
}