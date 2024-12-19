#include "pch.h"
#include "NNetUtils.h"
#include "Jlib/Math.h"

namespace jv::ai
{
	IOLayers Init(NNet& nnet, const InitType initType)
	{
		auto inputLayer = AddLayer(nnet, nnet.createInfo.inputSize, initType);
		auto outputLayer = AddLayer(nnet, nnet.createInfo.outputSize, initType);
		return { inputLayer, outputLayer };
	}

	Layer AddLayer(NNet& nnet, const uint32_t length, InitType initType)
	{
		for (uint32_t i = 0; i < length; i++)
			switch (initType)
			{
			case InitType::flat:
				AddNeuron(nnet, 1, 0);
				break;
			case InitType::random:
				AddNeuron(nnet, jv::RandF(0, 1), jv::RandF(0, 1));
				break;
			default:
				break;
			}

		return { nnet.neuronCount - length, nnet.neuronCount };
	}

	void Connect(NNet& nnet, Layer from, Layer to, InitType initType)
	{
		const uint32_t inSize = from.to - from.from;
		const uint32_t outSize = to.to - to.from;

		for (uint32_t i = 0; i < inSize; i++)
			for (uint32_t j = 0; j < outSize; j++)
				switch (initType)
				{
				case InitType::flat:
					AddWeight(nnet, from.from + i, to.from + j, 1);
					break;
				case InitType::random:
					AddWeight(nnet, from.from + i, to.from + j, jv::RandF(-1, 1));
					break;
				default:
					break;
				}
	}

	void ConnectIO(NNet& nnet, const InitType initType)
	{
		const uint32_t inSize = nnet.createInfo.inputSize;
		const uint32_t outSize = nnet.createInfo.outputSize;
		Connect(nnet, { 0, inSize }, { inSize, inSize + outSize }, initType);
	}
	Neuron* neurons;
	Weight* weights;
	uint32_t neuronCount;
	uint32_t weightCount;

	void Mutate(NNet& nnet, const Mutation mutation)
	{
		if (mutation.weightValueChance > 0)
		{
			for (size_t i = 0; i < nnet.weightCount; i++)
			{
				if (RandF(0, 1) > mutation.weightValueChance)
					continue;

				auto& weight = nnet.weights[i];
				// Either adjust the value percentage wise or replace it with a new one.
				bool pctWise = rand() % 2;
				weight.value = pctWise ? weight.value * RandF(0, 2) : RandF(-1, 1);
			}
		}
		if (mutation.thresholdValueChance > 0)
		{
			for (size_t i = 0; i < nnet.neuronCount; i++)
			{
				if (RandF(0, 1) > mutation.thresholdValueChance)
					continue;

				auto& neuron = nnet.neurons[i];
				neuron.threshold = RandF(0, 1);
			}
		}
	}

	void Copy(NNet& org, NNet& dst)
	{
		dst.neuronCount = org.neuronCount;
		dst.weightCount = org.weightCount;
		memcpy(dst.neurons, org.neurons, sizeof(Neuron) * org.neuronCount);
		memcpy(dst.weights, org.weights, sizeof(Weight) * org.weightCount);
	}
}