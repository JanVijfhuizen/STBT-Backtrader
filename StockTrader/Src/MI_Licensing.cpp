#include "pch.h"
#include "MenuItems/MI_Licensing.h"

namespace jv::bt
{
	const char* LICENSE_FILE_PATH = "license.txt";

	void MI_Licensing::Init(Arena& arena, STBT& stbt)
	{
		std::ifstream f(LICENSE_FILE_PATH);
		if (f.good())
		{
			std::string line;
			getline(f, line);
			memcpy(stbt.license, line.c_str(), line.length());
		}

		std::string strLicense = stbt.license;
		if (strLicense == "")
			stbt.output.Add() = "WARNING: Missing licensing.";
	}

	void MI_Licensing::Load(STBT& stbt)
	{
		
	}

	bool MI_Licensing::DrawMainMenu(STBT& stbt, uint32_t& index)
	{
		return false;
	}

	bool MI_Licensing::DrawSubMenu(STBT& stbt, uint32_t& index)
	{
		ImGui::Text("Alpha Vantage");
		if (ImGui::InputText("##", stbt.license, 17))
		{
			std::ofstream outFile(LICENSE_FILE_PATH); //"7HIFX74MVML11CUF"
			outFile << stbt.license << std::endl;
		}

		return false;
	}

	const char* MI_Licensing::GetMenuTitle()
	{
		return "Licensing";
	}

	const char* MI_Licensing::GetSubMenuTitle()
	{
		return "Licenses";
	}

	const char* MI_Licensing::GetDescription()
	{
		return "NOTE THAT THIS PROGRAM DOES \nNOT REQUIRE ANY LICENSING. \nLicenses however, are \nrequired for some external \nAPI calls.";
	}

	void MI_Licensing::Unload(STBT& stbt)
	{
	}
}