#include "pch.h"
#include "MenuItems/MI_Licensing.h"

namespace jv::bt
{
	const char* LICENSE_FILE_PATH = "license.txt";

	void MI_Licensing::Load(STBT& stbt)
	{
	}

	void MI_Licensing::Update(STBT& stbt)
	{
		ImGui::Text("Alpha Vantage");
		if (ImGui::InputText("##", stbt.license, 17))
		{
			std::ofstream outFile(LICENSE_FILE_PATH); //"7HIFX74MVML11CUF"
			outFile << stbt.license << std::endl;
		}
	}

	void MI_Licensing::Unload(STBT& stbt)
	{
	}
}