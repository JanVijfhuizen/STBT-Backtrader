#pragma once
#include <Jlib/Map.h>
#include "GeneticAlgorithm.h"

namespace jv::ai
{
	enum class DynType
	{
		classification
	};

	struct DynNNetCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;
		DynType type = DynType::classification;

		uint32_t generationSize = 500;
		// How many mutations per new (non conceived) instance.
		uint32_t initialAlpha = 10;
		uint32_t neuronCapacity = 1000;
		uint32_t weightCapacity = 5000;
		bool connectStartingNeurons = true;
	};

	struct Neuron final
	{
		struct Spike final
		{
			float threshold;
			float decay;
		};
		struct Step final
		{
			float threshold;
		};
		struct Gauss final
		{
			float mean;
			float stddev;
		};
		union
		{
			Spike spike;
			Step step;
			Gauss gauss;
		};

		enum class Type
		{
			sigmoid,
			sine,
			spike,
			abs,
			inv,
			relu,
			sin,
			cos,
			step,
			tanh,
			lin,
			gauss,
			output
		};

		Type type;
		float value;
		bool signalled;
		Array<uint32_t> cWeights{};
	};

	struct Weight final
	{
		float value;
		uint32_t from, to;
	};

	struct DynInstance final
	{
		enum class OutputType
		{
			lin,
			softmax
		};

		Array<uint32_t> neurons;
		Array<uint32_t> weights;
		float* parameters;
		OutputType outputType;

		void Copy(Arena& arena, DynInstance& dst, bool copyParameters) const;
	};

	struct DynNNet final
	{
		DynNNetCreateInfo info;
		float apexPct = .1f;
		float breedablePct = .4f;
		float arrivalsPct = .1f;
		float weightToNeuronMutateChance = .5f;
		uint32_t alpha = 1;
		uint32_t kmPointCount = 3;
		uint32_t kmCycles = 50;

		uint32_t gaLength = 80;
		float gaMutateChance = .01f;
		float gaMutateAddition = 1;
		float gaMutateMultiplier = .1f;
		uint32_t gaKmPointCount = 3;

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
		DynInstance::OutputType outputType;

		void CreateParameters(Arena& arena);
		void DestroyParameters(Arena& arena);

		[[nodiscard]] DynInstance& GetCurrent();
		void Rate(Arena& arena, Arena& tempArena);
		[[nodiscard]] float* GetCurrentParameters();
		void RateParameters(Arena& arena, Arena& tempArena, float rating);

		void Construct(Arena& arena, Arena& tempArena, const DynInstance& instance);
		void Deconstruct(Arena& arena, const DynInstance& instance);
		void ConstructParameters(DynInstance& instance, float* values);
		
		void Flush(DynInstance& instance);
		void Propagate(Arena& tempArena, const Array<float>& input, const Array<float>& output);

		[[nodiscard]] static DynNNet Create(Arena& arena, Arena& tempArena, const DynNNetCreateInfo& info);
		static void Destroy(Arena& arena, const DynNNet& nnet);
	};
}

