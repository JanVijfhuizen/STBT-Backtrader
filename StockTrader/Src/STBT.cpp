#include "pch.h"
#include "STBT.h"

namespace jv::ai
{
	bool STBT::Update()
	{
		return false;
	}
	STBT CreateSTBT()
	{
		STBT stbi{};

		jv::gr::RendererCreateInfo createInfo{};
		createInfo.title = "STBI (Stock Trading Back Tester)";
		stbi.renderer = jv::gr::CreateRenderer(createInfo);

		return stbi;
	}
	void DestroySTBT(const STBT& stbt)
	{
		DestroyRenderer(stbt.renderer);
	}
}