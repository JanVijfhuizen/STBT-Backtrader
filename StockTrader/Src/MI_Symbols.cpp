#include "pch.h"
#include "MenuItems/MI_Symbols.h"
#include <JLib/ArrayUtils.h>
#include <MenuItems/MI_Utils.h>

namespace jv::bt
{
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

	void MI_Symbols::Load(STBT& stbt)
	{
		LoadSymbols(stbt);
		stbt.timeSeriesArr = CreateArray<TimeSeries>(stbt.arena, 1);
		LoadRandColors(stbt);
	}

	void MI_Symbols::Update(STBT& stbt)
	{
	}

	void MI_Symbols::Unload(STBT& stbt)
	{
	}
	void MI_Symbols::LoadSymbols(STBT& stbt)
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
	void MI_Symbols::LoadEnabledSymbols(STBT& stbt)
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
}