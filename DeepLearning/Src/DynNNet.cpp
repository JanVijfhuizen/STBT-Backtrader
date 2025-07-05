#include "pch.h"
#include "Algorithms/DynNNet.h"
#include <Jlib/MapUtils.h>
#include <JLib/ArrayUtils.h>
#include <JLib/QueueUtils.h>

namespace jv::ai
{
	const uint32_t NEURON_VARIABLE_COUNT = 3;

	struct Key final
	{
		union
		{
			struct
			{
				uint32_t from;
				uint32_t to;
			};
			uint64_t value;
		};

		Key(const uint32_t from, const uint32_t to)
		{
			this->from = from;
			this->to = to;
		}
	};

	bool Comparer(float& a, float& b)
	{
		return a > b;
	}

	bool ComparerI(uint32_t& a, uint32_t& b)
	{
		return a < b;
	}

	void Mutate(DynNNet& nnet, Vector<uint32_t>& neurons, Vector<uint32_t>& weights, const uint32_t times)
	{
		for (uint32_t i = 0; i < times; i++)
		{
			const bool canGenerateNeuron = nnet.neurons.count < nnet.neurons.length && 
				nnet.weights.count < nnet.weights.length - 1;
			const bool canGenerateWeight = nnet.weights.count < nnet.weights.length;

			if (canGenerateNeuron && RandF(0, 1) > nnet.weightToNeuronMutateChance)
			{
				// Mutate into a neuron (+ corresponding weights).
				const uint32_t refWeightIndex = rand() % weights.count;
				const uint32_t weightIndex = weights[refWeightIndex];
				const auto& weight = nnet.weights[weightIndex];

				const Key key(weight.from, weight.to);
				const uint64_t* index = nnet.neuronMap.Contains(key.value);

				bool valid = true;

				if (index)
				{
					// Check if this instance already has this neuron.			
					for(auto& neuron : neurons)
						if (neuron == *index)
						{
							valid = false;
							break;
						}

					// ONLY if the neuron does not already exist in this instance, will it be added.
					if (valid)
					{
						// If the mutation is already known to the nnet, use that one.
						neurons.Add() = *index;

						// We can assume the connections will also be there.
						const Key inpKey(weight.from, *index);
						const Key outpKey(*index, weight.to);
						weights.Add() = *nnet.weightMap.Contains(inpKey.value);
						weights.Add() = *nnet.weightMap.Contains(outpKey.value);
					}
				}
				else
				{
					// This is a completely new mutation, so add it to the registry.
					const uint32_t index = nnet.neurons.count;
					neurons.Add() = index;
					nnet.neurons.Add();
					nnet.neuronMap.Insert(index, key.value);

					const Key inpKey(weight.from, index);
					const Key outpKey(index, weight.to);

					uint32_t weightIndex = nnet.weights.count;
					auto& fromWeight = nnet.weights.Add();
					fromWeight.from = weight.from;
					fromWeight.to = index;
					nnet.weightMap.Insert(weightIndex, inpKey.value);
					weights.Add() = weightIndex;
					
					weightIndex++;
					auto& toWeight = nnet.weights.Add();
					toWeight.from = index;
					toWeight.to = weight.to;
					nnet.weightMap.Insert(weightIndex, outpKey.value);
					weights.Add() = weightIndex;
				}

				// Remove old weight.
				weights.RemoveAtOrdered(refWeightIndex);
			}
			else if(canGenerateWeight)
			{
				// Mutate into a weight.
				const auto& info = nnet.info;
				const uint32_t from = rand() % (neurons.count - info.outputCount);
				const uint32_t to = info.inputCount + (rand() % (neurons.count - info.inputCount));

				const Key key(from, to);
				if (!nnet.neuronMap.Contains(key.value))
				{
					const uint32_t weightIndex = nnet.weights.count;
					const Key key(from, to);
					auto& newWeight = nnet.weights.Add();
					newWeight.from = from;
					newWeight.to = to;
					nnet.weightMap.Insert(weightIndex, key.value);
					weights.Add() = weightIndex;
				}
			}
		}
		LinearSort(neurons.ptr, neurons.count, ComparerI);
		LinearSort(weights.ptr, weights.count, ComparerI);
	}

