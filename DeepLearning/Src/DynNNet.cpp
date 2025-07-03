#include "pch.h"
#include "Algorithms/DynNNet.h"
#include <Jlib/MapUtils.h>
#include <JLib/ArrayUtils.h>

namespace jv::ai
{
	struct Key final
	{
		union
		{
			struct
			{
				uint32_t from;
				uint32_t to;
			};
			uint64_t value;
		};

		Key(const uint32_t from, const uint32_t to)
		{
			this->from = from;
			this->to = to;
		}
	};

	void Mutate(Arena& tempArena, DynNNet& nnet, const DynNNetCreateInfo& info, 
		Vector<uint32_t>& neurons, Vector<uint32_t>& weights, const uint32_t times)
	{
		const auto tempScope = tempArena.CreateScope();

		for (uint32_t i = 0; i < times; i++)
		{
			if (RandF(0, 1) > info.weightToNeuronMutateChance)
			{
				// Mutate into a neuron (+ corresponding weights).
				const uint32_t from = rand() % (neurons.count - info.outputCount);
				const uint32_t to = info.inputCount + (rand() % (neurons.count - info.inputCount));

				// If the mutation is already known to the nnet, use that one.
				const Key key(from, to);
				if (nnet.neuronMap.Contains(key.value))
				{

				}
			}
			else
			{
				// Mutate into a weight.
			}
		}

		tempArena.DestroyScope(tempScope);
	}

	DynInstance CreateArrivalInstance(Arena& arena, Arena& tempArena, DynNNet& nnet, const DynNNetCreateInfo& info)
	{
		DynInstance instance{};
		const auto tempScope = tempArena.CreateScope();
		
		const uint32_t weightInitialCount = info.connectStartingNeurons ? info.inputCount * info.outputCount : 0;
		auto neurons = CreateVector<uint32_t>(tempArena, info.inputCount + info.outputCount + info.initialAlpha);
		// Multiply by 2 because every neuron will also come with 2 new weights.
		auto weights = CreateVector<uint32_t>(tempArena, weightInitialCount + info.initialAlpha * 2);

		const uint32_t predefNeuronCount = info.inputCount + info.outputCount;
		neurons.count = predefNeuronCount;
		for (uint32_t i = 0; i < predefNeuronCount; i++)
			neurons[i] = i;
		if (info.connectStartingNeurons)
		{
			const uint32_t predefWeightCount = info.inputCount * info.outputCount;
			weights.count = predefWeightCount;
			for (uint32_t i = 0; i < predefNeuronCount; i++)
				weights[i] = i;
		}

		Mutate(tempArena, nnet, info, neurons, weights, info.initialAlpha);

		tempArena.DestroyScope(tempScope);
		return instance;
	}

	DynNNet DynNNet::Create(Arena& arena, Arena& tempArena, const DynNNetCreateInfo& info)
	{
		DynNNet nnet{};
		nnet.scope = arena.CreateScope();
		nnet.weights = CreateVector<Weight>(arena, info.weightCapacity);
		nnet.neuronMap = CreateMap<uint64_t>(arena, info.neuronCapacity);
		nnet.weightMap = CreateMap<uint64_t>(arena, info.weightCapacity);

		const uint32_t predefNeuronCount = info.inputCount + info.outputCount;
		nnet.neuronCount = predefNeuronCount;

		if (info.connectStartingNeurons)
			for (uint32_t i = 0; i < info.inputCount; i++)
				for (uint32_t j = 0; j < info.outputCount; j++)
				{
					auto& weight = nnet.weights.Add();
					weight.from = i;
					weight.to = info.inputCount + j;
				}	

		nnet.generationScope = arena.CreateScope();
		nnet.generation = CreateArray<DynInstance>(arena, info.generationSize);
		for (uint32_t i = 0; i < info.generationSize; i++)
		{
			auto& instance = nnet.generation[i];
			instance = CreateArrivalInstance(arena, tempArena, nnet, info);
		}

		return nnet;
	}
	void DynNNet::Destroy(Arena& arena, const DynNNet& nnet)
	{
		arena.DestroyScope(nnet.scope);
	}
}
