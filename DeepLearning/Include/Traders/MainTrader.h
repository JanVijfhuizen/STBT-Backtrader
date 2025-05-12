#pragma once
#include <Traders/TMM.h>
#include <Traders/Modules/ModMA.h>
#include <Algorithms/GeneticAlgorithm.h>

namespace jv
{
	constexpr uint32_t MT_BUFFER_SIZE = 16;

	struct MainTrader final
	{
		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		ModMA modMA;
		tmm::Manager manager;

		bool training = true;
		const char* boolsNames = "Training";
		char loadFile[MT_BUFFER_SIZE] = "";
		char saveFile[MT_BUFFER_SIZE] = "";
		char* buffers[2];
		const char* bufferNames[2]
		{
			"LoadFile",
			"SaveFile"
		};
		uint32_t bufferSizes[2]
		{
			MT_BUFFER_SIZE,
			MT_BUFFER_SIZE
		};

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

		bool isFinalRun;

		[[nodiscard]] static MainTrader Create(Arena& arena, Arena& tempArena);
		static void Destroy(Arena& arena, MainTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
		void InitGA();
	};
}
