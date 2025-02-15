#pragma once
#include <Graphics/Renderer.h>
#include "JLib/Queue.h"

namespace jv::bt
{
	struct STBTCreateInfo final
	{
		const char** symbols;
		uint32_t symbolsLength;
	};

	struct STBT final
	{
		gr::Renderer renderer;
		Tracker tracker;

		Arena arena, tempArena, frameArena;

		Queue<const char*> output;
		Menu<STBT> menu;

		char license[32];

		tm from, to;
		int graphType;
		uint32_t days;
		uint32_t ma;
		
		/*
		uint32_t subMenuIndex;
		uint64_t subScope;

		Array<std::string> scripts;
		std::string activeScript;
		lua_State* L;

		char buffer3[8];
		char buffBuffer[8];
		char feeBuffer[8];
		char runBuffer[8];
		bool randomizeDate, log;

		Array<char*> buffArr;

		uint32_t runsQueued;
		Array<uint32_t> portfolio;
		float liquidity;
		*/

		__declspec(dllexport) bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT();
	__declspec(dllexport) void DestroySTBT(STBT& stbt);
}