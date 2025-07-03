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

	struct Weight final
	{
		uint32_t from, to;
	};

	struct DynInstance final
	{
		Array<uint32_t> neurons;
		Array<uint32_t> weights;
	};

	struct CWeight final
	{
		float value;
		uint32_t to;
	};

	struct CNeuron final
	{
		float value;
		Array<CWeight> weights;
	};

	struct DynCInstance final
	{
		Array<CNeuron> neurons;

		void Propagate(Arena& tempArena, const Array<float>& input, const Array<float>& output);
		[[nodiscard]] static DynCInstance Create(Arena& arena, Arena& tempArena, 
			const struct DynNNet& nnet, const DynInstance& instance);
	};

	struct DynNNet final
	{
		uint64_t scope;
		uint64_t generationScope;

		uint32_t neuronCount;
		Vector<Weight> weights;

		Map<uint64_t> neuronMap;
		Map<uint64_t> weightMap;

		Array<DynInstance> generation;

		[[nodiscard]] static DynNNet Create(Arena& arena, Arena& tempArena, const DynNNetCreateInfo& info);
		static void Destroy(Arena& arena, const DynNNet& nnet);
	};
}

