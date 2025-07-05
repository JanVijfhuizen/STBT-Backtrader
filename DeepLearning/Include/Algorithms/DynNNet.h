#pragma once
#include <Jlib/Map.h>
#include "GeneticAlgorithm.h"

namespace jv::ai
{
	struct DynNNetCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;

		uint32_t generationSize = 500;
		// How many mutations per new (non conceived) instance.
		uint32_t initialAlpha = 10;
		uint32_t neuronCapacity = 1000;
		uint32_t weightCapacity = 5000;
		bool connectStartingNeurons = true;
	};

	struct CWeight final
	{
		float mul;
		uint32_t index;
	};

	struct Neuron final
	{
		struct Sigmoid final
		{

		};
		struct Spike final
		{
			float threshold;
			float decay;
		};
		union
		{
			Sigmoid sigmoid;
			Spike spike;
		};

		enum class Type
		{
			sigmoid,
			spike,
			length
		};
		Type type;

		float value;
		bool signalled;

		Array<CWeight> cWeights{};

		[[nodiscard]] bool Enabled() const;
	};

	struct Weight final
	{
		float value;
		uint32_t from, to;
	};

	struct DynInstance final
	{
		Array<uint32_t> neurons;
		Array<uint32_t> weights;

		void Copy(Arena& arena, DynInstance& dst) const;
	};

	struct DynNNet final
	{
		DynNNetCreateInfo info;
		float apexPct = .1f;
		float breedablePct = .4f;
		float arrivalsPct = .1f;
		float weightToNeuronMutateChance = .5f;
		uint32_t alpha = 1;

		float gaMutateChance = .01f;
		float gaMutateAddition = 1;
		float gaMutateMultiplier = .1f;

		uint64_t scope;
		uint64_t resultScope;
		uint64_t generationScope;
		uint64_t constructScope;

		Vector<Neuron> neurons;
		Vector<Weight> weights;

		Map<uint64_t> neuronMap;
		Map<uint64_t> weightMap;

		DynInstance result;
		Array<DynInstance> generation;
		float* ratings;
		float generationRating = 0;
		float rating = 0;
		uint32_t generationId = 0;
		uint32_t currentId = 0;

		uint64_t parameterScope;
		GeneticAlgorithm ga;

		void CreateParameters(Arena& arena);
		void DestroyParameters(Arena& arena);

		[[nodiscard]] DynInstance& GetCurrent();
		void Rate(Arena& arena, Arena& tempArena);
		[[nodiscard]] float* GetCurrentParameters();
		void RateParameters(Arena& arena, Arena& tempArena, float rating);

		void Construct(Arena& arena, Arena& tempArena, const DynInstance& instance);
		void Deconstruct(Arena& arena, const DynInstance& instance);
		void ConstructParameters(DynInstance& instance, float* values);

		void Propagate(Arena& tempArena, const Array<float>& input, const Array<bool>& output);

		[[nodiscard]] static DynNNet Create(Arena& arena, Arena& tempArena, const DynNNetCreateInfo& info);
		static void Destroy(Arena& arena, const DynNNet& nnet);
	};
}

