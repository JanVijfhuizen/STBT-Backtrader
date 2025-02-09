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
		bool canRandomize = true;
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
	__declspec(dllexport) IOLayers Init(NNet& nnet, InitType initType, uint32_t& gId);
	// Adds a new layer of neurons.
	__declspec(dllexport) Layer AddLayer(NNet& nnet, uint32_t length, InitType initType, uint32_t& gId);
	__declspec(dllexport) void Connect(NNet& nnet, Layer from, Layer to, InitType initType, uint32_t& gId);
	// Connect the input and output layers.
	__declspec(dllexport) void ConnectIO(NNet& nnet, InitType initType, uint32_t& gId);

	__declspec(dllexport) [[nodiscard]] float GetCompability(NNet& a, NNet& b);
	__declspec(dllexport) [[nodiscard]] NNet Breed(NNet& a, NNet& b, Arena& arena, Arena& tempArena);

	__declspec(dllexport) void Mutate(NNet& nnet, Mutations mutations, uint32_t& gId);
	__declspec(dllexport) void Copy(NNet& org, NNet& dst, Arena* arena = nullptr);
}