	DynInstance CreateArrivalInstance(Arena& arena, Arena& tempArena, DynNNet& nnet)
	{
		DynInstance instance{};
		const auto tempScope = tempArena.CreateScope();
		
		const auto& info = nnet.info;
		const uint32_t weightInitialCount = info.connectStartingNeurons ? info.inputCount * info.outputCount : 0;
		auto neurons = CreateVector<uint32_t>(tempArena, info.inputCount + info.outputCount + info.initialAlpha);
		// Multiply by 2 because every neuron will also come with 2 new weights.
		auto weights = CreateVector<uint32_t>(tempArena, weightInitialCount + info.initialAlpha * 2);

		const uint32_t predefNeuronCount = info.inputCount + info.outputCount;
		neurons.count = predefNeuronCount;
		for (uint32_t i = 0; i < predefNeuronCount; i++)
			neurons[i] = i;
		if (info.connectStartingNeurons)
		{
			const uint32_t predefWeightCount = info.inputCount * info.outputCount;
			weights.count = predefWeightCount;
			for (uint32_t i = 0; i < predefWeightCount; i++)
				weights[i] = i;
		}

		Mutate(nnet, neurons, weights, info.initialAlpha);

		instance.neurons = CreateArray<uint32_t>(arena, neurons.count);
		instance.weights = CreateArray<uint32_t>(arena, weights.count);
		memcpy(instance.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.count);
		memcpy(instance.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.count);

		tempArena.DestroyScope(tempScope);
		return instance;
	}
	void DynNNet::Construct(Arena& arena, Arena& tempArena, const DynInstance& instance)
	{
		constructScope = arena.CreateScope();
		const auto tempScope = tempArena.CreateScope();

		const auto nums = CreateArray<uint32_t>(tempArena, neurons.count);
		for (const auto& i : instance.weights)
		{
			auto& weight = weights[i];
			nums[weight.from]++;
		}

		for (uint32_t i = 0; i < neurons.count; i++)
		{
			if (nums[i] == 0)
				continue;
			auto& neuron = neurons[i];
			neuron.cWeights = CreateArray<CWeight>(arena, nums[i]);
		}

		for (auto& i : nums)
			i = 0;
		for (const auto& i : instance.weights)
		{
			auto& weight = weights[i];
			auto& n = nums[weight.from];
			auto& cWeight = neurons[weight.from].cWeights[n++];
			cWeight.index = i;
		}

		tempArena.DestroyScope(tempScope);
	}
	void DynNNet::Deconstruct(Arena& arena, const DynInstance& instance)
	{
		for (auto& neuron : neurons)
			neuron.cWeights = {};
		arena.DestroyScope(constructScope);
	}

	void DynNNet::ConstructParameters(DynInstance& instance, float* values)
	{
		// First do the weights (it's more straightforward)
		for (uint32_t i = 0; i < instance.weights.length; i++)
		{
			const uint32_t w = instance.weights[i];
			auto& weight = weights[w];
			weight.value = values[i];
		}

		// Now do the neurons, which gets a bit more complicated due to types.
		for (uint32_t i = 0; i < instance.neurons.length; i++)
		{
			const uint32_t n = instance.neurons[i];
			auto& neuron = neurons[n];

			const uint32_t ind = instance.weights.length + i * NEURON_VARIABLE_COUNT;
			
			const uint32_t typeCount = static_cast<uint32_t>(Neuron::Type::length);
			const uint32_t currentType = static_cast<uint32_t>(round(values[ind] * typeCount));
			const Neuron::Type convType = static_cast<Neuron::Type>(currentType);
			neuron.type = convType;

			switch (convType)
			{
			case Neuron::Type::spike:
				neuron.spike.decay = values[ind + 1];
				neuron.spike.threshold = values[ind + 2];
				break;
			default:
				break;
			}
		}
	}

	void DynNNet::CreateParameters(Arena& arena)
	{
		parameterScope = arena.CreateScope();

		const auto& current = GetCurrent();

		GeneticAlgorithmCreateInfo info{};
		info.length = gaLength;
		// Neuron length first value is the type. First values are shared ones and the rest is type specific.
		info.width = current.neurons.length * NEURON_VARIABLE_COUNT + current.weights.length;
		info.mutateChance = gaMutateChance;
		info.mutateAddition = gaMutateAddition;
		info.mutateMultiplier = gaMutateMultiplier;
		ga = GeneticAlgorithm::Create(arena, info);
	}

	void DynNNet::DestroyParameters(Arena& arena)
	{
		arena.DestroyScope(parameterScope);
	}

	// Source: https://stackoverflow.com/questions/10732027/fast-sigmoid-algorithm
	float FastSigmoid(const float x)
	{
		return x / (1.f + abs(x));
	}

	DynInstance& DynNNet::GetCurrent()
	{
		return generation[currentId];
	}

