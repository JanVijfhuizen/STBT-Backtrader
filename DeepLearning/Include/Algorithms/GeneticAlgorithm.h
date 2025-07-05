#pragma once

namespace jv
{
	struct GeneticAlgorithmCreateInfo final
	{
		uint32_t width = 400;
		uint32_t length = 100;
		float mutateChance = .01f;
		float mutateAddition = 1;
		float mutateMultiplier = .1f;

		float apexPct = .1f;
		float breedablePct = .4f;
		float arrivalsPct = .1f;
		uint32_t kmPointCount = 5;
		bool inbreedingOnly = false;
	};

	struct GeneticAlgorithm final
	{
		GeneticAlgorithmCreateInfo info;

		uint64_t scope;
		uint64_t resScope;
		uint64_t genScope;

		float** generation;
		float* result;
		float* genRatings;
		float rating;
		float genRating;
		uint32_t genId;
		uint32_t trainId;

		[[nodiscard]] float* GetTrainee();
		void Rate(Arena& arena, Arena& tempArena, float rating, Queue<bt::OutputMsg>* output = nullptr);

		[[nodiscard]] static GeneticAlgorithm Create(Arena& arena, GeneticAlgorithmCreateInfo& info);
		static void Destroy(Arena& arena, GeneticAlgorithm& ga);
	};
}

