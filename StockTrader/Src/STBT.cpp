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

	static void RenderSymbolData(STBT& stbt)
	{
		const auto& timeSeries = stbt.timeSeries;

		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::time_t tCurrent = std::chrono::system_clock::to_time_t(now);

		auto tTo = mktime(&stbt.to);
		auto tFrom = mktime(&stbt.from);
		if (tFrom > tTo)
		{
			auto tTemp = tTo;
			tTo = tFrom;
			tFrom = tTemp;

			stbt.from = *std::gmtime(&tFrom);
			stbt.to = *std::gmtime(&tTo);
		}
		if (tTo > tCurrent)
		{
			tTo = tCurrent;
			stbt.to = *std::gmtime(&tCurrent);
		}

		auto diff = difftime(tTo, tFrom);
		diff = Min<double>(diff, (timeSeries.length - 1) * 60 * 60 * 24);
		auto orgDiff = Max<double>(difftime(tCurrent, tTo), 0);
		uint32_t daysDiff = diff / 60 / 60 / 24;
		uint32_t daysOrgDiff = orgDiff / 60 / 60 / 24;

		auto points = CreateArray<jv::gr::GraphPoint>(stbt.frameArena, daysDiff);

		for (uint32_t i = 0; i < daysDiff; i++)
		{
			points[i].open = timeSeries.open[i + daysOrgDiff];
			points[i].close = timeSeries.close[i + daysOrgDiff];
			points[i].high = timeSeries.high[i + daysOrgDiff];
			points[i].low = timeSeries.low[i + daysOrgDiff];
		}
		stbt.renderer.DrawGraph({ -.5, 0 }, glm::vec2(stbt.renderer.GetAspectRatio(), 1), points.ptr, points.length, jv::gr::GraphType::line, false);
		stbt.renderer.DrawGraph({ .5, 0 }, glm::vec2(stbt.renderer.GetAspectRatio(), 1), points.ptr, points.length, jv::gr::GraphType::candle, false);
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
				for (auto& symbol : loadedSymbols)
					index += symbol < s;

				auto arr = CreateArray<bool>(arena, enabledSymbols.length + 1);
				for (uint32_t i = 0; i < enabledSymbols.length; i++)
					arr[i + (i >= index)] = enabledSymbols[i];
				arr[index] = false;

				enabledSymbols = arr;
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
						output.Add() = "ERROR: No stock data found for given symbol:";
						output.Add() = loadedSymbols[i].c_str();
					}
					else
						timeSeries = tracker.ConvertDataToTimeSeries(arena, str);
				}
			}

			ImGui::End();

			if (symbolIndex != -1)
			{
				RenderSymbolData(*this);

				ImGui::Begin("Stock Analysis", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
				ImGui::SetWindowPos({ 400, 0 });
				ImGui::SetWindowSize({ 400, 80 });
				ImGui::DatePicker("Date 1", from);
				ImGui::DatePicker("Date 2", to);
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

		using namespace std::chrono_literals;
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		auto past = now - 60s * 60 * 24 * 14;
		std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
		std::time_t pastTime = std::chrono::system_clock::to_time_t(past);
		stbt.to = *std::gmtime(&currentTime);
		stbt.from = *std::gmtime(&pastTime);
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