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
	};

	struct GeneticAlgorithm final
	{
		// Generation length.
		uint32_t length;
		// Instance width.
		uint32_t width;
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

