#include "pch.h"
#include "MenuItems/MI_Backtrader.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>

namespace jv::bt
{
	enum BTMenuIndex
	{
		btmiPortfolio,
		btmiScripts
	};

	static void LoadScripts(MI_Backtrader& bt, STBT& stbt)
	{
		stbt.arena.DestroyScope(bt.subScope);

		std::string path("Scripts/");
		std::string ext(".lua");

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

		bt.scripts = arr;
	}

	void MI_Backtrader::Load(STBT& stbt)
	{
		const auto tempScope = stbt.tempArena.CreateScope();

		names = MI_Symbols::GetSymbolNames(stbt);
		enabled = MI_Symbols::GetEnabled(stbt, names, enabled);

		uint32_t c = 0;
		for (uint32_t i = 0; i < names.length; i++)
			c += enabled[i];

		buffers = CreateArray<char*>(stbt.arena, c + 1);
		for (uint32_t i = 0; i < buffers.length; i++)
			buffers[i] = stbt.arena.New<char>(16);
		timeSeries = CreateArray<TimeSeries>(stbt.arena, c);

		uint32_t index = 0;
		for (size_t i = 0; i < enabled.length; i++)
		{
			if (!enabled[i])
				continue;
			uint32_t _;
			timeSeries[index++] = MI_Symbols::LoadSymbol(stbt, i, names, _);
		}

		auto namesCharPtrs = CreateArray<const char*>(stbt.tempArena, names.length);
		for (uint32_t i = 0; i < names.length; i++)
			namesCharPtrs[i] = names[i].c_str();

		portfolio = Portfolio::Create(stbt.arena, namesCharPtrs.ptr, names.length);

		subIndex = 0;
		symbolIndex = -1;
		normalizeGraph = false;

		stbt.tempArena.DestroyScope(tempScope);
		subScope = stbt.arena.CreateScope();
	}

	bool MI_Backtrader::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		const char* buttons[]
		{
			"Environment",
			"Run Info"
		};

		for (uint32_t i = 0; i < 2; i++)
		{
			const bool selected = subIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });
			if (ImGui::Button(buttons[i]))
			{
				LoadScripts(*this, stbt);
				subIndex = i;
			}
			if (selected)
				ImGui::PopStyleColor();
		}

		if (ImGui::Button("Back"))
			index = 0;
		return false;
	}

	bool MI_Backtrader::DrawSubMenu(STBT& stbt, uint32_t& index)
	{
		if (subIndex == btmiPortfolio)
		{
			ImGui::Begin("Portfolio", nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 200, 0 });
			ImGui::SetWindowSize({ 200, 400 });

			uint32_t index = 0;
			ImGui::InputText("Cash", buffers[0], 9, ImGuiInputTextFlags_CharsScientific);

			for (uint32_t i = 0; i < names.length; i++)
			{
				if (!enabled[i])
					continue;
				++index;

				ImGui::PushID(i);
				if (ImGui::InputText("##", buffers[index], 9, ImGuiInputTextFlags_CharsDecimal))
				{
					int32_t n = std::atoi(buffers[index]);
					if (n < 0)
						snprintf(buffers[index], sizeof(buffers[index]), "%i", 0);
				}
				ImGui::PopID();
				ImGui::SameLine();

				const bool selected = symbolIndex == i;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

				const auto symbol = names[i].c_str();
				if (ImGui::Button(symbol))
					symbolIndex = i;

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::End();
			MI_Symbols::TryRenderSymbol(stbt, timeSeries, names, enabled, symbolIndex, normalizeGraph);
		}
		/*
		if (subIndex == btmiScripts)
		{
			ImGui::Begin("Scripts", nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 200, 200 });
			ImGui::SetWindowSize({ 200, 200 });

			std::string s = "Active: " + (L ? activeScript : "none");
			ImGui::Text(s.c_str());

			for (uint32_t i = 0; i < scripts.length; i++)
			{
				if (ImGui::Button(scripts[i].c_str()))
				{
					activeScript = scripts[i];
					//OpenLua(*this);
				}
			}

			ImGui::End();

			ImGui::Begin("Run Info", nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 200, 0 });
			ImGui::SetWindowSize({ 200, 200 });

			ImGui::Checkbox("Randomize date", &randomizeDate);

			if (randomizeDate)
			{
				ImGui::InputText("Days", dayBuffer, 5, ImGuiInputTextFlags_CharsDecimal);
			}
			else
			{
				ImGui::DatePicker("##", from);
				ImGui::SameLine();
				ImGui::Text("Date 1");
				ImGui::DatePicker("##2", to);
				ImGui::SameLine();
				ImGui::Text("Date 2");

				std::time_t tFrom = mktime(&from), tTo = mktime(&to), tCurrent;
				uint32_t tLength;
				ClampDates(*this, tFrom, tTo, tCurrent, tLength, std::atoi(buffBuffer));
			}

			ImGui::InputText("Buffer", buffBuffer, 4, ImGuiInputTextFlags_CharsDecimal);
			ImGui::InputText("Fee", feeBuffer, 5, ImGuiInputTextFlags_CharsScientific);

			ImGui::PushItemWidth(40);
			if (ImGui::InputText("Runs", runBuffer, 5, ImGuiInputTextFlags_CharsScientific))
			{
				int32_t n = std::atoi(runBuffer);
				if (n < 1)
					snprintf(runBuffer, sizeof(runBuffer), "%i", 1);
			}

			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Checkbox("Log", &log);

			if (ImGui::Button("Run Script"))
			{
				if (activeScript == "")
				{
					output.Add() = "ERROR: No script selected.";
				}
				else
				{
					bool valid = true;
					if (randomizeDate)
					{
						time_t current;
						uint32_t length;
						uint32_t buffer = std::atoi(buffBuffer);
						const auto days = std::atoi(dayBuffer);

						if (!GetMaxLength(*this, current, length, buffer) || days > length)
						{
							output.Add() = "ERROR: Can't start run.";
							valid = false;
						}
					}
					if (valid)
						runsQueued = std::atoi(runBuffer);
				}
			}

			if (difftime(mktime(&from), mktime(&to)) > 0)
			{
				auto temp = to;
				to = from;
				from = to;
			}

			ImGui::End();
		}
		*/
		return false;
	}

	const char* MI_Backtrader::GetMenuTitle()
	{
		return "Backtrader";
	}

	const char* MI_Backtrader::GetSubMenuTitle()
	{
		return "Details";
	}

	const char* MI_Backtrader::GetDescription()
	{
		return "Test trade algorithms \nin paper trading.";
	}

	void MI_Backtrader::Unload(STBT& stbt)
	{
	}
}