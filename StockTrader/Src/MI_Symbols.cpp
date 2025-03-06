#include "pch.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <Utils/UT_Colors.h>
#include <Utils/UT_Time.h>

namespace jv::bt
{
	const std::string ENABLED_PATH = "Symbols/enabled.txt";

	void MI_Symbols::Load(STBT& stbt)
	{
		names = GetSymbolNames(stbt);
		enabled = GetEnabled(stbt, names, enabled);
		timeSeries = CreateArray<TimeSeries>(stbt.arena, 1);
		if (symbolIndex != -1 && symbolIndex >= names.length)
			symbolIndex = -1;
		if(symbolIndex != -1)
			timeSeries[0] = LoadSymbol(stbt, symbolIndex, names, symbolIndex);
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
			std::string s{ nameBuffer };
			for (auto& name : names)
				if (name == s)
				{
					stbt.output.Add() = "ERROR: Symbol already present.";
					return false;
				}

			const auto tempScope = stbt.tempArena.CreateScope();
			const auto c = stbt.tracker.GetData(stbt.tempArena, nameBuffer, "Symbols/", stbt.license);
			stbt.tempArena.DestroyScope(tempScope);
			if (c[0] == '{')
			{
				stbt.output.Add() = "ERROR: Unable to download symbol data.";	
				return false;
			}

			uint32_t lIndex = 0;
			if (s != "")
			{
				for (auto& symbol : names)
					lIndex += symbol < s;

				auto arr = CreateArray<bool>(stbt.arena, enabled.length + 1);
				for (uint32_t i = 0; i < enabled.length; i++)
					arr[i + (i >= lIndex)] = enabled[i];
				arr[lIndex] = false;
				enabled = arr;
			}

			SaveEnabledSymbols(stbt, enabled);
			RequestReload();
			return false;
		}

		ImGui::SameLine();
		ImGui::InputText("#", nameBuffer, 5, ImGuiInputTextFlags_CharsUppercase);

		for (uint32_t i = 0; i < names.length; i++)
		{
			ImGui::PushID(i);
			ImGui::Checkbox("", &enabled[i]);
			ImGui::PopID();
			ImGui::SameLine();

			const bool selected = symbolIndex == i;
			if (selected)
				ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

			const auto symbol = names[i].c_str();
			if (ImGui::Button(symbol))
			{
				symbolIndex = i;
				RequestReload();
			}

			if (selected)
				ImGui::PopStyleColor();
		}

