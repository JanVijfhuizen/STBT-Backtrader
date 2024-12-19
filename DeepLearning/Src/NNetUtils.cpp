#include "pch.h"
#include "NNetUtils.h"
#include "Jlib/Math.h"

namespace jv::ai
{
	void Init(NNet& nnet, const InitType initType)
	{
		const uint32_t c = nnet.createInfo.inputSize + nnet.createInfo.outputSize;
		for (uint32_t i = 0; i < c; i++)
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
	}

	void ConnectIOLayers(NNet& nnet, const InitType initType)
	{
		const uint32_t inSize = nnet.createInfo.inputSize;
		const uint32_t outSize = nnet.createInfo.outputSize;

		for (uint32_t i = 0; i < inSize; i++)
			for (uint32_t j = 0; j < outSize; j++)
				switch (initType)
				{
				case InitType::flat:
					AddWeight(nnet, i, inSize + j, 1);
					break;
				case InitType::random:
					AddWeight(nnet, i, inSize + j, jv::RandF(-1, 1));
					break;
				default:
					break;
				}
	}
}