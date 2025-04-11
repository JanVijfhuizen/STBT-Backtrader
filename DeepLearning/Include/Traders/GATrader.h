#pragma once
#include <Algorithms/GeneticAlgorithm.h>

namespace jv
{
	struct GATrader final
	{
		jv::Arena* arena;
		jv::Arena* tempArena;
		jv::GeneticAlgorithm ga;
		bool training = true;
		const char* boolsNames = "Training";

		// Train instance info.
		float startV;
		uint32_t start, end;

		uint64_t tempScope;
		jv::Array<float> ma30;

		// GE info.
		uint32_t width = 30;
		uint32_t length = 20;
		float mutateChance = .2f;
		float mutateAddition = 1;
		float mutateMultiplier = .1f;

		[[nodiscard]] static GATrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};

	bool GATraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output);
	bool GATraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output);
	void GATraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output);

	void* GACreate(jv::Arena& arena, void* userPtr);
	void* GACopy(jv::Arena& arena, void* instance, void* userPtr);
	void GAMutate(jv::Arena& arena, void* instance, void* userPtr);
	void* GABreed(jv::Arena& arena, void* a, void* b, void* userPtr);
}