		return false;
	}

	bool MI_Symbols::DrawFree(STBT& stbt, uint32_t& index)
	{
		if (reload)
			return false;
		TryRenderSymbol(stbt, timeSeries, names, enabled, symbolIndex, normalizeGraph);
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
		std::ofstream fout(ENABLED_PATH);
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
		std::ifstream fin(ENABLED_PATH);
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

	Array<gr::GraphPoint> MI_Symbols::RenderSymbolData(STBT& stbt, Array<TimeSeries>& timeSeries,
		const Array<std::string>& names, const Array<bool>& enabled, uint32_t& symbolIndex, const bool normalizeGraph)
	{
		std::time_t tFrom, tTo, tCurrent;
		uint32_t length;
		ClampDates(stbt, tFrom, tTo, tCurrent, timeSeries, length, 0);

		auto diff = difftime(tTo, tFrom);
		diff = Min<double>(diff, (length - 1) * 60 * 60 * 24);
		auto orgDiff = Max<double>(difftime(tCurrent, tTo), 0);
		uint32_t daysDiff = diff / 60 / 60 / 24;
		uint32_t daysOrgDiff = orgDiff / 60 / 60 / 24;

		// Get symbol index to normal index.
		uint32_t sId = 0;
		if (timeSeries.length > 1)
		{
			for (uint32_t i = 0; i < names.length; i++)
			{
				if (!enabled[i])
					continue;
				if (i == symbolIndex)
					break;
				++sId;
			}
			if (sId == timeSeries.length)
			{
				sId = 0;
				for (uint32_t j = 0; j < length; j++)
					if (enabled[j])
					{
						symbolIndex = j;
						break;
					}
			}
		}

		auto randColors = LoadRandColors(stbt.frameArena, timeSeries.length);

		auto graphPoints = CreateArray<Array<jv::gr::GraphPoint>>(stbt.frameArena, timeSeries.length);
		for (uint32_t i = 0; i < timeSeries.length; i++)
		{
			auto& series = timeSeries[i];
			auto& points = graphPoints[i] = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, daysDiff);

			for (uint32_t i = 0; i < daysDiff; i++)
			{
				const uint32_t index = daysDiff - i + daysOrgDiff - 1;
				points[i].open = series.open[index];
				points[i].close = series.close[index];
				points[i].high = series.high[index];
				points[i].low = series.low[index];
			}

			stbt.renderer.graphBorderThickness = 0;
			stbt.renderer.SetLineWidth(1.f + (sId == i) * 1.f);

			auto color = randColors[i];
			color *= .2f + .8f * (sId == i);

			stbt.renderer.DrawGraph({ .5, 0 },
				glm::vec2(stbt.renderer.GetAspectRatio(), 1),
				points.ptr, points.length, static_cast<gr::GraphType>(stbt.graphType),
				true, normalizeGraph, color);
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
					v += timeSeries[sId].close[index];
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
				true, normalizeGraph, glm::vec4(0, 1, 0, 1));
		}

		return graphPoints[sId];
	}

	void MI_Symbols::TryRenderSymbol(STBT& stbt, Array<TimeSeries>& timeSeries,
		const Array<std::string>& names, const Array<bool>& enabled, uint32_t& symbolIndex, bool& normalizeGraph)
	{
		if (symbolIndex == -1)
			return;

		auto graphPoints = RenderSymbolData(stbt, timeSeries, names, enabled, symbolIndex, normalizeGraph);

		DrawTopRightWindow("Settings");
		ImGui::DatePicker("Date 1", stbt.from);
		ImGui::DatePicker("Date 2", stbt.to);

		const char* items[]{ "Line","Candles" };
		bool check = ImGui::Combo("Graph Type", &stbt.graphType, items, 2);

		if (ImGui::Button("Days"))
		{
			const int i = stbt.days;
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

		ImGui::PushItemWidth(40);

		char dayBuffer[6];
		snprintf(dayBuffer, sizeof(dayBuffer), "%i", stbt.days);

		ImGui::SameLine();
		if (ImGui::InputText("##", dayBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			stbt.days = std::atoi(dayBuffer);

		char maBuffer[6];
		snprintf(maBuffer, sizeof(maBuffer), "%i", stbt.ma);

		ImGui::SameLine();
		if (ImGui::InputText("MA", maBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
			stbt.ma = std::atoi(maBuffer);

		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Checkbox("Norm", &normalizeGraph);
		ImGui::SameLine();
		if (ImGui::Button("Lifetime"))
		{
			stbt.from = {};
			auto t = GetTime();
			stbt.to = *std::gmtime(&t);
		}
		ImGui::End();

		std::string title = "Details: ";
		title += names[symbolIndex];
		DrawBottomRightWindow(title.c_str());

		float min = FLT_MAX, max = 0;

		for (uint32_t i = 0; i < graphPoints.length; i++)
		{
			const auto& point = graphPoints[i];
			min = Min<float>(min, point.low);
			max = Max<float>(max, point.high);
		}

		if (graphPoints.length > 0)
		{
			auto str = std::format("{:.2f}", graphPoints[0].open);
			str = "[Start] " + str;
			ImGui::Text(str.c_str());
			ImGui::SameLine();

			str = std::format("{:.2f}", graphPoints[graphPoints.length - 1].close);
			str = "[End] " + str;
			ImGui::Text(str.c_str());
			ImGui::SameLine();

			const float change = graphPoints[graphPoints.length - 1].close - graphPoints[0].open;
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
	void MI_Symbols::DrawTopRightWindow(const char* name)
	{
		ImGui::Begin(name, nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 400, 0 });
		ImGui::SetWindowSize({ 400, 124 });
	}
	void MI_Symbols::DrawBottomRightWindow(const char* name, const bool popup)
	{
		const ImVec2 pos = { 400, 500 };
		const ImVec2 size = { 400, 100 };

		if (!popup)
		{
			ImGui::Begin(name, nullptr, WIN_FLAGS);
			ImGui::SetWindowPos(pos);
			ImGui::SetWindowSize(size);
		}
		else
		{
			ImGuiIO& io = ImGui::GetIO();
			ImGui::SetNextWindowSize(size);
			ImGui::SetNextWindowPos(pos);
			ImGui::OpenPopup(name);
			ImGui::BeginPopup(name);
		}
	}
}