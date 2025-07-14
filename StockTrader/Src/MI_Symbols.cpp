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
		colors = LoadRandColors(stbt.arena, names.length);
		// If no symbol has been selected or if the index is currently out of range.
		if (symbolIndex != -1 && symbolIndex >= names.length)
			symbolIndex = -1;
		// Load symbol in if it's already selected.
		if(symbolIndex != -1)
			timeSeries[0] = LoadSymbol(stbt, symbolIndex, names, symbolIndex);
	}

	bool MI_Symbols::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		std::string enableText = "When using the backtrader,\nall enabled stocks will\nbe loaded in.\n";
		enableText += "Your license might limit\nthe amount of load calls\nyou can make per day.";

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
					stbt.output.Add() = OutputMsg::Create("Symbol already present.", OutputMsg::error);
					return false;
				}

			const auto tempScope = stbt.tempArena.CreateScope();
			const auto c = stbt.tracker.GetData(stbt.tempArena, nameBuffer, "Symbols/", stbt.license);
			stbt.tempArena.DestroyScope(tempScope);
			if (c[0] == '{')
			{
				stbt.output.Add() = OutputMsg::Create("Unable to download symbol data.", OutputMsg::error);

				const auto scope = stbt.tempArena.CreateScope();
				const auto msgs = OutputMsg::CreateMultiple(stbt.tempArena, c.c_str());
				for (auto& msg : msgs)
					stbt.output.Add() = msg;
				stbt.tempArena.DestroyScope(scope);
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
		ImGui::InputText("#", nameBuffer, 9, ImGuiInputTextFlags_CharsUppercase);

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

		SymbolsDataDrawInfo drawInfo{};
		drawInfo.timeSeries = timeSeries.ptr;
		drawInfo.names = names.ptr;
		drawInfo.enabled = enabled.ptr;
		drawInfo.colors = colors.ptr;
		drawInfo.length = timeSeries.length;
		drawInfo.symbolIndex = &symbolIndex;
		drawInfo.normalizeGraph = &normalizeGraph;
		RenderSymbolData(stbt, drawInfo);
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

		const char* SYMBOLS_DIR = "Symbols";
		if (!std::filesystem::is_directory(SYMBOLS_DIR))
			std::filesystem::create_directories(SYMBOLS_DIR);

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
			stbt.output.Add() = OutputMsg::Create("No valid symbol data found.", OutputMsg::error);
		}
		else
		{
			index = i;
			auto timeSeries = stbt.tracker.ConvertDataToTimeSeries(stbt.arena, str);
			if (!CompDates(timeSeries.date, GetTime()))
				stbt.output.Add() = OutputMsg::Create("Symbol data is outdated.", OutputMsg::warning);
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

	Array<gr::GraphPoint> MI_Symbols::RenderSymbolGraph(STBT& stbt, const SymbolGraphDrawInfo& drawInfo)
	{
		const uint32_t length = drawInfo.length;
		const bool normalizeGraph = drawInfo.normalizeGraph;
		auto timeSeries = drawInfo.timeSeries;
		auto enabled = drawInfo.enabled;
		auto colors = drawInfo.colors;
		auto& symbolIndex = *drawInfo.symbolIndex;
		const bool reverse = drawInfo.reverse;

		// Get symbol index to normal index.
		uint32_t sId = 0;
		if (length > 1)
		{
			uint32_t i = 0;
			while(true)
			{
				if (i == symbolIndex)
					break;
				if (!enabled[i++])
					continue;
				++sId; 
			}
			if (sId == length)
			{
				sId = 0;
				for (uint32_t j = 0; j < stbt.range; j++)
					if (enabled[j])
					{
						symbolIndex = j;
						break;
					}
			}
		}

		const float ratio = stbt.renderer.GetAspectRatio();

		auto graphPoints = CreateArray<Array<jv::gr::GraphPoint>>(stbt.frameArena, length);
		for (uint32_t i = 0; i < length; i++)
		{
			auto& series = timeSeries[i];
			auto& points = graphPoints[i] = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, stbt.range);

			for (uint32_t j = 0; j < stbt.range; j++)
			{
				const uint32_t cJ = reverse ? stbt.range - j - 1 : j;

				points[j].open = series.open[cJ];
				points[j].close = series.close[cJ];
				points[j].high = series.high[cJ];
				points[j].low = series.low[cJ];
			}

			stbt.renderer.SetLineWidth(1.f + (sId == i) * 1.f);

			auto color = colors[i];
			color *= .2f + .8f * (sId == i);

			jv::gr::DrawLineGraphInfo drawLineInfo{};
			drawLineInfo.aspectRatio = ratio;
			drawLineInfo.position = { .5f, 0 };
			drawLineInfo.points = points.ptr;
			drawLineInfo.length = points.length;
			drawLineInfo.type = static_cast<gr::GraphType>(stbt.graphType);
			drawLineInfo.normalize = normalizeGraph;
			drawLineInfo.color = color;
			stbt.renderer.DrawLineGraph(drawLineInfo);
		}
		stbt.renderer.SetLineWidth(1);

		return graphPoints[sId];
	}

	void MI_Symbols::RenderSymbolData(STBT& stbt, const SymbolsDataDrawInfo& drawInfo)
	{
		const uint32_t length = drawInfo.length;
		bool* normalizeGraph = drawInfo.normalizeGraph;
		auto timeSeries = drawInfo.timeSeries;
		auto enabled = drawInfo.enabled;
		auto colors = drawInfo.colors;
		auto names = drawInfo.names;
		auto& symbolIndex = *drawInfo.symbolIndex;

		if (symbolIndex == -1)
			return;

		SymbolGraphDrawInfo graphDrawInfo{};
		graphDrawInfo.timeSeries = timeSeries;
		graphDrawInfo.names = names;
		graphDrawInfo.enabled = enabled;
		graphDrawInfo.colors = colors;
		graphDrawInfo.length = length;
		graphDrawInfo.symbolIndex = &symbolIndex;
		graphDrawInfo.normalizeGraph = *normalizeGraph;	
		graphDrawInfo.reverse = true;
		auto graphPoints = RenderSymbolGraph(stbt, graphDrawInfo);

		DrawTopRightWindow("Settings");

		const char* items[]{ "Line","Candles" };
		bool check = ImGui::Combo("Graph Type", &stbt.graphType, items, 2);

		ImGui::PushItemWidth(40);
		char dayBuffer[6];
		snprintf(dayBuffer, sizeof(dayBuffer), "%i", stbt.range);

		if (ImGui::InputText("Range", dayBuffer, 5, ImGuiInputTextFlags_CharsDecimal))
		{
			const int i = std::atoi(dayBuffer);
			if (i < 0)
			{
				stbt.output.Add() = OutputMsg::Create("Invalid number of days given.", OutputMsg::error);
				stbt.range = 0;
			}
			else
				stbt.range = i;
		}

		uint32_t l = UINT32_MAX;
		for (uint32_t i = 0; i < length; i++)
			l = Min(l, timeSeries[i].length);
		if (l < stbt.range)
			stbt.range = l;

		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Checkbox("Norm", normalizeGraph);
		ImGui::SameLine();
		if (ImGui::Button("Lifetime"))
			stbt.range = l;
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
}