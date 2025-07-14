#include "pch.h"
#include "Algorithms/DynNNet.h"
#include <Jlib/MapUtils.h>
#include <JLib/ArrayUtils.h>
#include <JLib/QueueUtils.h>
#include <Algorithms/KMeans.h>

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

	[[nodiscard]] uint32_t Dist(const DynInstance& instance, int32_t* map, const uint32_t neuronCount)
	{
		uint32_t d = 0;
		for (auto& n : instance.neurons)
			d += map[n] <= 0;
		for (auto& w : instance.weights)
			d += map[neuronCount + w] <= 0;
		return d;
	}

	void Clear(int32_t* map, const uint32_t count)
	{
		for (uint32_t i = 0; i < count; i++)
			map[i] = 0;
	}

	void Add(int32_t* map, const DynInstance& instance, const uint32_t neuronCount)
	{
		for (auto& n : instance.neurons)
			++map[n];
		for (auto& w : instance.weights)
			++map[neuronCount + w];
	}

	Array<uint32_t> ApplyKMeans(Arena& arena, Arena& tempArena, DynNNet& nnet, DynInstance* cpyGen, const uint32_t maxCycles)
	{
		auto arr = CreateArray<uint32_t>(arena, nnet.info.generationSize);

		auto tempScope = tempArena.CreateScope();

		auto closedPoints = CreateVector<uint32_t>(tempArena, nnet.kmPointCount);
		auto points = CreateArray<int32_t*>(tempArena, nnet.kmPointCount);

		const uint32_t l = nnet.info.generationSize;

		// Convert random instances in semi "bitmaps".
		for (uint32_t i = 0; i < nnet.kmPointCount; i++)
		{
			uint32_t rInd;
			do
			{
				rInd = rand() % l;
			} while (Contains(closedPoints, rInd) != -1);

			closedPoints.Add() = rInd;
			auto& point = points[i] = tempArena.New<int32_t>(nnet.neurons.count + nnet.weights.count);
			auto& instance = cpyGen[rInd];

			for (auto& n : instance.neurons)
				point[n] = 1;
			for (auto& w : instance.weights)
				point[nnet.neurons.count + w] = 1;
		}

		uint32_t i = 0;
		for (; i < maxCycles; i++)
		{
			bool changed = false;

			// Assign instances to closest points.
			for (uint32_t j = 0; j < l; j++)
			{
				float minDis = FLT_MAX;
				uint32_t p = 0;

				for (uint32_t k = 0; k < nnet.kmPointCount; k++)
				{
					uint32_t dst = Dist(cpyGen[j], points[k], nnet.neurons.count);
					if (dst < minDis)
					{
						minDis = dst;
						p = k;
					}
				}

				// Assign point to instance.
				changed = changed ? true : arr[j] != p;
				arr[j] = p;
			}

			if (!changed)
				break;

			// Set points to the average of their positions.
			if (i < maxCycles - 1)
			{
				// Reset the points.
				const uint32_t c = nnet.neurons.count + nnet.weights.count;
				for (uint32_t j = 0; j < nnet.kmPointCount; j++)
					Clear(points[j], c);

				auto tScope = tempArena.CreateScope();
				auto counts = CreateArray<uint32_t>(tempArena, nnet.kmPointCount);

				// Get average of point instances.
				for (uint32_t j = 0; j < l; j++)
				{
					const uint32_t ind = arr[j];
					++counts[ind];
					Add(points[ind], cpyGen[j], nnet.neurons.count);
				}
				for (uint32_t j = 0; j < nnet.kmPointCount; j++)
				{
					auto point = points[j];
					for (uint32_t i = 0; i < c; i++)
					{
						auto& p = point[i];
						p = p >= ((l + 1) / 2);
					}
				}

				tempArena.DestroyScope(tScope);
			}
		}

		tempArena.DestroyScope(tempScope);
		return arr;
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
				uint32_t from = rand() % (neurons.count - info.outputCount);
				// If this is in the output range, move it out of there.
				if (from >= info.inputCount && from < (info.inputCount + info.outputCount))
					from += info.outputCount;

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

	[[nodiscard]] size_t GetParameterSize(const DynInstance& instance)
	{
		return instance.neurons.length * NEURON_VARIABLE_COUNT + instance.weights.length;
	}

	[[nodiscard]] Array<DynInstance::OutputType> GetOutputTypes(Arena& arena, const DynType type)
	{
		Array<DynInstance::OutputType> ret;
		switch (type)
		{
		case DynType::classification:
			ret = CreateArray<DynInstance::OutputType>(arena, 2);
			ret[0] = DynInstance::OutputType::lin;
			ret[1] = DynInstance::OutputType::softmax;
			break;
		default:
			ret = CreateArray<DynInstance::OutputType>(arena, 1);
			ret[0] = DynInstance::OutputType::lin;
			break;
		}

		return ret;
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

		Mutate(nnet, neurons, weights, rand() % (info.initialAlpha + 1));

		instance.neurons = CreateArray<uint32_t>(arena, neurons.count);
		instance.weights = CreateArray<uint32_t>(arena, weights.count);
		memcpy(instance.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.count);
		memcpy(instance.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.count);

		instance.parameters = arena.New<float>(GetParameterSize(instance));

		// Choose output type. This never mutates.
		auto oTypes = GetOutputTypes(tempArena, info.type);
		instance.outputType = Rand(oTypes);

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
			neuron.cWeights = CreateArray<uint32_t>(arena, nums[i]);
		}

		for (auto& i : nums)
			i = 0;
		for (const auto& i : instance.weights)
		{
			auto& weight = weights[i];
			auto& n = nums[weight.from];
			auto& cWeight = neurons[weight.from].cWeights[n++];
			cWeight = i;
		}

		outputType = instance.outputType;
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
			
			const uint32_t typeCount = static_cast<uint32_t>(Neuron::Type::output);
			const float f = values[ind];
			const uint32_t currentType = floor((f + 1) / 2 * typeCount);
			const Neuron::Type convType = static_cast<Neuron::Type>(currentType);
			neuron.type = convType;

			switch (convType)
			{
			case Neuron::Type::spike:
				neuron.spike.decay = values[ind + 1];
				neuron.spike.threshold = values[ind + 2];
				break;
			case Neuron::Type::step:
				neuron.step.threshold = values[ind + 1];
				break;
			case Neuron::Type::gauss:
				neuron.gauss.mean = values[ind + 1];
				neuron.gauss.stddev = values[ind + 2];
				break;
			default:
				break;
			}
		}

		// Always set the output to a single type.
		for (uint32_t i = 0; i < info.outputCount; i++)
			neurons[info.inputCount + i].type = Neuron::Type::output;
	}

	void DynNNet::CreateParameters(Arena& arena)
	{
		parameterScope = arena.CreateScope();

		const auto& current = GetCurrent();

		GeneticAlgorithmCreateInfo info{};
		info.length = gaLength;
		// Neuron length first value is the type. First values are shared ones and the rest is type specific.
		info.width = GetParameterSize(current);
		info.mutateChance = gaMutateChance;
		info.mutateAddition = gaMutateAddition;
		info.mutateMultiplier = gaMutateMultiplier;
		info.kmPointCount = gaKmPointCount;
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

	// Single parent breeding.
	DynInstance Breed(Arena& arena, Arena& tempArena, DynNNet& nnet, const DynInstance& a)
	{
		const auto tempScope = tempArena.CreateScope();

		auto neurons = CreateVector<uint32_t>(tempArena, a.neurons.length + nnet.alpha);
		auto weights = CreateVector<uint32_t>(tempArena, a.weights.length + nnet.alpha * 2);
		memcpy(neurons.ptr, a.neurons.ptr, sizeof(uint32_t) * a.neurons.length);
		memcpy(weights.ptr, a.weights.ptr, sizeof(uint32_t) * a.weights.length);
		neurons.count = a.neurons.length;
		weights.count = a.weights.length;

		Mutate(nnet, neurons, weights, rand() % (nnet.alpha + 1));

		DynInstance instance{};
		instance.neurons = CreateArray<uint32_t>(arena, neurons.count);
		instance.weights = CreateArray<uint32_t>(arena, weights.count);
		memcpy(instance.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.count);
		memcpy(instance.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.count);

		instance.parameters = arena.New<float>(GetParameterSize(instance));
		instance.outputType = a.outputType;

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
				generation[i].Copy(tempArena, cpyGen[i], true);

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
				cpyGen[0].Copy(arena, result, true);
				generationScope = arena.CreateScope();
			}

			assert(breedablePct > 0 && breedablePct <= 1);
			const uint32_t apexLen = (float)length * apexPct;
			const uint32_t breedableLen = (float)length * breedablePct;
			const uint32_t end = length - (float)length * arrivalsPct;
			assert(end < length && end > breedableLen);

			if (kmPointCount > 1)
			{
				auto res = ApplyKMeans(arena, tempArena, *this, cpyGen, kmCycles);
				const auto conv = jv::ConvKMeansRes(arena, tempArena, res, kmPointCount);

				// Now order the breedables based on roundabout
				uint32_t offset = 0;
				for (uint32_t i = 0; i < breedableLen; i++)
				{
					uint32_t groupId;
					uint32_t ind;
					uint32_t l;

					do
					{
						groupId = offset++ % kmPointCount;
						ind = offset / kmPointCount;
						l = conv[groupId].length;
					} while (l <= ind);

					const auto temp = cpyGen[i];
					auto& apexInst = cpyGen[conv[groupId][ind]];

					// Swap places.
					cpyGen[i] = apexInst;
					apexInst = temp;
				}
			}

			// Copy apex.
			for (uint32_t i = 0; i < apexLen; i++)
				cpyGen[i].Copy(arena, generation[i], true);

			// Copy and mutate from apex.
			for (uint32_t i = apexLen; i < end; i++)
			{
				uint32_t a = rand() % breedableLen;
				generation[i] = Breed(arena, tempArena, *this, cpyGen[a]);
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
		if (ga.trainId == 0)
		{
			auto& instance = generation[currentId];
			const auto result = ga.result;
			const size_t s = sizeof(float) * ga.info.width;
			memcpy(instance.parameters, result, s);
		}
	}

	void DynNNet::Flush(DynInstance& instance)
	{
		for (auto& n : instance.neurons)
			neurons[n].value = 0;
	}

	void Linear(const DynNNet& nnet, const Array<float>& output)
	{
		const uint32_t iC = nnet.info.inputCount;
		const uint32_t oC = nnet.info.outputCount;

		for (uint32_t i = 0; i < oC; i++)
			output[i] = nnet.neurons[iC + i].value;
	}

	void SoftMax(const DynNNet& nnet, const Array<float>& output)
	{
		const uint32_t iC = nnet.info.inputCount;
		const uint32_t oC = nnet.info.outputCount;

		float sum = 0;
		for (uint32_t i = 0; i < oC; i++)
		{
			auto& o = output[i] = nnet.neurons[iC + i].value;
			o = exp(o);
			sum += o;
		}
		sum = 1.f / sum;
		for (uint32_t i = 0; i < oC; i++)
			output[i] *= sum;
	}

	void DynNNet::Propagate(Arena& tempArena, const Array<float>& input, const Array<float>& output)
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
			case Neuron::Type::abs:
				propagatedValue = abs(neuron.value);
				neuron.value = 0;
				break;
			case Neuron::Type::inv:
				propagatedValue = -neuron.value;
				neuron.value = 0;
				break;
			case Neuron::Type::relu:
				propagatedValue = Max(0.f, neuron.value);
				neuron.value = 0;
				break;
			case Neuron::Type::sin:
				propagatedValue = sin(neuron.value);
				neuron.value = 0;
				break;
			case Neuron::Type::cos:
				propagatedValue = cos(neuron.value);
				neuron.value = 0;
				break;
			case Neuron::Type::step:
				propagatedValue = neuron.value > neuron.step.threshold;
				neuron.value = 0;
				break;
			case Neuron::Type::tanh:
				propagatedValue = tanh(neuron.value);
				neuron.value = 0;
				break;
			case Neuron::Type::lin:
				propagatedValue = neuron.value;
				neuron.value = 0;
				break;
			case Neuron::Type::gauss:
				propagatedValue = Gauss(neuron.value, abs(neuron.gauss.mean), abs(neuron.gauss.stddev));
				neuron.value = 0;
				break;
			default:
				break;
			}

			// Propagate through weights.
			for (auto& cWeight : neuron.cWeights)
			{
				auto& weight = weights[cWeight];
				auto& toNeuron = neurons[weight.to];

				queue.Add() = weight.to;
				toNeuron.value += weight.value * propagatedValue;
			}
		}

		switch (outputType)
		{
		case DynInstance::OutputType::softmax:
			SoftMax(*this, output);
			break;
		default:
			Linear(*this, output);
			break;
		}

		for (auto& neuron : neurons)
		{
			switch (neuron.type)
			{
				// Update neuron values with decay.
			case Neuron::Type::spike:
				neuron.value -= neuron.spike.decay;
				break;
				// Doubles as output.
			case Neuron::Type::output:
				neuron.value = 0;
				break;
			default:
				break;
			}

			neuron.value = Max(neuron.value, 0.f);
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
	void DynInstance::Copy(Arena& arena, DynInstance& dst, const bool copyParameters) const
	{
		dst.neurons = CreateArray<uint32_t>(arena, neurons.length);
		dst.weights = CreateArray<uint32_t>(arena, weights.length);
		memcpy(dst.neurons.ptr, neurons.ptr, sizeof(uint32_t) * neurons.length);
		memcpy(dst.weights.ptr, weights.ptr, sizeof(uint32_t) * weights.length);

		const size_t s = GetParameterSize(*this);
		dst.parameters = arena.New<float>(s);
		if (copyParameters)
			memcpy(dst.parameters, parameters, s);
		dst.outputType = outputType;
	}
}
