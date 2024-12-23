#include "pch.h"
#include "NNet.h"
#include "Jlib/Math.h"

namespace jv::ai
{
	NNet CreateNNet(NNetCreateInfo& info, Arena& arena)
	{
		NNet nnet{};
		nnet.createInfo = info;
		nnet.neuronCount = 0;
		nnet.weightCount = 0;
		nnet.scope = arena.CreateScope();
		nnet.neurons = arena.New<Neuron>(info.neuronCapacity);
		nnet.weights = arena.New<Weight>(info.weightCapacity);
		return nnet;
	}

	void DestroyNNet(NNet& nnet, Arena& arena)
	{
		arena.DestroyScope(nnet.scope);
	}

	void Clean(NNet& nnet)
	{
		for (uint32_t i = 0; i < nnet.neuronCount; i++)
			nnet.neurons[i].value = 0;
	}

	void Clear(NNet& nnet)
	{
		nnet.neuronCount = 0;
		nnet.weightCount = 0;
	}

	void Propagate(NNet& nnet, float* input, bool* output)
	{
		for (uint32_t i = 0; i < nnet.createInfo.inputSize; i++)
			nnet.neurons[i].value = input[i];

		for (uint32_t i = 0; i < nnet.neuronCount; i++)
		{
			auto& neuron = nnet.neurons[i];
			uint32_t weightId = neuron.weightsId;
			neuron.value = Max<float>(neuron.value, 0);

			if (neuron.value > neuron.threshold)
			{
				while (weightId != -1)
				{
					auto& weight = nnet.weights[weightId];
					auto& nextNeuron = nnet.neurons[weight.to];
					float value = weight.value * weight.enabled;
					nextNeuron.value += value;
					weightId = weight.next;
				}
			}
		}

		// Ready output.
		for (uint32_t i = 0; i < nnet.createInfo.outputSize; i++)
		{
			const auto& neuron = nnet.neurons[nnet.createInfo.inputSize + i];
			output[i] = neuron.value > neuron.threshold;
		}

		// Clamp values, reset spiked neurons and apply decay.
		for (uint32_t i = 0; i < nnet.neuronCount; i++)
		{
			auto& neuron = nnet.neurons[i];
			neuron.value = neuron.value > neuron.threshold ? 0 : neuron.value;
			neuron.value *= neuron.decay;
		}
	}

	bool AddWeight(NNet& nnet, const uint32_t from, const uint32_t to, const float value, uint32_t& gId)
	{
		if (nnet.weightCount >= nnet.createInfo.weightCapacity)
			return false;
		assert(from < nnet.neuronCount);
		assert(to < nnet.neuronCount);

		// Not allowed to add a weight with an input node as a destination.
		assert(to >= nnet.createInfo.inputSize);
		Neuron& neuron = nnet.neurons[from];
		Weight& weight = nnet.weights[nnet.weightCount] = {};
		weight.innovationId = gId++;
		weight.from = from;
		weight.to = to;
		weight.value = value;
		weight.next = neuron.weightsId;
		neuron.weightsId = nnet.weightCount++;
		return true;
	}

	bool AddNeuron(NNet& nnet, const float decay, const float threshold, uint32_t& gId)
	{
		if (nnet.neuronCount >= nnet.createInfo.neuronCapacity)
			return false;

		Neuron& neuron = nnet.neurons[nnet.neuronCount++] = {};
		neuron.value = 0;
		neuron.decay = decay;
		neuron.threshold = threshold;
		neuron.innovationId = gId++;
		return true;
	}
}