	DynInstance Breed(Arena& arena, Arena& tempArena, DynNNet& nnet, const DynInstance& a, const DynInstance& b)
	{
		const auto tempScope = tempArena.CreateScope();

		auto neurons = CreateVector<uint32_t>(tempArena, a.neurons.length + b.neurons.length + nnet.alpha);
		uint32_t iA = 0, iB = 0;

		while (iA < a.neurons.length && iB < b.neurons.length)
		{
			const uint32_t nA = a.neurons[iA];
			const uint32_t nB = b.neurons[iB];

			const bool equal = nA == nB;
			const bool dir = nA < nB;

			iA += equal ? 1 : dir ? 1 : 0;
			iB += equal ? 1 : dir ? 0 : 1;

			// Pick lowest and continue.
			if (!equal)
			{
				neurons.Add() = dir ? nA : nB;
				continue;
			}

			// Just pick the first one, since they're the same anyway.
			neurons.Add() = nA;
		}

		// Add remaining neurons.
		while (iA < a.neurons.length)
			neurons.Add() = a.neurons[iA++];
		while (iB < b.neurons.length)
			neurons.Add() = b.neurons[iB++];

		// Do the same thing for weights.
		auto weights = CreateVector<uint32_t>(tempArena, a.weights.length + b.weights.length + nnet.alpha * 2);
		iA = 0; iB = 0;

		while (iA < a.weights.length && iB < b.weights.length)
		{
			const uint32_t wA = a.weights[iA];
			const uint32_t wB = b.weights[iB];

			const bool equal = wA == wB;
			const bool dir = wA < wB;

			iA += equal ? 1 : dir ? 1 : 0;
			iB += equal ? 1 : dir ? 0 : 1;

			if (!equal)
			{
				weights.Add() = dir ? wA : wB;
				continue;
			}

			weights.Add() = wA;
		}

		while (iA < a.weights.length)
			weights.Add() = a.weights[iA++];
		while (iB < b.weights.length)
			weights.Add() = b.weights[iB++];

		Mutate(nnet, neurons, weights, nnet.alpha);

		DynInstance instance{};
		instance.neurons = CreateArray<uint32_t>(arena, neurons.count);
		instance.weights = CreateArray<uint32_t>(arena, weights.count);
		memcpy(instance.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.count);
		memcpy(instance.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.count);

		tempArena.DestroyScope(tempScope);
		return instance;
	}

	void DynNNet::Rate(Arena& arena, Arena& tempArena)
	{
		ratings[currentId++] = ga.rating;
		if (currentId >= generation.length)
		{
			currentId = 0;
			++generationId;

			auto tempScope = tempArena.CreateScope();

			// Copy all instances to temp arena.
			const uint32_t length = generation.length;
			DynInstance* cpyGen = tempArena.New<DynInstance>(length);
			for (uint32_t i = 0; i < length; i++)
				generation[i].Copy(tempArena, cpyGen[i]);

			arena.DestroyScope(generationScope);

			auto indices = tempArena.New<uint32_t>(length);
			jv::CreateSortableIndices(indices, length);
			jv::ExtLinearSort(ratings, indices, length, Comparer);
			jv::ApplyExtLinearSort(tempArena, cpyGen, indices, length);

			// Copy new best instance to result if applicable.
			auto bestRating = ratings[indices[0]];
			generationRating = bestRating;

			if (bestRating > this->rating)
			{
				arena.DestroyScope(resultScope);
				this->rating = bestRating;
				cpyGen[0].Copy(arena, result);
				generationScope = arena.CreateScope();
			}

			assert(breedablePct > 0 && breedablePct <= 1);
			const uint32_t apexLen = (float)length * apexPct;
			const uint32_t breedableLen = (float)length * breedablePct;
			const uint32_t end = length - (float)length * arrivalsPct;
			assert(end < length && end > breedableLen);

			// Breed successfull instances.
			for (uint32_t i = 0; i < end; i++)
			{
				uint32_t a = rand() % apexLen;
				uint32_t b = rand() % breedableLen;
				generation[i] = Breed(arena, tempArena, *this, cpyGen[a], cpyGen[b]);
			}

			// Create new instances.
			for (uint32_t i = end; i < length; i++)
				generation[i] = CreateArrivalInstance(arena, tempArena, *this);

			tempArena.DestroyScope(tempScope);
		}
	}

	float* DynNNet::GetCurrentParameters()
	{
		return ga.GetTrainee();
	}

	void DynNNet::RateParameters(Arena& arena, Arena& tempArena, float rating)
	{
		ga.Rate(arena, tempArena, rating, nullptr);
	}

