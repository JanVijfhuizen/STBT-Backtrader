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
		Arena arena, tempArena, frameArena;
		uint32_t menuIndex;
		uint64_t currentScope;

		Array<std::string> loadedSymbols;

		__declspec(dllexport) bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT();
	__declspec(dllexport) void DestroySTBT(STBT& stbt);
}