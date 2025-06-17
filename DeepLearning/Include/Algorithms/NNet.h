#pragma once

namespace jv::nnet
{
	struct InstanceCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;
		bool connected;
		bool randomize;
		uint32_t maxPropagations = 5;
	};

	struct GroupCreateInfo final
	{

	};

	struct Neuron final
	{
		float value;
		float threshold;
		// Can be negative for reverse decay.
		float decay;
		bool signal;
	};

	struct Weight final
	{
		uint32_t from, to;
		float value;
		// When a neuron fires, it can allow for the weight to fire multiple times afterwards.
		uint32_t propagations;
		uint32_t maxPropagations;
	};

	struct Instance final
	{
		Array<Neuron> neurons;
		Array<Weight> weights{};

		void Propagate(const Array<float>& input, const Array<bool>& output);

		[[nodiscard]] static Instance Create(Arena& arena, const InstanceCreateInfo& info);
	};

	struct Group final
	{
		[[nodiscard]] static Group Create(Arena& arena, Arena& tempArena, const GroupCreateInfo& info);
		static void Destroy(Group& group, Arena& arena);
	};
}
