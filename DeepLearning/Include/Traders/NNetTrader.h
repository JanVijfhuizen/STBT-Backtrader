#pragma once
#include <Algorithms/DynNNet.h>

namespace jv
{
	struct NNetTrader final
	{
		uint32_t epochs = 1; // 10
		uint32_t currentEpoch = 0;

		uint32_t batchSize = 1; // 5
		uint32_t currentBatch = 0;

		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		ai::DynNNet nnet;

		uint32_t stockId;

		jv::FPFNTester tester;
		float rating;
		float genRating;

		[[nodiscard]] static NNetTrader Create(Arena& arena, Arena& tempArena);
		static void Destroy(Arena& arena, NNetTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}


