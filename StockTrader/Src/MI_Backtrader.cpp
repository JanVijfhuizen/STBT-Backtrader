#include "pch.h"
#include "MenuItems/MI_Backtrader.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>

namespace jv::bt
{
	void MI_Backtrader::Load(STBT& stbt)
	{
		/*
		MI_Symbols::LoadSymbols(stbt);
		uint32_t c = 0;
		for (uint32_t i = 0; i < stbt.loadedSymbols.length; i++)
			c += stbt.enabledSymbols[i];
		stbt.buffArr = CreateArray<char*>(stbt.arena, c + 1);
		for (uint32_t i = 0; i < stbt.buffArr.length; i++)
			stbt.buffArr[i] = stbt.arena.New<char>(16);
		stbt.timeSeriesArr = CreateArray<TimeSeries>(stbt.arena, c);

		uint32_t index = 0;
		for (size_t i = 0; i < stbt.enabledSymbols.length; i++)
		{
			if (!stbt.enabledSymbols[i])
				continue;
			stbt.timeSeriesArr[index++] = LoadSymbol(stbt, i);
		}

		LoadRandColors(stbt);
		stbt.subScope = stbt.arena.CreateScope();
		stbt.subMenuIndex = 0;
		if (c > 0)
			stbt.symbolIndex = 0;

		stbt.portfolio = CreateArray<uint32_t>(stbt.arena, index);
		*/
	}

	bool MI_Backtrader::DrawMainMenu(uint32_t& index)
	{
		/*
		const char* buttons[]
		{
			"Environment",
			"Run Info"
		};

		for (uint32_t i = 0; i < 2; i++)
		{
			const bool selected = subMenuIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });
			if (ImGui::Button(buttons[i]))
			{
				LoadScripts(*this);
				subMenuIndex = i;
			}
			if (selected)
				ImGui::PopStyleColor();
		}
		*/
		return false;
	}

	bool MI_Backtrader::DrawSubMenu(uint32_t& index)
	{
		/*
		if (subMenuIndex == btmiPortfolio)
		{
			ImGui::Begin("Portfolio", nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 200, 0 });
			ImGui::SetWindowSize({ 200, 400 });

			uint32_t index = 0;
			ImGui::InputText("Cash", buffArr[0], 9, ImGuiInputTextFlags_CharsScientific);

			for (uint32_t i = 0; i < loadedSymbols.length; i++)
			{
				if (!enabledSymbols[i])
					continue;
				++index;

				ImGui::PushID(i);
				if (ImGui::InputText("##", buffArr[index], 9, ImGuiInputTextFlags_CharsDecimal))
				{
					int32_t n = std::atoi(buffArr[index]);
					if (n < 0)
						snprintf(buffArr[index], sizeof(buffArr[index]), "%i", 0);
				}
				ImGui::PopID();
				ImGui::SameLine();

				const bool selected = symbolIndex == i;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

				const auto symbol = loadedSymbols[i].c_str();
				if (ImGui::Button(symbol))
					symbolIndex = i;

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::End();
			TryRenderSymbol(*this);
		}
		if (subMenuIndex == btmiScripts)
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
					OpenLua(*this);
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
		return nullptr;
	}

	const char* MI_Backtrader::GetDescription()
	{
		return "Test trade algorithms \nin paper trading.";
	}

	void MI_Backtrader::Unload(STBT& stbt)
	{
	}
}