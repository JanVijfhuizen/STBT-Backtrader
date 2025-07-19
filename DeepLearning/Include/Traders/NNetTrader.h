#pragma once
#include <Algorithms/DynNNet.h>

namespace jv
{
	struct NNetTraderMod final
	{
		void (*init)(const bt::STBTBotInfo& info, uint32_t stockId, void* userPtr);
		void (*update)(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t current, float* out, void* userPtr);
		uint32_t(*getMinBufferSize)(const bt::STBTBotInfo& info, uint32_t stockId, void* userPtr) = nullptr;

		uint32_t outputCount = 1;
		void* userPtr = nullptr;
	};

	struct NNetTrader final
	{
		uint32_t epochs = 10;
		uint32_t currentEpoch = 0;

		uint32_t batchSize = 5;
		uint32_t currentBatch = 0;

		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		Array<NNetTraderMod> mods;
		ai::DynNNet nnet;

		uint32_t stockId;

		jv::FPFNTester tester;
		float rating;
		float genRating;

		[[nodiscard]] static NNetTrader Create(Arena& arena, Arena& tempArena, const Array<NNetTraderMod>& mods);
		static void Destroy(Arena& arena, NNetTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};

	[[nodiscard]] NNetTraderMod NNetGetDefaultMod();
}


