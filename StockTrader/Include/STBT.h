#pragma once
#include <Graphics/Renderer.h>
#include "JLib/Queue.h"
#include "Portfolio.h"

namespace jv::bt
{
	// Local scope that the bots can access.
	struct STBTScope final
	{
	public:
		// Get the money in the bank.
		__declspec(dllexport) [[nodiscard]] float GetLiquidity() const;
		// Get the value of the current portfolio
		__declspec(dllexport) [[nodiscard]] float GetPortValue(uint32_t current) const;
		// Get the number of stocks of target symbol I in the portfolio. 
		__declspec(dllexport) [[nodiscard]] uint32_t GetNInPort(uint32_t index) const;
		// Get a timeseries for target symbol.
		__declspec(dllexport) [[nodiscard]] TimeSeries GetTimeSeries(uint32_t index) const;
		// Get the length of the scope (days).
		__declspec(dllexport) [[nodiscard]] uint32_t GetLength() const;
		// Get the amount of unique symbols.
		__declspec(dllexport) [[nodiscard]] uint32_t GetTimeSeriesCount() const;

		[[nodiscard]] static STBTScope Create(Portfolio* portfolio, Array<TimeSeries> timeSeries);

	private:
		Portfolio* portfolio;
		Array<TimeSeries> timeSeries;
	};

	struct STBTTrade final
	{
		int32_t change = 0;
	};

	// AI stock trader. Can be tested in the STBT program.
	struct STBTBot final
	{
		const char* name = "NO NAME GIVEN";
		const char* author = "NO AUTHOR GIVEN";
		const char* description = "NO DESCRIPTION GIVEN";

		// Executes once at the start of a run.
		bool(*init)(const STBTScope& scope, void* userPtr, uint32_t runIndex, 
			uint32_t runCount, Queue<const char*>& output) = nullptr;
		// Executes every day in a run.
		bool(*update)(const STBTScope& scope, STBTTrade* trades, uint32_t current, 
			void* userPtr, Queue<const char*>& output);
		// Executes at the end of a run.
		void(*cleanup)(const STBTScope& scope, void* userPtr, Queue<const char*>& output) = nullptr;
		// A custom pointer can be given here.
		void* userPtr = nullptr;

		// Custom variables that you can give to the backtester.
		bool* bools;
		const char** boolsNames;
		uint32_t boolsLength = 0;
		char** buffers;
		const char** buffersNames;
		uint32_t* bufferSizes;
		uint32_t buffersLength = 0;
	};

	struct STBTCreateInfo final
	{
		// Stock symbols.
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

		bool enableTutorialMode;

		char license[32];
		tm from, to;
		int graphType;
		uint32_t days;
		// Moving Average.
		uint32_t ma;

		__declspec(dllexport) bool Update();
	};

	__declspec(dllexport) [[nodiscard]] STBT CreateSTBT(STBTBot* bots, uint32_t botCount);
	__declspec(dllexport) void DestroySTBT(STBT& stbt);
}