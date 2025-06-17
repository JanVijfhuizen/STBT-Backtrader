#include "pch.h"
#include "Algorithms/NNet.h"
#include "Jlib/ArrayUtils.h"

namespace jv::nnet
{
	void Instance::Propagate(const Array<float>& input, const Array<bool>& output)
	{
		// Propagate input.
		for (uint32_t i = 0; i < input.length; i++)
			weights[i].value += input[i];

		// Signal all neurons that are over the threshold.
		for (auto& neuron : neurons)
		{
			neuron.signal = neuron.value > neuron.threshold;
			neuron.value = Min(neuron.value, neuron.threshold);
			neuron.value -= neuron.decay;
			neuron.value = Max(neuron.value, 0.f);
			// Reset to 0 if the neuron has been signalled.
			neuron.value = neuron.signal ? 0 : neuron.value;
		}

		// Propagate signalled neurons through weights.
		for (auto& weight : weights)
		{
			auto& a = neurons[weight.from];
			if (a.signal)
				weight.propagations = weight.maxPropagations;
			if (weight.propagations > 0)
				continue;

			--weight.propagations;
			auto& b = neurons[weight.to];
			b.value += weight.value;
		}

		// Return output.
		for (uint32_t i = 0; i < output.length; i++)
		{
			const uint32_t ind = neurons.length - output.length + i;
			output[i] = neurons[ind].signal;
		}
	}
	Instance Instance::Create(Arena& arena, const InstanceCreateInfo& info)
	{
		Instance instance{};
		instance.neurons = CreateArray<Neuron>(arena, info.inputCount + info.outputCount);
		if (info.connected)
		{
			instance.weights = CreateArray<Weight>(arena, info.inputCount * info.outputCount);
			for (uint32_t i = 0; i < info.inputCount; i++)
				for (uint32_t j = 0; j < info.outputCount; j++)
				{
					auto& weight = instance.weights[i * info.inputCount + j];
					weight.from = i;
					weight.to = j;
				}
		}
			
		if (info.randomize)
		{
			for (auto& neuron : instance.neurons)
			{
				neuron.signal = false;
				neuron.value = 0;
				neuron.decay = RandF(-1, 1);
				neuron.threshold = RandF(0, 1);
			}
			for (auto& weight : instance.weights)
			{
				weight.value = 0;
				weight.propagations = 0;
				weight.maxPropagations = rand() % info.maxPropagations;
			}
		}
		return instance;
	}
	Group Group::Create(Arena& arena, Arena& tempArena, const GroupCreateInfo& info)
	{
		Group group{};

		return group;
	}
	void Group::Destroy(Group& group, Arena& arena)
	{

	}
}