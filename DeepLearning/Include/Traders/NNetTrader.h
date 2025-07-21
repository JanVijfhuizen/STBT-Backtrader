#pragma once
#include <Algorithms/DynNNet.h>

namespace jv
{
	struct NNetTraderDefaultMod final
	{
		float multiplier;
	};

	struct NNetTraderMod final
	{
		void (*init)(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t warmup, void* userPtr);
		void (*update)(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t current, float* out, void* userPtr);
		uint32_t(*getMinBufferSize)(const bt::STBTBotInfo& info, uint32_t stockId, void* userPtr) = nullptr;

		uint32_t outputCount = 1;
		void* userPtr = nullptr;
	};

	struct NNetTraderCreateInfo final
	{
		Array<NNetTraderMod> mods;
		Array<uint32_t> timeFrames;
	};

	struct NNetTrader final
	{
		NNetTraderCreateInfo info;

		uint32_t epochs = 25;
		uint32_t batchSize = 20;
		uint32_t maxCyclesWithoutProgress = 5;

		float cycleHighestRating = 0;
		uint32_t lastCycleWithProgress = 0;
		uint32_t currentEpoch = 0;
		uint32_t currentBatch = 0;

		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		Array<NNetTraderMod> mods;
		Array<uint32_t> timeFrames;
		Array<jv::FPFNTester> testers;
		ai::DynNNet nnet;

		uint32_t stockId;
		float rating;
		float genRating;

		[[nodiscard]] static NNetTrader Create(Arena& arena, Arena& tempArena, const NNetTraderCreateInfo& info);
		static void Destroy(Arena& arena, NNetTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};

	[[nodiscard]] NNetTraderMod NNetGetDefaultMod(NNetTraderDefaultMod& out);
}


