#include "pch.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>
#include <Utils/UT_Time.h>

namespace jv::bt
{
	void MI_Symbols::Load(STBT& stbt)
	{
		names = GetSymbolNames(stbt);
		enabled = GetEnabled(stbt, names, enabled);
		timeSeries = CreateArray<TimeSeries>(stbt.arena, 1);
		timeSeriesColors = LoadRandColors(stbt, 1);
		index = -1;
	}

	bool MI_Symbols::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		if (ImGui::Button("Enable All"))
			for (auto& b : enabled)
				b = true;
		if (ImGui::Button("Disable All"))
			for (auto& b : enabled)
				b = false;
		if (ImGui::Button("Save changes"))
			SaveOrCreateEnabledSymbols(stbt, names, enabled);
		if (ImGui::Button("Back"))
			index = 0;

		return false;
	}

	bool MI_Symbols::DrawSubMenu(STBT& stbt, uint32_t& index)
	{
		if (ImGui::Button("Add"))
		{
			const auto tempScope = stbt.tempArena.CreateScope();
			const auto c = stbt.tracker.GetData(stbt.tempArena, nameBuffer, "Symbols/", stbt.license);
			if (c[0] == '{')
				stbt.output.Add() = "ERROR: Unable to download symbol data.";

			stbt.tempArena.DestroyScope(tempScope);

			uint32_t index = 0;
			std::string s{ nameBuffer };
			if (s != "")
			{
				for (auto& symbol : names)
					index += symbol < s;

				auto arr = CreateArray<bool>(stbt.arena, enabled.length + 1);
				for (uint32_t i = 0; i < enabled.length; i++)
					arr[i + (i >= index)] = enabled[i];
				arr[index] = false;
				enabled = arr;
			}

			SaveEnabledSymbols(stbt, enabled);
			RequestReload();
		}
		ImGui::SameLine();
		ImGui::InputText("#", nameBuffer, 5, ImGuiInputTextFlags_CharsUppercase);

		for (uint32_t i = 0; i < names.length; i++)
		{
			ImGui::PushID(i);
			ImGui::Checkbox("", &enabled[i]);
			ImGui::PopID();
			ImGui::SameLine();

			const bool selected = index == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

			const auto symbol = names[i].c_str();
			if (ImGui::Button(symbol))
			{
				timeSeries[0] = LoadSymbol(stbt, i, names, index);
				index = i;
				RequestReload();
			}

			if (selected)
				ImGui::PopStyleColor();
		}

		return false;
	}

	bool MI_Symbols::DrawFree(STBT& stbt, uint32_t& index)
	{
		TryRenderSymbol(stbt);
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

	void MI_Symbols::SaveEnabledSymbols(STBT& stbt, const Array<bool>& enabled)
	{
		const std::string path = "Symbols/enabled.txt";
		std::ofstream fout(path);

		for (const auto enabled : enabled)
			fout << enabled << std::endl;
		fout.close();
	}

	void MI_Symbols::SaveOrCreateEnabledSymbols(STBT& stbt, const Array<std::string>& names, Array<bool>& enabled)
	{
		if (enabled.length != names.length)
		{
			enabled = jv::CreateArray<bool>(stbt.arena, names.length);
			for (auto& b : enabled)
				b = true;
		}

		SaveEnabledSymbols(stbt, enabled);
	}

	Array<std::string> MI_Symbols::GetSymbolNames(STBT& stbt)
	{
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

		return arr;
	}

	TimeSeries MI_Symbols::LoadSymbol(STBT& stbt, const uint32_t i, const Array<std::string>& names, uint32_t& index)
	{
		index = -1;
		const auto str = stbt.tracker.GetData(stbt.tempArena, names[i].c_str(), "Symbols/", stbt.license);
		// If the data is invalid.
		if (str[0] == '{')
		{
			stbt.output.Add() = "ERROR: No valid symbol data found.";
		}
		else
		{
			index = i;
			auto timeSeries = stbt.tracker.ConvertDataToTimeSeries(stbt.arena, str);
			if (timeSeries.date != GetTime())
				stbt.output.Add() = "WARNING: Symbol data is outdated.";
			return timeSeries;
		}

		return {};
	}

	Array<bool> MI_Symbols::GetEnabled(STBT& stbt, const Array<std::string>& names, Array<bool>& enabled)
	{
		const std::string path = "Symbols/enabled.txt";
		std::ifstream fin(path);
		std::string line;

		if (!fin.good())
		{
			SaveOrCreateEnabledSymbols(stbt, names, enabled);
			return {};
		}

		uint32_t length = 0;
		while (std::getline(fin, line))
			++length;

		if (length != names.length)
		{
			SaveOrCreateEnabledSymbols(stbt, names, enabled);
			return {};
		}

		fin.clear();
		fin.seekg(0, std::ios::beg);

		auto arr = jv::CreateArray<bool>(stbt.arena, length);

		length = 0;
		while (std::getline(fin, line))
			arr[length++] = std::stoi(line);

		return arr;
	}

	void MI_Symbols::RenderSymbolData(STBT& stbt)
	{
		/*
		std::time_t tFrom, tTo, tCurrent;
		uint32_t length;
		ClampDates(stbt, tFrom, tTo, tCurrent, length, 0);

		auto diff = difftime(tTo, tFrom);
		diff = Min<double>(diff, (length - 1) * 60 * 60 * 24);
		auto orgDiff = Max<double>(difftime(tCurrent, tTo), 0);
		uint32_t daysDiff = diff / 60 / 60 / 24;
		uint32_t daysOrgDiff = orgDiff / 60 / 60 / 24;

		// Get symbol index to normal index.
		uint32_t sId = 0;
		if (stbt.timeSeriesArr.length > 1)
		{
			for (uint32_t i = 0; i < stbt.enabledSymbols.length; i++)
			{
				if (!stbt.enabledSymbols[i])
					continue;
				if (i == stbt.symbolIndex)
					break;
				++sId;
			}
		}

		auto graphPoints = CreateArray<Array<jv::gr::GraphPoint>>(stbt.frameArena, stbt.timeSeriesArr.length);
		for (uint32_t i = 0; i < stbt.timeSeriesArr.length; i++)
		{
			auto& timeSeries = stbt.timeSeriesArr[i];
			auto& points = graphPoints[i] = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, daysDiff);

			for (uint32_t i = 0; i < daysDiff; i++)
			{
				const uint32_t index = daysDiff - i + daysOrgDiff - 1;
				points[i].open = timeSeries.open[index];
				points[i].close = timeSeries.close[index];
				points[i].high = timeSeries.high[index];
				points[i].low = timeSeries.low[index];
			}

			stbt.renderer.graphBorderThickness = 0;
			stbt.renderer.SetLineWidth(1.f + (sId == i) * 1.f);

			auto color = stbt.randColors[i];
			color *= .2f + .8f * (sId == i);

			stbt.renderer.DrawGraph({ .5, 0 },
				glm::vec2(stbt.renderer.GetAspectRatio(), 1),
				points.ptr, points.length, static_cast<gr::GraphType>(stbt.graphType),
				true, stbt.normalizeGraph, color);
		}
		stbt.renderer.SetLineWidth(1);

		// If it's not trying to get data from before this stock existed.
		if (stbt.ma > 0 && daysOrgDiff + daysDiff + 1 + stbt.ma < length && stbt.ma < 10000)
		{
			auto points = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, daysDiff);
			for (uint32_t i = 0; i < daysDiff; i++)
			{
				float v = 0;

				for (uint32_t j = 0; j < stbt.ma; j++)
				{
					const uint32_t index = daysDiff - i + j + daysOrgDiff - 1;
					v += stbt.timeSeriesArr[sId].close[index];
				}
				v /= stbt.ma;

				points[i].open = v;
				points[i].close = v;
				points[i].high = v;
				points[i].low = v;
			}

			stbt.renderer.DrawGraph({ .5, 0 },
				glm::vec2(stbt.renderer.GetAspectRatio(), 1),
				points.ptr, points.length, gr::GraphType::line,
				true, stbt.normalizeGraph, glm::vec4(0, 1, 0, 1));
		}

		stbt.graphPoints = graphPoints[sId];
		*/
	}

	void MI_Symbols::TryRenderSymbol(STBT& stbt)
	{
		/*
		if (stbt.symbolIndex != -1)
		{
			RenderSymbolData(stbt);

			ImGui::Begin("Settings", nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 400, 0 });
			ImGui::SetWindowSize({ 400, 124 });
			ImGui::DatePicker("Date 1", stbt.from);
			ImGui::DatePicker("Date 2", stbt.to);

			const char* items[]{ "Line","Candles" };
			bool check = ImGui::Combo("Graph Type", &stbt.graphType, items, 2);

			if (ImGui::Button("Days"))
			{
				const int i = std::atoi(stbt.dayBuffer);
				if (i < 1)
				{
					stbt.output.Add() = "ERROR: Invalid number of days given.";
				}
				else
				{
					auto t = GetTime();
					stbt.to = *std::gmtime(&t);
					t = GetTime(i);
					stbt.from = *std::gmtime(&t);
				}
			}

			ImGui::SameLine();
			ImGui::PushItemWidth(40);
			ImGui::InputText("##", stbt.dayBuffer, 5, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			ImGui::InputText("MA", stbt.buffer3, 5, ImGuiInputTextFlags_CharsDecimal);
			stbt.ma = std::atoi(stbt.buffer3);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Checkbox("Norm", &stbt.normalizeGraph);
			ImGui::SameLine();
			if (ImGui::Button("Lifetime"))
			{
				stbt.from = {};
				auto t = GetTime();
				stbt.to = *std::gmtime(&t);
			}
			ImGui::End();

			std::string title = "Details: ";
			title += stbt.loadedSymbols[stbt.symbolIndex];
			ImGui::Begin(title.c_str(), nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 400, 500 });
			ImGui::SetWindowSize({ 400, 100 });

			float min = FLT_MAX, max = 0;

			for (uint32_t i = 0; i < stbt.graphPoints.length; i++)
			{
				const auto& point = stbt.graphPoints[i];
				min = Min<float>(min, point.low);
				max = Max<float>(max, point.high);
			}

			if (stbt.graphPoints.length > 0)
			{
				auto str = std::format("{:.2f}", stbt.graphPoints[0].open);
				str = "[Start] " + str;
				ImGui::Text(str.c_str());
				ImGui::SameLine();

				str = std::format("{:.2f}", stbt.graphPoints[stbt.graphPoints.length - 1].close);
				str = "[End] " + str;
				ImGui::Text(str.c_str());
				ImGui::SameLine();

				const float change = stbt.graphPoints[stbt.graphPoints.length - 1].close - stbt.graphPoints[0].open;
				ImGui::PushStyleColor(ImGuiCol_Text, { 1.f * (change < 0), 1.f * (change >= 0), 0, 1 });
				str = std::format("{:.2f}", change);
				str = "[Change] " + str;
				ImGui::Text(str.c_str());
				ImGui::PopStyleColor();

				str = std::format("{:.2f}", max);
				str = "[High] " + str;
				ImGui::Text(str.c_str());
				ImGui::SameLine();

				str = std::format("{:.2f}", min);
				str = "[Low] " + str;
				ImGui::Text(str.c_str());
			}

			ImGui::End();
		}
		*/
	}
}