	void DynNNet::Flush(DynInstance& instance)
	{
		for (auto& n : instance.neurons)
			neurons[n].value = 0;
	}

	void DynNNet::Propagate(Arena& tempArena, const Array<float>& input, const Array<bool>& output)
	{
		const auto tempScope = tempArena.CreateScope();

		// Propagate input.
		for (uint32_t i = 0; i < input.length; i++)
			neurons[i].value += input[i];

		auto queue = CreateQueue<uint32_t>(tempArena, neurons.length - 1);
		for (uint32_t i = 0; i < input.length; i++)
			queue.Add() = i;

		for (auto& neuron : neurons)
			neuron.signalled = false;

		while (queue.count > 0)
		{
			const uint32_t current = queue.Pop();
			auto& neuron = neurons[current];

			// Each neuron can only signal once per propagation.
			// This avoids infinite loops.
			if (neuron.signalled)
				continue;

			bool fire = true;
			switch (neuron.type)
			{
			case Neuron::Type::spike:
				if (neuron.value < neuron.spike.threshold)
					fire = false;
				break;
			default:
				break;
			}
			if (!fire)
				continue;

			neuron.signalled = true;

			float propagatedValue = 0;
			uint32_t signalCount;
			switch (neuron.type)
			{
			case Neuron::Type::spike:
				signalCount = neuron.value / neuron.spike.threshold;
				propagatedValue = neuron.spike.threshold * signalCount;
				neuron.value -= propagatedValue;
				break;
			case Neuron::Type::sigmoid:
				propagatedValue = FastSigmoid(neuron.value);
				neuron.value = 0;
				break;
			case Neuron::Type::sine:
				propagatedValue = sin(neuron.value);
				neuron.value = 0;
				break;
			default:
				break;
			}

			// Propagate through weights.
			for (auto& cWeight : neuron.cWeights)
			{
				auto& weight = weights[cWeight.index];
				auto& toNeuron = neurons[weight.to];

				queue.Add() = weight.to;
				toNeuron.value += cWeight.mul * propagatedValue;
			}
		}

		for (auto& neuron : neurons)
		{
			switch (neuron.type)
			{
				// Update neuron values with decay.
			case Neuron::Type::spike:
				neuron.value -= neuron.spike.decay;
				break;
			default:
				break;
			}

			neuron.value = Max(neuron.value, 0.f);
		}

		// Return output.
		for (uint32_t i = 0; i < output.length; i++)
		{
			const uint32_t ind = input.length + i;
			output[i] = neurons[ind].signalled;
		}

		tempArena.DestroyScope(tempScope);
	}
	DynNNet DynNNet::Create(Arena& arena, Arena& tempArena, const DynNNetCreateInfo& info)
	{
		DynNNet nnet{};
		nnet.info = info;
		nnet.scope = arena.CreateScope();
		nnet.neurons = CreateVector<Neuron>(arena, info.neuronCapacity);
		nnet.weights = CreateVector<Weight>(arena, info.weightCapacity);
		nnet.neuronMap = CreateMap<uint64_t>(arena, info.neuronCapacity);
		nnet.weightMap = CreateMap<uint64_t>(arena, info.weightCapacity);
		nnet.ratings = arena.New<float>(info.generationSize);
		nnet.generation = CreateArray<DynInstance>(arena, info.generationSize);

		const uint32_t predefNeuronCount = info.inputCount + info.outputCount;
		nnet.neurons.count = predefNeuronCount;

		if (info.connectStartingNeurons)
			for (uint32_t i = 0; i < info.inputCount; i++)
				for (uint32_t j = 0; j < info.outputCount; j++)
				{
					auto& weight = nnet.weights.Add();
					weight.from = i;
					weight.to = info.inputCount + j;
				}	

		nnet.resultScope = arena.CreateScope();
		nnet.generationScope = arena.CreateScope();
		for (uint32_t i = 0; i < info.generationSize; i++)
		{
			auto& instance = nnet.generation[i];
			instance = CreateArrivalInstance(arena, tempArena, nnet);
		}

		return nnet;
	}
	void DynNNet::Destroy(Arena& arena, const DynNNet& nnet)
	{
		arena.DestroyScope(nnet.scope);
	}
	bool Neuron::Enabled() const
	{
		return cWeights.length > 0;
	}
	void DynInstance::Copy(Arena& arena, DynInstance& dst) const
	{
		dst.neurons = CreateArray<uint32_t>(arena, neurons.length);
		dst.weights = CreateArray<uint32_t>(arena, weights.length);
		memcpy(dst.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.length);
		memcpy(dst.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.length);
	}
}
