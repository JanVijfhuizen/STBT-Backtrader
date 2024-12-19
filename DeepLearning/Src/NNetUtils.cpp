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
}