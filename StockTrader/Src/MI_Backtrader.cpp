#include "pch.h"
#include "MenuItems/MI_Backtrader.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>
#include <Ext/ImGuiLoadBar.h>

namespace jv::bt
{
	enum BTMenuIndex
	{
		btmiPortfolio,
		btmiAlgorithms,
		btmiRunInfo
	};

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
		if (enabled.length > 0)
			symbolIndex = 0;
		normalizeGraph = false;

		algoIndex = -1;
		running = false;

		uint32_t n = 1;
		snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);

		stbt.tempArena.DestroyScope(tempScope);
		subScope = stbt.arena.CreateScope();
	}

	bool MI_Backtrader::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		const char* buttons[]
		{
			"Environment",
			"Algorithms",
			"Run Info"
		};

		for (uint32_t i = 0; i < 3; i++)
		{
			const bool selected = subIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });
			if (ImGui::Button(buttons[i]))
				subIndex = i;
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
			ImGui::Text("Portfolio");

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
					n = Max(n, 0);
					snprintf(buffers[index], sizeof(buffers[index]), "%i", n);
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
		}
		if (subIndex == btmiAlgorithms)
		{
			for (uint32_t i = 0; i < stbt.bots.length; i++)
			{
				auto& bot = stbt.bots[i];
				const bool selected = i == algoIndex;

				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });
				if (ImGui::Button(bot.name))
					algoIndex = i;
				if (selected)
					ImGui::PopStyleColor();

				if (selected)
				{
					ImGui::Text(bot.author);
					ImGui::Text(bot.description);
				}
			}
		}
		if (subIndex == btmiRunInfo)
		{
			if (ImGui::InputText("Buffer", buffBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(buffBuffer);
				n = Max(n, 0);
				snprintf(buffBuffer, sizeof(buffBuffer), "%i", n);
			}

			if (ImGui::InputText("Fee", feeBuffer, 8, ImGuiInputTextFlags_CharsDecimal))
			{
				float n = std::atof(feeBuffer);
				n = Max(n, 0.f);
				snprintf(feeBuffer, sizeof(feeBuffer), "%f", n);
			}

			if (ImGui::InputText("Runs", runCountBuffer, 4, ImGuiInputTextFlags_CharsDecimal))
			{
				int32_t n = std::atoi(runCountBuffer);
				n = Max(n, 1);
				snprintf(runCountBuffer, sizeof(runCountBuffer), "%i", n);
			}

			ImGui::Checkbox("Log", &log);
			ImGui::Checkbox("Randomize Date", &randomizeDate);

			if (randomizeDate)
			{
				if (ImGui::InputText("Length", lengthBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
				{
					int32_t n = std::atoi(lengthBuffer);
					n = Max(n, 0);
					snprintf(lengthBuffer, sizeof(lengthBuffer), "%i", n);
				}
			}

			if (ImGui::Button("Run"))
			{
				bool valid = true;
				if (algoIndex == -1)
				{
					valid = false;
					stbt.output.Add() = "ERROR: No algorithm selected!";
				}

				if (symbolIndex == -1)
				{
					valid = false;
					stbt.output.Add() = "ERROR: No symbols available!";
				}

				if (valid)
				{
					// check buffer w/ min date
					const int32_t buffer = std::atoi(buffBuffer);
					const int32_t length = std::atoi(lengthBuffer);

					const auto tFrom = mktime(&stbt.from);
					const auto tTo = mktime(&stbt.to);

					const auto diff = difftime(tTo, tFrom);
					const uint32_t daysDiff = diff / 60 / 60 / 24;

					if (daysDiff < buffer + 1)
					{
						valid = false;
						stbt.output.Add() = "ERROR: Buffer range is out of scope!";
					}

					if (randomizeDate && daysDiff < buffer + length)
					{
						valid = false;
						stbt.output.Add() = "ERROR: Length is out of scope!";
					}

					running = true;
				}
			}
		}
		return false;
	}

	bool MI_Backtrader::DrawFree(STBT& stbt, uint32_t& index)
	{
		MI_Symbols::TryRenderSymbol(stbt, timeSeries, names, enabled, symbolIndex, normalizeGraph);

		if (running)
		{
			ImGuiIO& io = ImGui::GetIO();
			glm::vec2 size = { 240, 120 };
			ImGui::SetNextWindowSize({ size.x, size.y });
			ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
			ImGui::OpenPopup("name");
			if (ImGui::BeginPopup("name"))
			{
				const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
				const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

				ImGui::Spinner("##spinner", 15, 6, col);

				ImGui::Text("Run 1/240, Day 56/213");
				ImGui::Text("Est. 1 min 54 sec remaining");

				ImGui::BufferingBar("##buffer_bar", 0.7f, ImVec2(200, 6), bg, col);
				ImGui::Text("Calculating MA");

				ImGui::End();
			}
		}
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