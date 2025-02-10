#include "pch.h"
#include "STBT.h"
#include <Jlib/ArrayUtils.h>
#include "JLib/QueueUtils.h"

namespace jv::bt
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

	enum BTMenuIndex
	{
		btmiPortfolio,
		btmiScripts
	};

	void* MAlloc(const uint32_t size)
	{
		return malloc(size);
	}
	void MFree(void* ptr)
	{
		return free(ptr);
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

	static void LoadScripts(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.subScope);

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

		stbt.scripts = arr;
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

	static void LoadRandColors(STBT& stbt)
	{
		const uint32_t l = stbt.timeSeriesArr.length;
		stbt.randColors = CreateArray<glm::vec4>(stbt.arena, l);

		glm::vec4 predetermined[5]
		{
			glm::vec4(1, 0, 0, 1),
			glm::vec4(0, 0, 1, 1),
			glm::vec4(0, 1, 1, 1),
			glm::vec4(1, 1, 0, 1),
			glm::vec4(1, 0, 1, 1)
		};

		for (uint32_t i = 0; i < l; i++)
		{
			if (i < (sizeof(predetermined) / sizeof(glm::vec4)))
				stbt.randColors[i] = predetermined[i];
			else
				stbt.randColors[i] = glm::vec4(RandF(0, 1), RandF(0, 1), RandF(0, 1), 1);
		}
	}

	static void LoadSymbolSubMenu(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.currentScope);
		LoadSymbols(stbt);
		stbt.timeSeriesArr = CreateArray<TimeSeries>(stbt.arena, 1);
		LoadRandColors(stbt);
	}

	TimeSeries LoadSymbol(STBT& stbt, const uint32_t i)
	{
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
			auto timeSeries = stbt.tracker.ConvertDataToTimeSeries(stbt.arena, str);
			if (timeSeries.date != GetT())
				stbt.output.Add() = "WARNING: Symbol data is outdated.";
			return timeSeries;
		}
		return {};
	}

	static void LoadBacktraderSubMenu(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.currentScope);
		LoadSymbols(stbt);
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
		stbt.symbolIndex = -1;
		LoadRandColors(stbt);
		stbt.subScope = stbt.arena.CreateScope();
		stbt.subMenuIndex = 0;
	}

	static void RenderSymbolData(STBT& stbt)
	{
		auto tTo = mktime(&stbt.to);
		auto tFrom = mktime(&stbt.from);
		std::time_t tCurrent;
		uint32_t length = UINT32_MAX;

		for (uint32_t i = 0; i < stbt.timeSeriesArr.length; i++)
		{
			const auto& timeSeries = stbt.timeSeriesArr[i];
			std::time_t current = timeSeries.date;
			if (i == 0)
				tCurrent = current;
			else if (tCurrent != current)
			{
				stbt.output.Add() = "ERROR: Some symbol data is outdated.";
				return;
			}
			length = Min<uint32_t>(length, timeSeries.length);
		}

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

	STBT* gSTBT;
	uint32_t gSymInd;

	int gSetSymbol(lua_State* L)
	{
		const char* symbol = lua_tolstring(L, 0, nullptr);

		uint32_t index = 0;
		for (uint32_t i = 0; i < gSTBT->loadedSymbols.length; i++)
		{
			if (!gSTBT->enabledSymbols[i])
				continue;
			if (gSTBT->loadedSymbols[i] == symbol)
			{
				gSymInd = index;
				return 0;
			}
			index++;
		}
		return 0;
	}

	int gGet(lua_State* L, float* TimeSeries::* arr)
	{
		const auto& active = gSTBT->timeSeriesArr[gSymInd];
		lua_createtable(L, active.length, 0);
		int newTable = lua_gettop(L);

		for (uint32_t i = 0; i < active.length; i++)
		{
			const auto n = (active.*arr)[i];
			lua_pushnumber(L, n);
			lua_rawseti(L, newTable, i);
		}

		return 1;
	}

	int gGetOpen(lua_State* L)
	{
		return gGet(L, &TimeSeries::open);
	}

	int gGetClose(lua_State* L)
	{
		return gGet(L, &TimeSeries::close);
	}

	int gGetHigh(lua_State* L)
	{
		return gGet(L, &TimeSeries::high);
	}

	int gGetLow(lua_State* L)
	{
		return gGet(L, &TimeSeries::low);
	}

	int gGetLength(lua_State* L)
	{
		lua_pushnumber(L, gSTBT->timeSeriesArr[gSymInd].length);
		return 1;
	}

	void CloseLua(STBT& stbt)
	{
		if (!stbt.L)
			return;
		lua_close(stbt.L);
		stbt.L = nullptr;
	}

	void OpenLua(STBT& stbt)
	{
		// Needed for global functions.
		gSTBT = &stbt;
		CloseLua(stbt);
		stbt.L = luaL_newstate();
		luaL_openlibs(stbt.L);
		std::string f = "Scripts/" + stbt.activeScript + ".lua";
		luaL_dofile(stbt.L, f.c_str());
		lua_register(stbt.L, "SetSymbol", gSetSymbol);
		lua_register(stbt.L, "GetOpen", gGetOpen);
		lua_register(stbt.L, "GetClose", gGetClose);
		lua_register(stbt.L, "GetHigh", gGetHigh);
		lua_register(stbt.L, "GetLow", gGetLow);
		lua_register(stbt.L, "GetLength", gGetLength);

		// test.
		lua_getglobal(stbt.L, "TEST");
		lua_call(stbt.L, 0, 1);
		float result = (float)lua_tonumber(stbt.L, -1);
		lua_pop(stbt.L, 1);
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
			if (menuIndex == miBacktrader)
			{
				const char* buttons[]
				{
					"Portfolio",
					"Scripts"
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
			}

			if (ImGui::Button("Back"))
				menuIndex = miMain;
		}
		
		ImGui::End();

		if (menuIndex == miBacktrader)
		{
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
					ImGui::InputText("##", buffArr[index], 9, ImGuiInputTextFlags_CharsDecimal);
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
				ImGui::SetWindowPos({ 200, 0 });
				ImGui::SetWindowSize({ 200, 400 });

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
			}
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
				{
					LoadSymbolSubMenu(*this);
					timeSeriesArr[0] = LoadSymbol(*this, i);
				}

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
		
		stbt.L = nullptr;
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