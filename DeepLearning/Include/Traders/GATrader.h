#pragma once
#include <Algorithms/GeneticAlgorithm.h>

namespace jv
{
	struct GATrader final
	{
		jv::Arena* arena;
		jv::Arena* tempArena;
		jv::GeneticAlgorithm ga;

		// Train instance info.
		float startV;
		uint32_t start, end;

		uint64_t tempScope;
		float* ma30;
		uint32_t score;
		float* correctness;
		bool running;

		const char* useSpeciationText = "Use Speciation";
		bool useSpeciation = false;

		// GE info.
		uint32_t width = 400;
		uint32_t length = 100;
		float mutateChance = .01f;
		float mutateAddition = 1;
		float mutateMultiplier = .1f;

		[[nodiscard]] static GATrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};

	void* GACreate(jv::Arena& arena, void* userPtr);
	void* GACopy(jv::Arena& arena, void* instance, void* userPtr);
	void GAMutate(jv::Arena& arena, void* instance, void* userPtr);
	void* GABreed(jv::Arena& arena, void* a, void* b, void* userPtr);
}
