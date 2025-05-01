#pragma once
#include <Traders/TMM.h>
#include <Traders/Modules/ModMA.h>
#include <Algorithms/GeneticAlgorithm.h>

namespace jv
{
	struct MainTrader final
	{
		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		ModMA modMA;
		tmm::Manager manager;

		bool training = true;
		const char* boolsNames = "Training";

		// Optimizer
		GeneticAlgorithm ga;
		uint32_t width = 4;
		uint32_t length = 20;
		float mutateChance = .2f;
		float mutateAddition = 1;
		float mutateMultiplier = .1f;

		uint32_t runsPerInstance = 10;
		uint32_t currentInstanceRun;
		float rating;

		float startV;
		uint32_t end;

		[[nodiscard]] static MainTrader Create(Arena& arena, Arena& tempArena);
		static void Destroy(Arena& arena, MainTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
		void InitGA();
	};
}
