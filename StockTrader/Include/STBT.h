#pragma once
#include <Renderer.h>
#include "JLib/Queue.h"

namespace jv::ai 
{
	struct STBTCreateInfo final
	{
		const char** symbols;
		uint32_t symbolsLength;
	};

	struct STBT final
	{
		gr::Renderer renderer;
		bt::Tracker tracker;

		Arena arena, tempArena, frameArena;
		Queue<const char*> output;
		
		uint32_t menuIndex;
		uint64_t currentScope;

		Array<std::string> loadedSymbols;
		Array<bool> enabledSymbols;
		bt::TimeSeries timeSeries;
		uint32_t symbolIndex;

		char buffer[8];

		__declspec(dllexport) bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT();
	__declspec(dllexport) void DestroySTBT(STBT& stbt);
}