#include "pch.h"
#include "Algorithms/NNet.h"
#include "Jlib/ArrayUtils.h"

namespace jv::nnet
{
	void Instance::Propagate(const Array<float>& input, const Array<bool>& output)
	{
		// Propagate input.
		for (uint32_t i = 0; i < input.length; i++)
			weights[i].value += input[i];

		// Signal all neurons that are over the threshold.
		for (auto& neuron : neurons)
		{
			neuron.signal = neuron.value > neuron.threshold;
			neuron.value = Min(neuron.value, neuron.threshold);
			neuron.value -= neuron.decay;
			neuron.value = Max(neuron.value, 0.f);
			// Reset to 0 if the neuron has been signalled.
			neuron.value = neuron.signal ? 0 : neuron.value;
		}

		// Propagate signalled neurons through weights.
		for (auto& weight : weights)
		{
			auto& a = neurons[weight.from];
			if (a.signal)
				weight.propagations = weight.maxPropagations;
			if (weight.propagations > 0)
				continue;

			--weight.propagations;
			auto& b = neurons[weight.to];
			b.value += weight.value;
		}

		// Return output.
		for (uint32_t i = 0; i < output.length; i++)
		{
			const uint32_t ind = neurons.length - output.length + i;
			output[i] = neurons[ind].signal;
		}
	}
	Instance Instance::Create(Arena& arena, const InstanceCreateInfo& info, Group& group)
	{
		Instance instance{};
		instance.neurons = CreateArray<Neuron>(arena, info.inputCount + info.outputCount);
		if (info.connected)
		{
			instance.weights = CreateArray<Weight>(arena, info.inputCount * info.outputCount);
			for (uint32_t i = 0; i < info.inputCount; i++)
				for (uint32_t j = 0; j < info.outputCount; j++)
				{
					auto& weight = instance.weights[i * info.outputCount + j];
					weight.from = i;
					weight.to = j;
				}
		}
			
		if (info.randomize)
		{
			for (auto& neuron : instance.neurons)
			{
				neuron.signal = false;
				neuron.value = 0;
				neuron.decay = RandF(-1, 1);
				neuron.threshold = RandF(0, 1);
				neuron.dominance = RandF(0, 1);
			}
			for (auto& weight : instance.weights)
			{
				weight.value = 0;
				weight.propagations = 0;
				weight.maxPropagations = rand() % info.maxPropagations;
				weight.dominance = RandF(0, 1);
			}
		}

		for (auto& neuron : instance.neurons)
			neuron.id = group.gId++;
		for (auto& weight : instance.weights)
			weight.id = group.gId++;

		return instance;
	}

	Instance& Group::GetTrainee()
	{
		return generation[trainId];
	}

	void CreateInstance(Arena& arena, Group& group, Instance& instance)
	{
		const auto& info = group.info;
		InstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.connected = true;
		instanceCreateInfo.inputCount = info.inputCount;
		instanceCreateInfo.outputCount = info.outputCount;
		instanceCreateInfo.maxPropagations = info.maxPropagations;
		instanceCreateInfo.randomize = true;
		instance = Instance::Create(arena, instanceCreateInfo, group);
	}

	Instance Copy(Arena& arena, Group& group, Instance& instance)
	{
		Instance cpy{};
		cpy.neurons = CreateArray<Neuron>(arena, instance.neurons.length);
		for (uint32_t i = 0; i < instance.neurons.length; i++)
			cpy.neurons[i] = instance.neurons[i];
		cpy.weights = CreateArray<Weight>(arena, instance.weights.length);
		for (uint32_t i = 0; i < instance.weights.length; i++)
			cpy.weights[i] = instance.weights[i];
		return instance;
	}

	float MutateF(float& f, const float min, const float max, const GroupCreateInfo& info)
	{
		const uint32_t type = rand() % 3;
		switch (type)
		{
			// Add/Sub
		case 0:
			f += RandF(-info.mutateAddition, info.mutateAddition);
			break;
			// Mul/Div
		case 1:
			f *= 1.f + RandF(-info.mutateMultiplier, info.mutateMultiplier);
			break;
			// New
		case 2:
			f = RandF(-1, 1);
			break;
		}
		f = Clamp(f, min, max);
	}

