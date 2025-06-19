#pragma once

namespace jv::nnet
{
	struct InstanceCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;
		bool connected;
		bool randomize;
		uint32_t maxPropagations;
	};

	struct GroupCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;
		uint32_t length;
		uint32_t maxPropagations = 5;

		float apexPct = .1f;
		float breedablePct = .4f;
		float arrivalsPct = .1f;
		uint32_t kmPointCount = 5;
	};

	struct Neuron final
	{
		float value;
		float threshold;
		// Can be negative for reverse decay.
		float decay;
		bool signal;
		uint32_t id;
	};

	struct Weight final
	{
		uint32_t from, to;
		float value;
		// When a neuron fires, it can allow for the weight to fire multiple times afterwards.
		uint32_t propagations;
		uint32_t maxPropagations;
		uint32_t id;
	};

	struct Instance final
	{
		Array<Neuron> neurons;
		Array<Weight> weights{};

		void Propagate(const Array<float>& input, const Array<bool>& output);

		[[nodiscard]] static Instance Create(Arena& arena, const InstanceCreateInfo& info, struct Group& group);
	};

	struct Group final
	{
		uint64_t scope;
		uint64_t resScope;
		uint64_t genScope;

		Instance result;

		GroupCreateInfo info;
		Array<Instance> generation;
		// Global Innovation Id;
		uint32_t gId;
		uint32_t genId;
		uint32_t trainId;

		float rating = FLT_MIN;
		float genRating = FLT_MIN;
		float* genRatings;

		[[nodiscard]] Instance& GetTrainee();
		void Rate(Arena& arena, Arena& tempArena, float rating, Queue<bt::OutputMsg>& output);

		[[nodiscard]] static Group Create(Arena& arena, const GroupCreateInfo& info);
		static void Destroy(Group& group, Arena& arena);
	};
}
