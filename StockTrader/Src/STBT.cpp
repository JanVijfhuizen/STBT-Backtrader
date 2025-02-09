#include "pch.h"
#include "STBT.h"
#include <Jlib/ArrayUtils.h>
#include "JLib/QueueUtils.h"

namespace jv::ai
{
	enum MenuIndex 
	{
		miMain,
		miSymbols,
		miBacktrader,
		miTrain,
		miUse
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
				points.ptr, points.length, gr::GraphType::line, true, glm::vec4(0, 1, 0, 1));
		}

		stbt.renderer.graphBorderThickness = 0;
		stbt.renderer.DrawGraph({ .5, 0 }, 
			glm::vec2(stbt.renderer.GetAspectRatio(), 1), 
			points.ptr, points.length, static_cast<gr::GraphType>(stbt.graphType), true);

		stbt.graphPoints = points;
	}

	bool STBT::Update()
	{
		const auto winFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
		ImGui::Begin("Menu", nullptr, winFlags);
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
		case miSymbols:
			title = "Symbols";
			description = "Debug symbols, (un)load \nthem and add new ones.";
			break;
		case miBacktrader:
			title = "Back Trader";
			break;
		case miTrain:
			title = "Train";
			break;
		case miUse:
			title = "Use";
			break;
		}
		ImGui::Text(title);
		ImGui::Text(description);

		if (menuIndex == miMain)
		{
			if (ImGui::Button("Symbols"))
			{
				LoadSymbolSubMenu(*this);
				menuIndex = miSymbols;
			}
			if (ImGui::Button("Backtrader"))
			{
				arena.DestroyScope(currentScope);
				menuIndex = miBacktrader;
			}
			if (ImGui::Button("Train"))
			{
				arena.DestroyScope(currentScope);
				menuIndex = miTrain;
			}	
			if (ImGui::Button("Use"))
			{
				arena.DestroyScope(currentScope);
				menuIndex = miUse;
			}	
			if (ImGui::Button("Exit"))
			{
				arena.DestroyScope(currentScope);
				return true;
			}
		}
		else 
		{
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

		if (menuIndex == miSymbols)
		{
			ImGui::Begin("List of symbols", nullptr, winFlags);
			ImGui::SetWindowPos({ 200, 0 });
			ImGui::SetWindowSize({ 200, 400 });

			if (ImGui::Button("Add")) 
			{
				const auto tempScope = tempArena.CreateScope();
				tracker.GetData(tempArena, buffer, "Symbols/");
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
					symbolIndex = i;

					const auto str = tracker.GetData(tempArena, loadedSymbols[i].c_str(), "Symbols/");
					// If the data is invalid.
					if (str[0] == '{')
					{
						symbolIndex = -1;
						output.Add() = "ERROR: No valid symbol data found.";
					}
					else
					{
						timeSeries = tracker.ConvertDataToTimeSeries(arena, str);
						if (timeSeries.date != GetT())
						{
							output.Add() = "WARNING: Symbol data is outdated.";
						}
					}
				}

				if(selected)
					ImGui::PopStyleColor();
			}

			ImGui::End();

			if (symbolIndex != -1)
			{
				RenderSymbolData(*this);

				ImGui::Begin("Settings", nullptr, winFlags);
				ImGui::SetWindowPos({ 400, 0 });
				ImGui::SetWindowSize({ 400, 124 });
				ImGui::DatePicker("Date 1", from);
				ImGui::DatePicker("Date 2", to);

				const char* items[]{ "Line","Candles" };
				bool check = ImGui::Combo("Graph Type", &graphType, items, 2);

				if (ImGui::Button("Days"))
				{
					const int i = std::atoi(buffer2);
					if (i < 1)
					{
						output.Add() = "ERROR: Invalid number of days given.";
					}
					else 
					{
						auto t = GetT();
						to = *std::gmtime(&t);
						t = GetT(i);
						from = *std::gmtime(&t);
					}
				}

				ImGui::SameLine();
				ImGui::PushItemWidth(40);
				ImGui::InputText("##", buffer2, 5, ImGuiInputTextFlags_CharsDecimal);
				ImGui::SameLine();
				ImGui::InputText("MA", buffer3, 5, ImGuiInputTextFlags_CharsDecimal);
				ma = std::atoi(buffer3);
				ImGui::PopItemWidth();

				ImGui::End();

				std::string title = "Details: ";
				title += loadedSymbols[symbolIndex];
				ImGui::Begin(title.c_str(), nullptr, winFlags);
				ImGui::SetWindowPos({ 400, 500 });
				ImGui::SetWindowSize({ 400, 100 });

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

		ImGui::Begin("Output", nullptr, winFlags);
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