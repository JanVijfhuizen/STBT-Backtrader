#include "pch.h"
#include "STBT.h"
#include <Jlib/ArrayUtils.h>
#include "JLib/QueueUtils.h"

namespace jv::ai
{
	const char* LICENSE_FILE_PATH = "license.txt";
	const auto WIN_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

	enum MenuIndex 
	{
		miMain,
		miLicense,
		miSymbols,
		miBacktrader
	};

	void* MAlloc(const uint32_t size)
	{
		return malloc(size);
	}
	void MFree(void* ptr)
	{
		return free(ptr);
	}

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

	static void LoadEnabledSymbols(STBT& stbt)
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

	static void LoadSymbols(STBT& stbt)
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

	static void LoadBacktraderSubMenu(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.currentScope);
		LoadSymbols(stbt);
		uint32_t c = 0;
		for (uint32_t i = 0; i < stbt.loadedSymbols.length; i++)
			c += stbt.enabledSymbols[i];
		stbt.buffArr = CreateArray<char*>(stbt.arena, c);
		for (uint32_t i = 0; i < stbt.buffArr.length; i++)
			stbt.buffArr[i] = stbt.arena.New<char>(16);
	}

	static void LoadSymbolSubMenu(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.currentScope);
		LoadSymbols(stbt);
	}

	static std::time_t GetT(const uint32_t days = 0)
	{
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

		using namespace std::chrono_literals;
		auto offset = 60s * 60 * 24 * days;
		now -= offset;

		std::time_t tCurrent = std::chrono::system_clock::to_time_t(now);

		auto lt = std::localtime(&tCurrent);
		lt->tm_hour = 0;
		lt->tm_min = 0;
		lt->tm_sec = 0;
		tCurrent = mktime(lt);
		return tCurrent;
	}

	static void RenderSymbolData(STBT& stbt)
	{
		const auto& timeSeries = stbt.timeSeries;

		std::time_t tCurrent = timeSeries.date;

		auto tTo = mktime(&stbt.to);
		auto tFrom = mktime(&stbt.from);

		if (tFrom >= tCurrent)
		{
			tFrom = tCurrent;
			stbt.from = *std::gmtime(&tCurrent);
		}
		if (tTo >= tCurrent)
		{
			tTo = tCurrent;
			stbt.to = *std::gmtime(&tCurrent);
		}
		if (tFrom > tTo)
		{
			auto tTemp = tTo;
			tTo = tFrom;
			tFrom = tTemp;
		}

		auto diff = difftime(tTo, tFrom);
		diff = Min<double>(diff, (timeSeries.length - 1) * 60 * 60 * 24);
		auto orgDiff = Max<double>(difftime(tCurrent, tTo), 0);
		uint32_t daysDiff = diff / 60 / 60 / 24;
		uint32_t daysOrgDiff = orgDiff / 60 / 60 / 24;

		auto points = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, daysDiff);

		for (uint32_t i = 0; i < daysDiff; i++)
		{
			const uint32_t index = daysDiff - i + daysOrgDiff - 1;
			points[i].open = timeSeries.open[index];
			points[i].close = timeSeries.close[index];
			points[i].high = timeSeries.high[index];
			points[i].low = timeSeries.low[index];
		}
		// If it's not trying to get data from before this stock existed.
		if (stbt.ma > 0 && daysOrgDiff + daysDiff + 1 + stbt.ma < timeSeries.length && stbt.ma < 10000)
		{
			auto points = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, daysDiff);
			for (uint32_t i = 0; i < daysDiff; i++)
			{
				float v = 0;

				for (uint32_t j = 0; j < stbt.ma; j++)
				{
					const uint32_t index = daysDiff - i + j + daysOrgDiff - 1;
					v += timeSeries.close[index];
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

		stbt.renderer.graphBorderThickness = 0;
		stbt.renderer.DrawGraph({ .5, 0 }, 
			glm::vec2(stbt.renderer.GetAspectRatio(), 1), 
			points.ptr, points.length, static_cast<gr::GraphType>(stbt.graphType), 
			true, stbt.normalizeGraph);

		stbt.graphPoints = points;
	}

	void LoadSymbol(STBT& stbt, const uint32_t i) 
	{
		LoadSymbolSubMenu(stbt);
		stbt.symbolIndex = i;

		const auto str = stbt.tracker.GetData(stbt.tempArena, stbt.loadedSymbols[i].c_str(), "Symbols/", stbt.license);
		// If the data is invalid.
		if (str[0] == '{')
		{
			stbt.symbolIndex = -1;
			stbt.output.Add() = "ERROR: No valid symbol data found.";
		}
		else
		{
			stbt.timeSeries = stbt.tracker.ConvertDataToTimeSeries(stbt.arena, str);
			if (stbt.timeSeries.date != GetT())
				stbt.output.Add() = "WARNING: Symbol data is outdated.";
		}
	}

	void TryRenderSymbol(STBT& stbt)
	{
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
				const int i = std::atoi(stbt.buffer2);
				if (i < 1)
				{
					stbt.output.Add() = "ERROR: Invalid number of days given.";
				}
				else
				{
					auto t = GetT();
					stbt.to = *std::gmtime(&t);
					t = GetT(i);
					stbt.from = *std::gmtime(&t);
				}
			}

			ImGui::SameLine();
			ImGui::PushItemWidth(40);
			ImGui::InputText("##", stbt.buffer2, 5, ImGuiInputTextFlags_CharsDecimal);
			ImGui::SameLine();
			ImGui::InputText("MA", stbt.buffer3, 5, ImGuiInputTextFlags_CharsDecimal);
			stbt.ma = std::atoi(stbt.buffer3);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::Checkbox("Norm", &stbt.normalizeGraph);

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
	}

	bool STBT::Update()
	{
		ImGui::Begin("Menu", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({0, 0});
		ImGui::SetWindowSize({200, 400});

		const char* title = "DEFAULT";
		const char* description = "";
		switch (menuIndex)
		{
		case miMain:
			title = "Main Menu";
			description = "This tool can be used to \ntrain, test and use stock \ntrade algorithms in \nrealtime.";
			break;
		case miLicense:
			title = "Licensing";
			description = "NOTE THAT THIS PROGRAM DOES \nNOT REQUIRE ANY LICENSING. \nLicenses however, are \nrequired for some external \nAPI calls.";
			break;
		case miSymbols:
			title = "Symbols";
			description = "Debug symbols, (un)load \nthem and add new ones.";
			break;
		case miBacktrader:
			title = "Backtrader";
			description = "Test trade algorithms \nin paper trading.";
			break;
		}
		ImGui::Text(title);
		ImGui::Text(description);
		ImGui::Dummy({ 0, 20 });

		if (menuIndex == miMain)
		{
			if (ImGui::Button("Symbols"))
			{
				LoadSymbolSubMenu(*this);
				menuIndex = miSymbols;
			}
			if (ImGui::Button("Backtrader"))
			{
				LoadBacktraderSubMenu(*this);
				menuIndex = miBacktrader;
			}
			if (ImGui::Button("Licensing"))
			{
				arena.DestroyScope(currentScope);
				menuIndex = miLicense;
			}
			if (ImGui::Button("Exit"))
			{
				arena.DestroyScope(currentScope);
				return true;
			}
		}
		else 
		{
			if (menuIndex == miLicense)
			{
				ImGui::Text("Alpha Vantage");
				if (ImGui::InputText("##", license, 17))
				{
					std::ofstream outFile(LICENSE_FILE_PATH); //"7HIFX74MVML11CUF"
					outFile << license << std::endl;
				}
			}
			if (menuIndex == miSymbols)
			{
				if (ImGui::Button("Reload"))
					LoadSymbolSubMenu(*this);
				if (ImGui::Button("Enable All"))
					for (auto& b : enabledSymbols)
						b = true;
				if(ImGui::Button("Disable All"))
					for (auto& b : enabledSymbols)
						b = false;
				if (ImGui::Button("Save changes"))
					SaveOrCreateEnabledSymbols(*this);
			}

			if (ImGui::Button("Back"))
				menuIndex = miMain;
		}
		
		ImGui::End();

		if (menuIndex == miBacktrader)
		{
			ImGui::Begin("Portfolio", nullptr, WIN_FLAGS);
			ImGui::SetWindowPos({ 200, 0 });
			ImGui::SetWindowSize({ 200, 400 });

			uint32_t index = 0;
			for (uint32_t i = 0; i < loadedSymbols.length; i++)
			{
				if (!enabledSymbols[i])
					continue;

				ImGui::PushID(i);
				ImGui::InputText("##", buffArr[index], 9, ImGuiInputTextFlags_CharsDecimal);
				ImGui::PopID();
				ImGui::SameLine();

				const bool selected = symbolIndex == i;
				if (selected)
					ImGui::PushStyleColor(ImGuiCol_Text, { 0, 1, 0, 1 });

				const auto symbol = loadedSymbols[i].c_str();
				if(ImGui::Button(symbol))
					LoadSymbol(*this, i);
				++index;

				if (selected)
					ImGui::PopStyleColor();
			}

			ImGui::End();
			TryRenderSymbol(*this);
		}

		if (menuIndex == miSymbols)
		{
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
					LoadSymbol(*this, i);

				if(selected)
					ImGui::PopStyleColor();
			}

			ImGui::End();

			TryRenderSymbol(*this);
		}

		ImGui::Begin("Output", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 0, 400 });
		ImGui::SetWindowSize({ 400, 200 });

		for (auto& a : output)
			ImGui::Text(a);

		ImGui::End();

		const bool ret = renderer.Render();
		frameArena.Clear();
		return ret;
	}
	STBT CreateSTBT()
	{
		STBT stbt{};
		stbt.tracker = {};
		stbt.graphType = 0;
		stbt.ma = 30;
		stbt.normalizeGraph = true;

		jv::gr::RendererCreateInfo createInfo{};
		createInfo.title = "STBT (Stock Trading Back Tester)";
		stbt.renderer = jv::gr::CreateRenderer(createInfo);
		stbt.menuIndex = 0;

		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		stbt.arena = Arena::Create(arenaCreateInfo);
		stbt.tempArena = Arena::Create(arenaCreateInfo);
		stbt.frameArena = Arena::Create(arenaCreateInfo);

		stbt.output = CreateQueue<const char*>(stbt.arena, 50);

		stbt.currentScope = stbt.arena.CreateScope();
		stbt.enabledSymbols = {};

		auto t = GetT();
		stbt.to = *std::gmtime(&t);
		t = GetT(30);
		stbt.from = *std::gmtime(&t);
		stbt.graphType = 0;

		std::ifstream f(LICENSE_FILE_PATH);
		if (f.good())
		{
			std::string line;
			getline(f, line);
			memcpy(stbt.license, line.c_str(), line.length());
		}

		std::string strLicense = stbt.license;
		if(strLicense == "")
			stbt.output.Add() = "WARNING: Missing licensing.";
			
		return stbt;
	}
	void DestroySTBT(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.currentScope);

		Arena::Destroy(stbt.frameArena);
		Arena::Destroy(stbt.tempArena);
		Arena::Destroy(stbt.arena);

		DestroyRenderer(stbt.renderer);
	}
}