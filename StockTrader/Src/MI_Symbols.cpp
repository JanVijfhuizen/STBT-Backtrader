#include "pch.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>

namespace jv::bt
{
	static void SaveEnabledSymbols(STBT& stbt)
	{
		const std::string path = "Symbols/enabled.txt";
		std::ofstream fout(path);

		for (const auto enabled : stbt.enabledSymbols)
			fout << enabled << std::endl;
		fout.close();
	}

	static void SaveOrCreateEnabledSymbols(STBT& stbt)
	{
		if (stbt.enabledSymbols.length != stbt.loadedSymbols.length)
		{
			stbt.enabledSymbols = jv::CreateArray<bool>(stbt.arena, stbt.loadedSymbols.length);
			for (auto& b : stbt.enabledSymbols)
				b = true;
		}

		SaveEnabledSymbols(stbt);
	}

	void MI_Symbols::Load(STBT& stbt)
	{
		LoadSymbols(stbt);
		stbt.timeSeriesArr = CreateArray<TimeSeries>(stbt.arena, 1);
		LoadRandColors(stbt);
	}

	bool MI_Symbols::DrawMainMenu(uint32_t& index)
	{
		/*
		if (ImGui::Button("Enable All"))
			for (auto& b : enabledSymbols)
				b = true;
		if (ImGui::Button("Disable All"))
			for (auto& b : enabledSymbols)
				b = false;
		if (ImGui::Button("Save changes"))
			SaveOrCreateEnabledSymbols(*this);
		*/
		return false;
	}

	bool MI_Symbols::DrawSubMenu(uint32_t& index)
	{
		/*
		ImGui::Begin("List of symbols", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 200, 0 });
		ImGui::SetWindowSize({ 200, 400 });

		if (ImGui::Button("Add"))
		{
			const auto tempScope = tempArena.CreateScope();
			const auto c = tracker.GetData(tempArena, buffer, "Symbols/", license);
			if (c[0] == '{')
				output.Add() = "ERROR: Unable to download symbol data.";

			tempArena.DestroyScope(tempScope);

			uint32_t index = 0;
			std::string s{ buffer };
			if (s != "")
			{
				for (auto& symbol : loadedSymbols)
					index += symbol < s;

				auto arr = CreateArray<bool>(arena, enabledSymbols.length + 1);
				for (uint32_t i = 0; i < enabledSymbols.length; i++)
					arr[i + (i >= index)] = enabledSymbols[i];
				arr[index] = false;
				enabledSymbols = arr;
			}

			SaveEnabledSymbols(*this);
			LoadSymbolSubMenu(*this);
		}
		ImGui::SameLine();
		ImGui::InputText("#", buffer, 5, ImGuiInputTextFlags_CharsUppercase);

		for (uint32_t i = 0; i < loadedSymbols.length; i++)
		{
			ImGui::PushID(i);
			ImGui::Checkbox("", &enabledSymbols[i]);
			ImGui::PopID();
			ImGui::SameLine();

			const bool selected = symbolIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

			const auto symbol = loadedSymbols[i].c_str();
			if (ImGui::Button(symbol))
			{
				LoadSymbolSubMenu(*this);
				timeSeriesArr[0] = LoadSymbol(*this, i);
			}

			if (selected)
				ImGui::PopStyleColor();
		}

		ImGui::End();

		TryRenderSymbol(*this);
		*/
		return false;
	}

	const char* MI_Symbols::GetMenuTitle()
	{
		return "Symbols";
	}

	const char* MI_Symbols::GetSubMenuTitle()
	{
		return "List of Symbols";
	}

	const char* MI_Symbols::GetDescription()
	{
		return "Debug symbols, (un)load \nthem and add new ones.";
	}

	void MI_Symbols::Unload(STBT& stbt)
	{
	}

	void MI_Symbols::LoadSymbols(STBT& stbt)
	{
		stbt.symbolIndex = -1;

		std::string path("Symbols/");
		std::string ext(".sym");

		uint32_t length = 0;
		for (auto& p : std::filesystem::recursive_directory_iterator(path))
			if (p.path().extension() == ext)
				++length;

		auto arr = jv::CreateArray<std::string>(stbt.arena, length);

		length = 0;
		for (auto& p : std::filesystem::recursive_directory_iterator(path))
		{
			if (p.path().extension() == ext)
				arr[length++] = p.path().stem().string();
		}

		stbt.loadedSymbols = arr;
		LoadEnabledSymbols(stbt);
	}
	void MI_Symbols::LoadEnabledSymbols(STBT& stbt)
	{
		const std::string path = "Symbols/enabled.txt";
		std::ifstream fin(path);
		std::string line;

		if (!fin.good())
		{
			SaveOrCreateEnabledSymbols(stbt);
			return;
		}

		uint32_t length = 0;
		while (std::getline(fin, line))
			++length;

		if (length != stbt.loadedSymbols.length)
		{
			SaveOrCreateEnabledSymbols(stbt);
			return;
		}

		fin.clear();
		fin.seekg(0, std::ios::beg);

		auto arr = jv::CreateArray<bool>(stbt.arena, length);

		length = 0;
		while (std::getline(fin, line))
			arr[length++] = std::stoi(line);

		stbt.enabledSymbols = arr;
	}
}