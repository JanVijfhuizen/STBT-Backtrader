#pragma once
#include "NNet.h"

namespace jv::ai 
{
	struct Layer final 
	{
		uint32_t from = UINT32_MAX;
		uint32_t to = UINT32_MAX;
	};

	struct IOLayers
	{
		Layer input, output;
	};

	struct Mutation final
	{
		float chance = 0;
		float linAlpha = 1;
		float pctAlpha = 1;
	};

	struct Mutations final
	{
		Mutation weight{};
		Mutation threshold{};
		Mutation decay{};
		float newNodeChance = 0;
		float newWeightChance = 0;
	};

	enum class InitType 
	{
		flat,
		random
	};

	/* 
	Initialize neural network with default input / output layers.
	Returns the input layer.
	*/
	IOLayers Init(NNet& nnet, InitType initType);
	// Adds a new layer of neurons.
	Layer AddLayer(NNet& nnet, uint32_t length, InitType initType);
	void Connect(NNet& nnet, Layer from, Layer to, InitType initType);
	// Connect the input and output layers.
	void ConnectIO(NNet& nnet, InitType initType);

	[[nodiscard]] float GetCompability(NNet& a, NNet& b);
	void Breed(NNet& a, NNet& b, NNet& c);

	void Mutate(NNet& nnet, Mutations mutations);
	void Copy(NNet& org, NNet& dst);
}

