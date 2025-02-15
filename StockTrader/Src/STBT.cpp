#include "pch.h"
#include "STBT.h"
#include <Jlib/ArrayUtils.h>
#include "JLib/QueueUtils.h"
#include <MenuItems/MI_MainMenu.h>
#include <MenuItems/MI_Symbols.h>
#include <MenuItems/MI_Licensing.h>
#include <MenuItems/MI_Backtrader.h>
#include <Utils/UT_Time.h>

namespace jv::bt
{
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
			if (timeSeries.date != GetTime())
				stbt.output.Add() = "WARNING: Symbol data is outdated.";
			return timeSeries;
		}
		return {};
	}

	static bool GetMaxLength(STBT& stbt, std::time_t& tCurrent, 
		uint32_t& length, const uint32_t buffer)
	{
		length = UINT32_MAX;

		for (uint32_t i = 0; i < stbt.timeSeriesArr.length; i++)
		{
			const auto& timeSeries = stbt.timeSeriesArr[i];
			std::time_t current = timeSeries.date;
			if (i == 0)
				tCurrent = current;
			else if (tCurrent != current)
			{
				stbt.output.Add() = "ERROR: Some symbol data is outdated.";
				return false;
			}
			length = Min<uint32_t>(length, timeSeries.length);
		}
		length = Max(buffer, length);
		length -= buffer;
		return true;
	}

	static void ClampDates(STBT& stbt, std::time_t& tFrom, std::time_t& tTo, 
		std::time_t& tCurrent, uint32_t& length, const uint32_t buffer)
	{
		if (!GetMaxLength(stbt, tCurrent, length, buffer))
			return;

		tFrom = mktime(&stbt.from);
		tTo = mktime(&stbt.to);

		auto minTime = tTo > tFrom ? tTo : tFrom;
		minTime -= (60 * 60 * 24) * length;
		auto& floor = tTo > tFrom ? tFrom : tTo;
		if (floor < minTime)
		{
			floor = minTime;
			(tTo > tFrom ? stbt.from : stbt.to) = *std::gmtime(&floor);
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
	}

	static void RenderSymbolData(STBT& stbt)
	{
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
	}
	/*
	STBT* gSTBT;
	int gId;

	int gGetSymbolIndex(lua_State* L)
	{
		const char* symbol = lua_tolstring(L, 0, nullptr);

		uint32_t index = 0;
		for (uint32_t i = 0; i < gSTBT->loadedSymbols.length; i++)
		{
			if (!gSTBT->enabledSymbols[i])
				continue;
			if (gSTBT->loadedSymbols[i] == symbol)
			{
				lua_pushnumber(L, index);
				return 1;
			}
			index++;
		}
		lua_pushnumber(L, -1);
		return 1;
	}

	int gSetIndex(lua_State* L)
	{
		gId = lua_tonumber(L, 0);
		return 0;
	}

	int gGet(lua_State* L, float* TimeSeries::* arr)
	{
		const auto& active = gSTBT->timeSeriesArr[gId];
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
		lua_pushnumber(L, gSTBT->timeSeriesArr[gId].length);
		return 1;
	}

	int gGetVolume(lua_State* L)
	{
		const auto& active = gSTBT->timeSeriesArr[gId];
		lua_createtable(L, active.length, 0);
		int newTable = lua_gettop(L);

		for (uint32_t i = 0; i < active.length; i++)
		{
			const auto n = (active.volume)[i];
			lua_pushnumber(L, n);
			lua_rawseti(L, newTable, i);
		}

		return 1;
	}

	int gBuy(lua_State* L)
	{
		return 0;
	}

	int gSell(lua_State* L)
	{
		return 0;
	}

	int gGetNumInPort(lua_State* L)
	{
		return 1;
	}

	int gGetLiquidity(lua_State* L) 
	{
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
		lua_register(stbt.L, "GetSymbolId", gGetSymbolIndex);
		lua_register(stbt.L, "SetId", gSetIndex);
		lua_register(stbt.L, "GetOpen", gGetOpen);
		lua_register(stbt.L, "GetClose", gGetClose);
		lua_register(stbt.L, "GetHigh", gGetHigh);
		lua_register(stbt.L, "GetLow", gGetLow);
		lua_register(stbt.L, "GetLength", gGetLength);
		lua_register(stbt.L, "GetVolume", gGetVolume);
		lua_register(stbt.L, "Buy", gBuy);
		lua_register(stbt.L, "Sell", gSell);
		lua_register(stbt.L, "GetLiquidity", gGetLiquidity);
		lua_register(stbt.L, "GetNumInPort", gGetNumInPort);
	}

	static void ExecuteRun(STBT& stbt)
	{
		--stbt.runsQueued;

		ImGui::Begin("Active Run", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 300, 225 });
		ImGui::SetWindowSize({ 200, 150 });

		ImGui::Text("Running...");

		time_t current;
		uint32_t length;
		uint32_t buffer = std::atoi(stbt.buffBuffer);
		const auto days = std::atoi(stbt.dayBuffer);
		GetMaxLength(stbt, current, length, buffer);

		if (stbt.randomizeDate)
		{
			uint32_t startOffset = rand() % (length - days);
			auto a = GetT(startOffset);
			auto b = GetT(startOffset + days);
			stbt.from = *std::gmtime(&a);
			stbt.to = *std::gmtime(&b);
		}

		stbt.liquidity = std::atof(stbt.buffArr[0]);
		for (uint32_t i = 0; i < stbt.portfolio.length; i++)
			int32_t n = std::atoi(stbt.buffArr[i + 1]);

		//lua_getglobal(stbt.L, "PRE");

		for (uint32_t i = 0; i < length - buffer; i++)
		{

		}

		ImGui::End();
	}
	*/

	bool STBT::Update()
	{
		/*
		if (runsQueued > 0)
		{
			ExecuteRun(*this);
			ImGui::Render();
			const bool ret = renderer.Render();
			frameArena.Clear();
			return ret;
		}
		*/

		bool quit = menu.Update(*this);
		if (quit)
			return true;

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

		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		stbt.arena = Arena::Create(arenaCreateInfo);
		stbt.tempArena = Arena::Create(arenaCreateInfo);
		stbt.frameArena = Arena::Create(arenaCreateInfo);

		stbt.output = CreateQueue<const char*>(stbt.arena, 50);
		stbt.enabledSymbols = {};

		auto t = GetTime();
		stbt.to = *std::gmtime(&t);
		t = GetTime(DAYS_DEFAULT);
		stbt.from = *std::gmtime(&t);
		stbt.graphType = 0;
		
		stbt.L = nullptr;
		snprintf(stbt.feeBuffer, sizeof(stbt.feeBuffer), "%f", 1e-3f);
		snprintf(stbt.buffBuffer, sizeof(stbt.buffBuffer), "%i", 0);
		snprintf(stbt.dayBuffer, sizeof(stbt.dayBuffer), "%i", DAYS_DEFAULT);
		snprintf(stbt.runBuffer, sizeof(stbt.runBuffer), "%i", 1);
		stbt.randomizeDate = false;
		stbt.log = true;
		stbt.runsQueued = 0;

		auto& menu = stbt.menu = Menu<STBT>::CreateMenu(stbt.arena, 4);
		menu.Add() = stbt.arena.New<MI_MainMenu>();
		menu.Add() = stbt.arena.New<MI_Symbols>();
		menu.Add() = stbt.arena.New<MI_Backtrader>();
		menu.Add() = stbt.arena.New<MI_Licensing>();
		menu.SetIndex(stbt.arena, stbt, 0);
		return stbt;
	}
	void DestroySTBT(STBT& stbt)
	{
		Arena::Destroy(stbt.frameArena);
		Arena::Destroy(stbt.tempArena);
		Arena::Destroy(stbt.arena);

		DestroyRenderer(stbt.renderer);
	}
}