#pragma once
#include <Traders/NNetTrader.h>

namespace jv
{
	struct NNetTraderTrainerCreateInfo final : NNetTraderCreateInfo
	{
		Array<uint32_t> timeFrames;
	};

	struct NNetTraderTrainer final : NNetTrader
	{
		const char* saveFile = "savefile";

		Array<uint32_t> timeFrames;

		uint32_t epochs = 25; // 25
		uint32_t batchSize = 200; // 20
		uint32_t maxEpochsWithoutProgress = 8;

		float epochHighestRating = 0;
		uint32_t lastEpochWithProgress = 0;
		uint32_t currentEpoch = 0;
		uint32_t currentBatch = 0;

		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		Array<jv::FPFNTester> testers;
		ai::DynNNet nnet;

		uint32_t stockId;
		float rating;
		float genRating;

		bt::STBTProgress progress;

		[[nodiscard]] static NNetTraderTrainer Create(Arena& arena, Arena& tempArena, const NNetTraderTrainerCreateInfo& info);
		static void Destroy(Arena& arena, NNetTraderTrainer& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}