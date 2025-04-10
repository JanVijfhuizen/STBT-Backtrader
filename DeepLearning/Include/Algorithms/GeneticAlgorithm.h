#pragma once

namespace jv
{
	struct GeneticAlgorithmCreateInfo final
	{
		// Length of a generation.
		uint32_t length;
		// Width of an instance.
		uint32_t width;
		// Function to rate instances.
		float(*rate)(float* values);
		// Chance to mutate child instance.
		float mutateChance = .08f;
		float mutateMultiplier = .1f;
		float mutateAddition = .2f;
		// The percentage of survivers from one generation to the next.
		float surviverPct = .4f;
		// The amount of new arrivals in every generation.
		float arrivalsPct = .4f;
	};

	struct GeneticAlgorithm final
	{
		uint32_t length;
		uint32_t width;
		float mutateChance;
		float mutateMultiplier;
		float mutateAddition;
		float surviverPct;
		float arrivalsPct;

		uint64_t scope;
		// Array of value instances.
		float** generation;
		// Current optimal combination of values.
		float* result;
		// Ratings of generation.
		float* genRatings;
		// Rating of current result.
		float rating;
		// Generation index.
		uint32_t genId;
		// Training index.
		uint32_t trainId;

		// Randomly initialize generation.
		void RandInit();

		[[nodiscard]] float* GetTrainee();
		void Rate(Arena& tempArena, float rating);

		[[nodiscard]] static GeneticAlgorithm Create(Arena& arena, GeneticAlgorithmCreateInfo& info);
		static void Destroy(Arena& arena, GeneticAlgorithm& ga);
	};
}

