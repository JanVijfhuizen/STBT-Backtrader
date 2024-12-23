#pragma once
#include "JLib/Arena.h"

namespace jv::ai
{
	struct Neuron final
	{
		float value;
		float decay;
		float threshold;
		uint32_t weightsId = UINT32_MAX;
	};

	struct Weight final
	{
		float value;
		uint32_t from, to;
		uint32_t innovationId;
		uint32_t next = UINT32_MAX;
		bool enabled = true;
	};

	struct NNetCreateInfo final 
	{
		uint32_t neuronCapacity;
		uint32_t weightCapacity;
		uint32_t inputSize;
		uint32_t outputSize;
	};

	struct NNet final
	{
		NNetCreateInfo createInfo;
		uint64_t scope;
		Neuron* neurons;
		Weight* weights;
		uint32_t neuronCount;
		uint32_t weightCount;
		uint32_t globalInnovationId;
	};

	[[nodiscard]] NNet CreateNNet(NNetCreateInfo& info, Arena& arena);
	void DestroyNNet(NNet& nnet, Arena& arena);

	// Reset the current value of all neurons to 0.
	void Clean(NNet& nnet);
	// Destroy all neurons and weights.
	void Clear(NNet& nnet);
	// Forward information through the network.
	void Propagate(NNet& nnet, float* input, bool* output);

	bool AddWeight(NNet& nnet, uint32_t from, uint32_t to, float value);
	bool AddNeuron(NNet& nnet, float decay, float threshold);
}