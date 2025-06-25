#pragma once

namespace jv::nnet
{
	struct InstanceCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;
		bool connected;
		bool randomize;
	};

	struct GroupCreateInfo final
	{
		uint32_t inputCount;
		uint32_t outputCount;

		uint32_t length = 100;
		float mutateChance = .01f;
		float mutateAddition = 1;
		float mutateMultiplier = .1f;
		float mutateNewWeightChance = .01f;
		float mutateNewNodeChance = .01f;
		float decayCap = .01f;

		uint32_t maxNeurons = 20;
		uint32_t maxWeights = 50;

		float apexPct = .1f;
		float breedablePct = .4f;
		float arrivalsPct = .1f;
		uint32_t kmPointCount = 5;
	};

	struct Connections final
	{
		Array<uint32_t> weightIds;
	};

	struct Neuron final
	{
		float value;
		float threshold;
		// Can be negative for reverse decay.
		float decay;
		bool signalled;
		uint32_t id;
		float dominance;
	};

	struct Weight final
	{
		uint32_t from, to;
		float value;
		uint32_t id;
		float dominance;
		bool enabled = true;
	};

	struct Instance final
	{
		Array<Neuron> neurons;
		Array<Weight> weights{};
		Array<Connections> connections{};

		void Propagate(Arena& tempArena, const Array<float>& input, const Array<bool>& output);
		void Flush();

		[[nodiscard]] static Instance Create(Arena& arena, Arena& tempArena, 
			const InstanceCreateInfo& info, struct Group& group);
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

		[[nodiscard]] static Group Create(Arena& arena, Arena& tempArena, const GroupCreateInfo& info);
		static void Destroy(Group& group, Arena& arena);
	};
}
