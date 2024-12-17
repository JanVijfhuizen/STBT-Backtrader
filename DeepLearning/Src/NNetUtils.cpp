#include "pch.h"
#include "NNetUtils.h"
#include "Jlib/Math.h"

namespace jv::ai
{
	void InitFlat(NNet& nnet)
	{
		const uint32_t c = nnet.createInfo.inputSize + nnet.createInfo.outputSize;
		for (uint32_t i = 0; i < c; i++)
			AddNeuron(nnet, 1, 0);
	}

	void InitRandom(NNet& nnet)
	{
		const uint32_t c = nnet.createInfo.inputSize + nnet.createInfo.outputSize;
		for (uint32_t i = 0; i < c; i++)
			AddNeuron(nnet, jv::RandF(0, 1), jv::RandF(0, 1));
	}
}