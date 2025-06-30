#include "pch.h"
#include "Algorithms/HyperNNet.h"

namespace jv::ai
{
	HyperNNetInfo HyperNNetInfo::Create(const HyperNNetCreateInfo& info)
	{
		HyperNNetInfo ret{};
		ret.neuronCount = info.hiddenCount + info.outputCount;
		ret.inputCount = info.inputCount;
		ret.hiddenCount = info.hiddenCount;
		ret.outputCount = info.outputCount;
		ret.inputWeightCount = info.inputCount * info.hiddenCount;
		ret.hiddenWeightCount = info.hiddenCount * (info.hiddenCount - 1);
		ret.outputWeightCount = info.outputCount * info.hiddenCount;
		ret.weightCount = ret.inputWeightCount + ret.hiddenWeightCount + ret.outputWeightCount;
		ret.hiddenPropagations = info.hiddenPropagations;
		return ret;
	}

	HyperNNet HyperNNet::Create(Arena& arena, const HyperNNetCreateInfo& info)
	{
		const auto blueprint = HyperNNetInfo::Create(info);

		HyperNNet nnet{};
		nnet.thresholds = arena.New<float>(blueprint.neuronCount);
		nnet.decays = arena.New<float>(blueprint.neuronCount);
		nnet.values = arena.New<float>(blueprint.neuronCount);
		nnet.weights = arena.New<float>(blueprint.weightCount);
		return nnet;
	}
	void HyperNNet::Randomize(const HyperNNetInfo& info)
	{
		for (uint32_t i = 0; i < info.neuronCount; i++)
		{
			thresholds[i] = RandF(0, 1);
			decays[i] = RandF(-1, 1);
		}
		for (uint32_t i = 0; i < info.weightCount; i++)
			weights[i] = RandF(0, 1);
	}
	void HyperNNet::Flush(const HyperNNetInfo& info)
	{
		for (uint32_t i = 0; i < info.neuronCount; i++)
			values[i] = 0;
	}
	void HyperNNet::Copy(const HyperNNetCreateInfo& info, HyperNNet& src, HyperNNet& dst)
	{
		const auto blueprint = HyperNNetInfo::Create(info);
		memcpy(dst.thresholds, src.thresholds, sizeof(float) * blueprint.neuronCount);
		memcpy(dst.decays, src.decays, sizeof(float) * blueprint.neuronCount);
		memcpy(dst.weights, src.weights, sizeof(float) * blueprint.weightCount);
	}
	void HyperNNet::Mutate(const HyperNNetCreateInfo& info)
	{
		const auto blueprint = HyperNNetInfo::Create(info);

		for (uint32_t i = 0; i < blueprint.neuronCount; i++)
		{
			thresholds[i] = RandF(0, 1) < info.mutateChance ? RandF(0, 1) : thresholds[i];
			decays[i] = RandF(0, 1) < info.mutateChance ? RandF(0, 1) : decays[i];
		}
		for (uint32_t i = 0; i < blueprint.weightCount; i++)
			weights[i] = RandF(0, 1) < info.mutateChance ? RandF(0, 1) : weights[i];
	}
	void HyperNNet::Propagate(Arena& tempArena, const HyperNNetInfo& info, float* input, uint32_t* output)
	{
		// Propagate inputs to hidden layer.
		for (uint32_t i = 0; i < info.inputCount; i++)
			for (uint32_t j = 0; j < info.hiddenCount; j++)
				values[j] += input[i] * weights[i * info.hiddenCount + j];

		// Add the option to have multiple hidden propagations to add temporal complexity.
		for (uint32_t h = 0; h < info.hiddenPropagations; h++)
		{
			const auto tempScope = tempArena.CreateScope();
			const auto propValues = tempArena.New<float>(info.hiddenCount);

			// Find propagated values.
			for (uint32_t i = 0; i < info.hiddenCount; i++)
			{
				const uint32_t spikeCount = values[i] / thresholds[i];
				const float propagatedValue = thresholds[i] * spikeCount;
				propValues[i] = propagatedValue;
				values[i] -= propagatedValue;
			}

			// Forward propagated values.
			for (uint32_t i = 0; i < info.hiddenCount; i++)
				for (uint32_t j = 0; j < info.hiddenCount; j++)
				{
					const uint32_t weightId = info.inputWeightCount + i * info.hiddenCount + j;
					values[j] += propValues[i] * weights[weightId];
				}

			// Decay neurons.
			for (uint32_t i = 0; i < info.neuronCount; i++)
			{
				values[i] -= decays[i];
				values[i] = Max(values[i], 0.f);
			}

			// Attempt to activate output neurons.
			for (uint32_t i = 0; i < info.hiddenCount; i++)
				for (uint32_t j = 0; j < info.outputCount; j++)
				{
					const uint32_t weightId = info.inputWeightCount + info.hiddenWeightCount + i * info.outputCount + j;
					values[info.hiddenCount + j] += propValues[i] * weights[weightId];
				}

			// Update output.
			for (uint32_t i = 0; i < info.outputCount; i++)
			{
				const uint32_t outputId = info.hiddenCount + i;
				const uint32_t spikeCount = values[outputId] / thresholds[outputId];
				const float propagatedValue = thresholds[outputId] * spikeCount;
				values[outputId] -= propagatedValue;
				output[i] += spikeCount;
			}

			tempArena.DestroyScope(tempScope);
		}
	}
}