	void Mutate(Arena& arena, Group& group, Instance& instance)
	{
		const auto& info = group.info;

		for (uint32_t i = 0; i < instance.neurons.length; i++)
		{
			if (RandF(0, 1) > info.mutateChance)
				continue;

			auto& neuron = instance.neurons[i];

			if(rand() % 2 == 0)
				MutateF(neuron.decay, -1, 1, info);
			else
				MutateF(neuron.threshold, 0, 1, info);
		}

		for (uint32_t i = 0; i < instance.weights.length; i++)
		{
			if (RandF(0, 1) > info.mutateChance)
				continue;

			auto& weight = instance.weights[i];

			if (rand() % 2 == 0)
				MutateF(weight.value, -1, 1, info);
			else
				weight.maxPropagations = rand() % info.maxPropagations;
		}

		if (RandF(0, 1) < info.mutateNewWeightChance)
		{
			// Instead have the mutation chance calculated when they breed, so I dont waste any more space
		}

		if (RandF(0, 1) < info.mutateNewNodeChance)
		{

		}
	}

	Instance Breed(Arena& arena, Group& group, Instance& a, Instance& b)
	{
		Instance instance{};
		CreateInstance(arena, group, instance);
		return instance;
	}

	bool Comparer(float& a, float& b)
	{
		return a > b;
	}

	void Group::Rate(Arena& arena, Arena& tempArena, const float rating, Queue<bt::OutputMsg>& output)
	{
		// Finish generation and start new one if applicable.
		genRatings[trainId++] = rating;
		if (trainId >= info.length)
		{
			trainId = 0;
			++genId;

			auto tempScope = tempArena.CreateScope();

			// Copy all instances to temp arena.
			Instance* cpyGen = tempArena.New<Instance>(info.length);
			for (uint32_t i = 0; i < info.length; i++)
				cpyGen[i] = Copy(tempArena, *this, generation[i]);

			// Reset to start.
			arena.DestroyScope(genScope);

			auto indices = tempArena.New<uint32_t>(info.length);
			jv::CreateSortableIndices(indices, info.length);
			jv::ExtLinearSort(genRatings, indices, info.length, Comparer);
			jv::ApplyExtLinearSort(tempArena, cpyGen, indices, info.length);

			// Copy new best instance to result if applicable.
			auto bestRating = genRatings[indices[0]];
			genRating = bestRating;

			if (bestRating > this->rating)
			{
				arena.DestroyScope(resScope);
				this->rating = bestRating;
				result = Copy(arena, *this, cpyGen[0]);
				genScope = arena.CreateScope();
			}

			tempArena.DestroyScope(tempScope);

			assert(info.breedablePct > 0 && info.breedablePct <= 1);
			const uint32_t apexLen = (float)info.length * info.apexPct;
			const uint32_t breedableLen = (float)info.length * info.breedablePct;
			const uint32_t end = info.length - (float)info.length * info.arrivalsPct;
			assert(end < info.length && end > breedableLen);

			// Breed successfull instances.
			for (uint32_t i = apexLen; i < end; i++)
			{
				uint32_t a = rand() % apexLen;
				uint32_t b = rand() % breedableLen;
				generation[i] = Breed(arena, *this, cpyGen[a], cpyGen[b]);
			}
			// Create new instances.
			for (uint32_t i = end; i < info.length; i++)
				CreateInstance(arena, *this, generation[i]);
		}
	}

	Group Group::Create(Arena& arena, const GroupCreateInfo& info)
	{
		Group group{};
		group.info = info;
		group.scope = arena.CreateScope();
		group.genRatings = arena.New<float>(info.length);

		group.resScope = arena.CreateScope();
		CreateInstance(arena, group, group.result);

		group.genScope = arena.CreateScope();
		group.generation = CreateArray<Instance>(arena, info.length);
		for (auto& instance : group.generation)
			CreateInstance(arena, group, instance);

		return group;
	}
	void Group::Destroy(Group& group, Arena& arena)
	{
		arena.DestroyScope(group.scope);
	}
}