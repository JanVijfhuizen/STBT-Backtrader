#pragma once
#include <Renderer.h>

namespace jv::ai 
{
	struct STBTCreateInfo final
	{
		const char** symbols;
		uint32_t symbolsLength;
	};

	struct STBT final
	{
		jv::gr::Renderer renderer;

		bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT();
	__declspec(dllexport) void DestroySTBT(const STBT& stbt);
}