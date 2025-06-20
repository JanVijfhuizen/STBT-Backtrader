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
			if (!weight.enabled)
				continue;
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
	void Instance::Flush()
	{
		for (auto& neuron : neurons)
			neuron.value = 0;
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
		// First flush trainee.
		auto& instance = generation[trainId];
		instance.Flush();
		return instance;
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

	void MutateF(float& f, const float min, const float max, const GroupCreateInfo& info)
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
			f = RandF(min, max);
			break;
		}
		f = Clamp(f, min, max);
	}

	void Mutate(Arena& arena, Group& group, Instance& instance, const bool addNeuron, const bool addWeight)
	{
		const auto& info = group.info;

		// This also possibly mutates the "blank" 
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

		if (addWeight)
		{
			uint32_t offset = 1 + (addNeuron ? 2 : 0);
			auto& weight = instance.weights[instance.weights.length - offset];
			weight.dominance = RandF(0, 1);
			weight.id = group.gId++;
			weight.propagations = RandF(0, info.maxPropagations);
			weight.from = rand() % instance.neurons.length;
			do
			{
				weight.to = rand() % instance.neurons.length;
			} while (weight.to == weight.from);
		}

		if (addNeuron)
		{
			auto& neuron = instance.neurons[instance.neurons.length - 1];
			neuron.id = group.gId++;
			neuron.threshold = RandF(0, 1);
			neuron.dominance = RandF(0, 1);
			neuron.decay = RandF(-1, 1);

			uint32_t from = rand() % instance.neurons.length;
			uint32_t to;
			do
			{
				to = rand() % instance.neurons.length;
			} while (to == from);

			uint32_t conns[]
			{
				from, 
				instance.neurons.length - 1,
				to
			};

			for (uint32_t i = 0; i < 2; i++)
			{
				auto& w = instance.weights[instance.weights.length - i - 1];
				w.dominance = RandF(0, 1);
				w.id = group.gId++;
				w.propagations = RandF(0, info.maxPropagations);
				w.from = conns[i];
				w.to = conns[i + 1];
			}
		}
	}

	Instance Breed(Arena& arena, Arena& tempArena, Group& group, Instance& a, Instance& b)
	{
		const auto tempScope = tempArena.CreateScope();

		auto neurons = CreateVector<Neuron>(tempArena, a.neurons.length + b.neurons.length);
		uint32_t iA = 0, iB = 0;

		while (iA < a.neurons.length && iB < b.neurons.length)
		{
			auto& nA = a.neurons[iA];
			auto& nB = b.neurons[iB];

			const bool equal = nA.id == nB.id;
			const bool dir = nA.id < nB.id;

			iA += equal ? 1 : dir ? 1 : 0;
			iB += equal ? 1 : dir ? 0 : 1;

			// Pick lowest and continue.
			if (!equal)
			{
				neurons.Add() = dir ? nA : nB;
				continue;
			}

			// Pick between the two based on dominance factor.
			const bool domDir = RandF(0, nA.dominance + nB.dominance) < nA.dominance;
			neurons.Add() = domDir ? nA : nB;
		}

		// Add remaining neurons.
		while (iA < a.neurons.length)
			neurons.Add() = a.neurons[iA++];
		while (iB < b.neurons.length)
			neurons.Add() = b.neurons[iB++];

		// Do the same thing for weights.
		auto weights = CreateVector<Weight>(tempArena, a.weights.length + b.weights.length);
		iA = 0; iB = 0;

		while (iA < a.weights.length && iB < b.weights.length)
		{
			auto& wA = a.weights[iA];
			auto& wB = b.weights[iB];

			const bool equal = wA.id == wB.id;
			const bool dir = wA.id < wB.id;

			iA += equal ? 1 : dir ? 1 : 0;
			iB += equal ? 1 : dir ? 0 : 1;

			// Pick lowest and continue.
			if (!equal)
			{
				weights.Add() = dir ? wA : wB;
				continue;
			}

			// If one is disabled, automatically pick that one.
			// If both are disabled, just pick one. Doesn't really matter in that case.
			bool disInd = -1;
			disInd = wA.enabled ? disInd : 0;
			disInd = wB.enabled ? disInd : 1;
			if (disInd != -1)
			{
				weights.Add() = disInd == 1 ? wA : wB;
				continue;
			}

			// Pick between the two based on dominance factor.
			const bool domDir = RandF(0, wA.dominance + wB.dominance) < wA.dominance;
			weights.Add() = domDir ? wA : wB;
		}

		// Add remaining weights.
		while (iA < a.weights.length)
			weights.Add() = a.weights[iA++];
		while (iB < b.weights.length)
			weights.Add() = b.weights[iB++];

		const bool addWeight = RandF(0, 1) < group.info.mutateNewWeightChance;
		const bool addNeuron = RandF(0, 1) < group.info.mutateNewNodeChance;

		// Create instance (with larger size if it's going to mutate new topology)
		Instance instance{};
		instance.neurons = CreateArray<Neuron>(arena, neurons.count + addNeuron);
		for (uint32_t i = 0; i < neurons.count; i++)
			instance.neurons[i] = neurons[i];
		instance.weights = CreateArray<Weight>(arena, weights.count + 2 * addNeuron + addWeight);
		for (uint32_t i = 0; i < weights.count; i++)
			instance.weights[i] = weights[i];
		Mutate(arena, group, instance, addNeuron, addWeight);

		tempArena.DestroyScope(tempScope);
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

			assert(info.breedablePct > 0 && info.breedablePct <= 1);
			const uint32_t apexLen = (float)info.length * info.apexPct;
			const uint32_t breedableLen = (float)info.length * info.breedablePct;
			const uint32_t end = info.length - (float)info.length * info.arrivalsPct;
			assert(end < info.length && end > breedableLen);

			// Breed successfull instances.
			for (uint32_t i = 0; i < end; i++)
			{
				uint32_t a = rand() % apexLen;
				uint32_t b = rand() % breedableLen;
				generation[i] = Breed(arena, tempArena, *this, cpyGen[a], cpyGen[b]);
			}
			// Create new instances.
			for (uint32_t i = end; i < info.length; i++)
				CreateInstance(arena, *this, generation[i]);

			tempArena.DestroyScope(tempScope);
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