#pragma once

namespace jv::ai
{
	struct HyperNNetCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;
		uint32_t hiddenCount;

		uint32_t hiddenPropagations = 1;
		float mutateChance = .02f;
	};

	struct HyperNNetInfo final
	{
		uint32_t neuronCount;
		uint32_t inputCount;
		uint32_t hiddenCount;
		uint32_t outputCount;
		uint32_t inputWeightCount;
		uint32_t outputWeightCount;
		uint32_t hiddenWeightCount;
		uint32_t weightCount;
		uint32_t hiddenPropagations;

		[[nodiscard]] static HyperNNetInfo Create(const HyperNNetCreateInfo& info);
	};

	struct HyperNNet final
	{
		float* thresholds;
		float* decays;
		float* weights;
		float* values;

		[[nodiscard]] static HyperNNet Create(Arena& arena, const HyperNNetCreateInfo& info);

		void Randomize(const HyperNNetInfo& info);
		void Flush(const HyperNNetInfo& info);
		static void Copy(const HyperNNetCreateInfo& info, HyperNNet& src, HyperNNet& dst);
		void Mutate(const HyperNNetCreateInfo& info);
		void Propagate(Arena& tempArena, const HyperNNetInfo& info, float* input, uint32_t* output);
	};
}
