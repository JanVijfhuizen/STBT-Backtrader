#pragma once
#include <Graphics/Renderer.h>
#include "JLib/Queue.h"
#include "Portfolio.h"

namespace jv::bt
{
	struct STBTScope final
	{
	public:
		[[nodiscard]] float GetLiquidity() const;
		[[nodiscard]] uint32_t GetNInPort(uint32_t index) const;
		[[nodiscard]] TimeSeries GetTimeSeries(uint32_t index) const;
		[[nodiscard]] uint32_t GetLength() const;
		[[nodiscard]] uint32_t GetTimeSeriesCount() const;

		[[nodiscard]] static STBTScope Create(Portfolio* portfolio, Array<TimeSeries> timeSeries);

	private:
		Portfolio* portfolio;
		Array<TimeSeries> timeSeries;
	};

	struct STBTTrade final
	{
		int32_t change = 0;
	};

	struct STBTBot final
	{
		const char* name = "NO NAME GIVEN";
		const char* author = "NO AUTHOR GIVEN";
		const char* description = "NO DESCRIPTION GIVEN";

		void(*init)(const STBTScope& scope, void* userPtr) = nullptr;
		void(*update)(const STBTScope& scope, STBTTrade* trades, uint32_t current, void* userPtr);
		void(*cleanup)(const STBTScope& scope, void* userPtr) = nullptr;
		void* userPtr = nullptr;
	};

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

		Array<STBTBot> bots;

		char license[32];
		tm from, to;
		int graphType;
		uint32_t days;
		uint32_t ma;

		__declspec(dllexport) bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT(STBTBot* bots, uint32_t botCount);
	__declspec(dllexport) void DestroySTBT(STBT& stbt);
}