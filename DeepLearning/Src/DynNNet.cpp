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

	void Mutate(DynNNet& nnet, const DynNNetCreateInfo& info, 
		Vector<uint32_t>& neurons, Vector<uint32_t>& weights, const uint32_t times)
	{
		for (uint32_t i = 0; i < times; i++)
		{
			if (RandF(0, 1) > info.weightToNeuronMutateChance)
			{
				// Mutate into a neuron (+ corresponding weights).
				const uint32_t refWeightIndex = rand() % weights.count;
				const uint32_t weightIndex = weights[refWeightIndex];
				const auto& weight = nnet.weights[weightIndex];

				const Key key(weight.from, weight.to);
				const uint64_t* index = nnet.neuronMap.Contains(key.value);

				bool valid = true;

				if (index)
				{
					// Check if this instance already has this neuron.			
					for(auto& neuron : neurons)
						if (neuron == *index)
						{
							valid = false;
							break;
						}

					// ONLY if the neuron does not already exist in this instance, will it be added.
					if (valid)
					{
						// If the mutation is already known to the nnet, use that one.
						neurons.Add() = *index;

						// We can assume the connections will also be there.
						const Key inpKey(weight.from, *index);
						const Key outpKey(*index, weight.to);
						weights.Add() = *nnet.weightMap.Contains(inpKey.value);
						weights.Add() = *nnet.weightMap.Contains(outpKey.value);
					}
				}
				else
				{
					// This is a completely new mutation, so add it to the registry.
					const uint32_t index = nnet.neuronCount;
					neurons.Add() = nnet.neuronCount++;
					nnet.neuronMap.Insert(index, key.value);

					const Key inpKey(weight.from, index);
					const Key outpKey(index, weight.to);

					uint32_t weightIndex = nnet.weights.count;
					auto& fromWeight = nnet.weights.Add();
					fromWeight.from = weight.from;
					fromWeight.to = index;
					nnet.weightMap.Insert(weightIndex, inpKey.value);
					weights.Add() = weightIndex;
					
					weightIndex++;
					auto& toWeight = nnet.weights.Add();
					toWeight.from = index;
					toWeight.to = weight.to;
					nnet.weightMap.Insert(weightIndex, outpKey.value);
					weights.Add() = weightIndex;
				}

				// Remove old weight.
				weights.RemoveAtOrdered(refWeightIndex);
			}
			else
			{
				// Mutate into a weight.
				const uint32_t from = rand() % (neurons.count - info.outputCount);
				const uint32_t to = info.inputCount + (rand() % (neurons.count - info.inputCount));

				const Key key(from, to);
				if (!nnet.neuronMap.Contains(key.value))
				{
					const uint32_t weightIndex = nnet.weights.count;
					const Key key(from, to);
					auto& newWeight = nnet.weights.Add();
					newWeight.from = from;
					newWeight.to = to;
					nnet.weightMap.Insert(weightIndex, key.value);
					weights.Add() = weightIndex;
				}
			}
		}
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

		Mutate(nnet, info, neurons, weights, info.initialAlpha);

		instance.neurons = CreateArray<uint32_t>(arena, neurons.count);
		instance.weights = CreateArray<uint32_t>(arena, weights.count);
		memcpy(instance.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.count);
		memcpy(instance.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.count);

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
	void DynCInstance::Propagate(Arena& tempArena, const Array<float>& input, const Array<float>& output)
	{
		const auto tempScope = tempArena.CreateScope();

		// Decide on what kind of propagation will actually happen.

		tempArena.DestroyScope(tempScope);
	}
	DynCInstance DynCInstance::Create(Arena& arena, Arena& tempArena,
		const DynNNet& nnet, const DynInstance& instance)
	{
		DynCInstance constructed{};
		const auto tempScope = tempArena.CreateScope();

		const uint32_t l = instance.neurons.length;
		auto nums = tempArena.New<uint32_t>(l);
		for (auto& w : instance.weights)
		{
			auto& weight = nnet.weights[w];
			++nums[weight.from];
		}

		constructed.neurons = CreateArray<CNeuron>(arena, l);
		for (uint32_t i = 0; i < l; i++)
		{
			auto& neuron = constructed.neurons[i];
			neuron.weights = CreateArray<CWeight>(arena, nums[i]);
		}
		for (uint32_t i = 0; i < l; i++)
			nums[i] = 0;

		for (uint32_t i = 0; i < instance.weights.length; i++)
		{
			const uint32_t w = instance.weights[i];
			auto& weight = nnet.weights[w];

			auto& neuron = constructed.neurons[weight.from];
			auto& n = nums[weight.from];
			neuron.weights[n++].to = i;
		}

		tempArena.DestroyScope(tempScope);
		return constructed;
	}
}
