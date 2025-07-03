#pragma once
#include <Jlib/Map.h>

namespace jv::ai
{
	struct DynNNetCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;

		uint32_t generationSize = 500;
		// How many mutations per new (non conceived) instance.
		uint32_t initialAlpha = 10;
		float weightToNeuronMutateChance = .5f;

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
		float value;
		float threshold;
		float decay;
		bool signalled;

		Array<CWeight> cWeights{};

		[[nodiscard]] bool Enabled() const;
	};

	struct Weight final
	{
		uint32_t from, to;
	};

	struct DynInstance final
	{
		Array<uint32_t> neurons;
		Array<uint32_t> weights;
	};

	struct DynNNet final
	{
		uint64_t scope;
		uint64_t generationScope;
		uint64_t constructScope;
		bool isConstructed;

		Vector<Neuron> neurons;
		Vector<Weight> weights;

		Map<uint64_t> neuronMap;
		Map<uint64_t> weightMap;

		Array<DynInstance> generation;

		void Construct(Arena& arena, Arena& tempArena, const DynInstance& instance);
		void Deconstruct(Arena& arena, const DynInstance& instance);

		void Propagate(Arena& tempArena, const Array<float>& input, const Array<bool>& output);

		[[nodiscard]] static DynNNet Create(Arena& arena, Arena& tempArena, const DynNNetCreateInfo& info);
		static void Destroy(Arena& arena, const DynNNet& nnet);
	};
}

