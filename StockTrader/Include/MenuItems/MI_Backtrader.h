#pragma once
#include "MI_Main.h"
#include <Portfolio.h>
#include <Log.h>

namespace jv::bt
{
	struct RunInfo final
	{
		uint32_t range;
		uint32_t length;
		uint32_t buffer;
		uint32_t totalRuns;

		uint32_t from;
		uint32_t to;
	};

	class MI_Backtrader final : public MI_Main
	{
	public:
		Array<TimeSeries> timeSeries;
		Array<std::string> names;
		Array<bool> enabled;
		Array<char*> buffers;
		Portfolio portfolio;

		uint64_t subScope;
		uint32_t subIndex;
		uint32_t symbolIndex;
		bool normalizeGraph;

		uint32_t algoIndex;
		bool randomizeDate;
		char buffBuffer[8];
		char lengthBuffer[8];
		char zoomBuffer[8];
		char feeBuffer[8];
		char runCountBuffer[8];
		bool log;
		bool pauseOnFinish;
		bool pauseOnFinishAll;
		bool running;
		int runType;

		RunInfo runInfo;

		STBTTrade* trades;
		STBTScope stbtScope;
		uint32_t runIndex;
		uint32_t runDayIndex;

		// Scope while running one or more runs.
		uint64_t runningScope;
		// Scope for a single run.
		uint64_t runScope;
		Log runLog;
		bool stepCompleted;
		std::chrono::steady_clock::time_point tpStart;
		double timeElapsed;
		uint32_t highlightedGraphIndex;

		// Portfolio, Relative (Port to Stock Mark Average), Percentage, General (Average all runs)
		Array<jv::gr::GraphPoint> portPoints, relPoints, pctPoints, genPoints;
		Array<float> avrDeviations;
		std::chrono::system_clock::time_point runTimePoint;

		void Load(STBT& stbt) override;
		bool DrawMainMenu(STBT& stbt, uint32_t& index);
		bool DrawSubMenu(STBT& stbt, uint32_t& index);
		bool DrawFree(STBT& stbt, uint32_t& index);
		const char* GetMenuTitle();
		const char* GetSubMenuTitle();
		const char* GetDescription();
		void Unload(STBT& stbt) override;
		void BackTest(STBT& stbt, bool render);
		void DrawLog(STBT& stbt);

		void DrawPortfolioSubMenu(STBT& stbt);
		void DrawAlgorithmSubMenu(STBT& stbt);
		void DrawRunSubMenu(STBT& stbt);
		void RenderRun(STBT& stbt, const RunInfo& runInfo, bool& canFinish, bool& canEnd);
		void RenderGraphs(STBT& stbt, const RunInfo& runInfo, bool render);
	};
}