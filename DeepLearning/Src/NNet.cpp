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

	void Propagate(NNet& nnet, float* input, float* output)
	{
		for (uint32_t i = 0; i < nnet.createInfo.inputSize; i++)
			nnet.neurons[i].value = input[i];

		for (uint32_t i = 0; i < nnet.neuronCount; i++)
		{
			auto& neuron = nnet.neurons[i];
			uint32_t weightId = neuron.weightsId;
			float remainder = neuron.value - neuron.threshold;
			neuron.value = Max<float>(neuron.value, 0);

			// Makes sure you don't get extreme values if the nnet becomes very deep.
			remainder = Min<float>(remainder, 1);
			
			if (remainder > 0)
			{
				while (weightId != -1)
				{
					auto& weight = nnet.weights[weightId];
					auto& nextNeuron = nnet.neurons[weight.to];

					// Multiply origin neuron value by weight value if above threshold.
					float value = weight.value;
					nextNeuron.value += value;
					weightId = weight.next;
				}
				neuron.value = 0;
			}
		}

		// Ready output.
		for (uint32_t i = 0; i < nnet.createInfo.outputSize; i++)
		{
			output[i] = nnet.neurons[nnet.createInfo.inputSize + i].value;
			output[i] = jv::Clamp<float>(output[i], 0, 1);
		}

		// Clamp values and apply decay.
		for (uint32_t i = 0; i < nnet.neuronCount; i++)
		{
			auto& neuron = nnet.neurons[i];
			neuron.value *= neuron.decay;
		}
	}

	bool AddWeight(NNet& nnet, const uint32_t from, const uint32_t to, const float value)
	{
		if (nnet.weightCount >= nnet.createInfo.weightCapacity)
			return false;
		assert(from < nnet.neuronCount);
		assert(to < nnet.neuronCount);

		// Not allowed to add a weight with an input node as a destination.
		assert(to >= nnet.createInfo.inputSize);
		Neuron& neuron = nnet.neurons[from];
		Weight& weight = nnet.weights[nnet.weightCount] = {};
		weight.from = from;
		weight.to = to;
		weight.value = value;
		weight.next = neuron.weightsId;
		neuron.weightsId = nnet.weightCount++;
		return true;
	}

	bool AddNeuron(NNet& nnet, const float decay, const float threshold)
	{
		if (nnet.neuronCount >= nnet.createInfo.neuronCapacity)
			return false;

		Neuron& neuron = nnet.neurons[nnet.neuronCount++] = {};
		neuron.value = 0;
		neuron.decay = decay;
		neuron.threshold = threshold;
		return true;
	}
}