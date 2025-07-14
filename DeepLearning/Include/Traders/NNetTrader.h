#pragma once
#include <Algorithms/DynNNet.h>

namespace jv
{
	struct NNetTrader final
	{
		uint32_t epochs = 10;
		uint32_t currentEpoch = 0;

		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		ai::DynNNet nnet;

		uint64_t runScope;
		uint32_t stockId;

		jv::FPFNTester tester;
		float rating;
		float genRating;

		[[nodiscard]] static NNetTrader Create(Arena& arena, Arena& tempArena);
		static void Destroy(Arena& arena, NNetTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}


