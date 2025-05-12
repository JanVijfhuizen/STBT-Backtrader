#pragma once

namespace jv
{
	struct GeneticAlgorithmCreateInfo final
	{
		uint32_t length;

		void* (*create)(Arena& arena, void* userPtr);
		void* (*copy)(Arena& arena, void* a, void* userPtr);
		void (*mutate)(Arena& arena, void* instance, void* userPtr);
		void* (*breed)(Arena& arena, void* a, void* b, void* userPtr);
		
		float surviverPct = .4f;
		float arrivalsPct = .1f;
		void* userPtr = nullptr;
	};

	struct GeneticAlgorithm final
	{
		GeneticAlgorithmCreateInfo info;

		uint64_t scope;
		uint64_t resScope;
		uint64_t genScope;

		void** generation;
		void* result;
		float* genRatings;
		float rating;
		uint32_t genId;
		uint32_t trainId;

		bool debug;

		[[nodiscard]] void* GetTrainee();
		void Rate(Arena& arena, Arena& tempArena, float rating, Queue<bt::OutputMsg>& output);

		[[nodiscard]] static GeneticAlgorithm Create(Arena& arena, GeneticAlgorithmCreateInfo& info);
		static void Destroy(Arena& arena, GeneticAlgorithm& ga);
	};
}

