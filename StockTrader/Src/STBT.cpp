#include "pch.h"
#include "STBT.h"
#include <Jlib/ArrayUtils.h>

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

	static void SaveOrCreateEnabledSymbols(STBT& stbt) 
	{
		const std::string path = "Symbols/enabled.txt";
		std::ofstream fout(path);

		if (stbt.enabledSymbols.length != stbt.loadedSymbols.length)
		{
			stbt.enabledSymbols = jv::CreateArray<bool>(stbt.arena, stbt.loadedSymbols.length);
			for(auto& b : stbt.enabledSymbols)
				b = true;
		}

		for (const auto enabled : stbt.enabledSymbols)
			fout << enabled << std::endl;
		fout.close();
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
		stbt.symbolIndex = -1;

		LoadEnabledSymbols(stbt);
	}

	static void LoadSymbolSubMenu(STBT& stbt)
	{
		stbt.arena.DestroyScope(stbt.currentScope);
		LoadSymbols(stbt);
	}

	bool STBT::Update()
	{
		ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		ImGui::SetWindowPos({0, 0});
		ImGui::SetWindowSize({200, renderer.resolution.y});

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
			ImGui::Begin("List of symbols", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
			ImGui::SetWindowPos({ 200, 0 });
			ImGui::SetWindowSize({ 200, renderer.resolution.y });

			if (ImGui::Button("Add")) 
			{
				const auto tempScope = tempArena.CreateScope();
				tracker.GetData(tempArena, buffer, "Symbols/");
				tempArena.DestroyScope(tempScope);
				
				LoadSymbolSubMenu(*this);
			}
			ImGui::SameLine();
			ImGui::InputText("#", buffer, 5);

			for (uint32_t i = 0; i < loadedSymbols.length; i++)
			{
				ImGui::PushID(i);
				ImGui::Checkbox("", &enabledSymbols[i]);
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::Button(loadedSymbols[i].c_str());
			}

			ImGui::End();
		}

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
		stbt.currentScope = stbt.arena.CreateScope();

		stbt.enabledSymbols = {};
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