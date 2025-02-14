#pragma once
#include <Renderer.h>
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
		
		uint32_t menuIndex;
		uint32_t subMenuIndex;
		uint64_t currentScope;
		uint64_t subScope;

		Array<std::string> loadedSymbols;
		Array<bool> enabledSymbols;
		uint32_t symbolIndex;

		Array<std::string> scripts;
		std::string activeScript;
		lua_State* L;

		tm from, to;
		int graphType;
		uint32_t ma;
		Array<gr::GraphPoint> graphPoints;
		bool normalizeGraph;

		char buffer[8];
		char dayBuffer[8];
		char buffer3[8];
		char buffBuffer[8];
		char feeBuffer[8];
		char runBuffer[8];
		char license[32];
		bool randomizeDate, log;

		Array<char*> buffArr;
		Array<TimeSeries> timeSeriesArr;
		Array<glm::vec4> randColors;

		uint32_t runsQueued;

		__declspec(dllexport) bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT();
	__declspec(dllexport) void DestroySTBT(STBT& stbt);
}