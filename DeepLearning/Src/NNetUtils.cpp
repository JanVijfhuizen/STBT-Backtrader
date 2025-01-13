#include "pch.h"
#include "NNetUtils.h"
#include "Jlib/Math.h"

namespace jv::ai
{
	IOLayers Init(NNet& nnet, const InitType initType, uint32_t& gId)
	{
		auto inputLayer = AddLayer(nnet, nnet.createInfo.inputSize, initType, gId);
		auto outputLayer = AddLayer(nnet, nnet.createInfo.outputSize, initType, gId);
		return { inputLayer, outputLayer };
	}

	Layer AddLayer(NNet& nnet, const uint32_t length, InitType initType, uint32_t& gId)
	{
		for (uint32_t i = 0; i < length; i++)
		{
			bool valid = true;
			switch (initType)
			{
			case InitType::flat:
				valid = AddNeuron(nnet, 1, 0, gId);
				break;
			case InitType::random:
				valid = AddNeuron(nnet, jv::RandF(0, 1), jv::RandF(0, 1), gId);
				break;
			default:
				break;
			}
			assert(valid);
		}
			
		return { nnet.neuronCount - length, nnet.neuronCount };
	}

	void Connect(NNet& nnet, Layer from, Layer to, InitType initType, uint32_t& gId)
	{
		const uint32_t inSize = from.to - from.from;
		const uint32_t outSize = to.to - to.from;

		for (uint32_t i = 0; i < inSize; i++)
			for (uint32_t j = 0; j < outSize; j++)
			{
				bool valid = true;
				switch (initType)
				{
				case InitType::flat:
					valid = AddWeight(nnet, from.from + i, to.from + j, 1, gId);
					break;
				case InitType::random:
					valid = AddWeight(nnet, from.from + i, to.from + j, jv::RandF(-1, 1), gId);
					break;
				default:
					break;
				}
				assert(valid);
			}	
	}

	void ConnectIO(NNet& nnet, const InitType initType, uint32_t& gId)
	{
		const uint32_t inSize = nnet.createInfo.inputSize;
		const uint32_t outSize = nnet.createInfo.outputSize;
		Connect(nnet, { 0, inSize }, { inSize, inSize + outSize }, initType, gId);
	}

	float GetCompability(NNet& a, NNet& b)
	{
		uint32_t errorCount = 0;
		uint32_t aC = 0;
		uint32_t bC = 0;
		uint32_t wC = 0;

		while (aC < a.weightCount && bC < b.weightCount)
		{
			auto& aW = a.weights[aC];
			auto& bW = b.weights[bC];
			
			const bool eq = aW.innovationId == bW.innovationId;
			if (!eq)
				++errorCount;
			aC += aW.innovationId < bW.innovationId || eq;
			bC += bW.innovationId < aW.innovationId || eq;
			++wC;
		}
		errorCount += a.weightCount - aC + b.weightCount - bC;
		return 1.f - static_cast<float>(errorCount) / static_cast<float>(wC);
	}
	NNet Breed(NNet& a, NNet& b, Arena& arena, Arena& tempArena)
	{
		const auto tempScope = tempArena.CreateScope();
		NNetCreateInfo createInfo = a.createInfo;
		createInfo.neuronCapacity += b.createInfo.neuronCapacity;
		createInfo.weightCapacity += b.createInfo.weightCapacity;
		auto tempNNet = CreateNNet(createInfo, tempArena);

		uint32_t aC = 0;
		uint32_t bC = 0;
		uint32_t nC = 0;

		// Ordered double insert for neurons.
		while (aC < a.neuronCount && bC < b.neuronCount)
		{
			auto& aN = a.neurons[aC];
			auto& bN = b.neurons[bC];

			const bool eq = aN.innovationId == bN.innovationId;
			auto& n = tempNNet.neurons[tempNNet.neuronCount++];

			// Either add neuron from a, b or random.
			if (aN.innovationId < bN.innovationId)
				n = aN;
			if (aN.innovationId == bN.innovationId)
				n = rand() % 2 ? aN : bN;
			if (aN.innovationId > bN.innovationId)
				n = bN;

			aC += aN.innovationId < bN.innovationId || eq;
			bC += bN.innovationId < aN.innovationId || eq;
			++nC;
		}
		while (aC < a.neuronCount)
			tempNNet.neurons[tempNNet.neuronCount++] = a.neurons[aC++];
		while (bC < b.neuronCount)
			tempNNet.neurons[tempNNet.neuronCount++] = b.neurons[bC++];
		
		// Ordered double insert for weights.
		// With the change that from/to will be replaced by the neuron's innovation id, so that it can be tracked
		// despite the changed topology.
		aC = bC = nC = 0;
		while (aC < a.weightCount && bC < b.weightCount)
		{
			auto& aW = a.weights[aC];
			auto& bW = b.weights[bC];

			if (!aW.enabled)
			{
				++aC;
				continue;
			}
			if (!bW.enabled)
			{
				++bC;
				continue;
			}

			const bool eq = aW.innovationId == bW.innovationId;
			auto& w = tempNNet.weights[tempNNet.weightCount++];

			NNet* n = nullptr;

			// Either add neuron from a, b or random.
			if (aW.innovationId < bW.innovationId)
			{
				n = &a;
				w = aW;
			}
				
			if (aW.innovationId == bW.innovationId)
			{
				const uint32_t r = rand() % 2;
				n = r ? &a : &b;
				w = r ? aW : bW;
			}
			if (aW.innovationId > bW.innovationId)
			{
				n = &b;
				w = bW;
			}

			w.from = n->neurons[w.from].innovationId;
			w.to = n->neurons[w.to].innovationId;
			w.next = w.next == UINT32_MAX ? w.next : n->weights[w.next].innovationId;

			aC += aW.innovationId < bW.innovationId || eq;
			bC += bW.innovationId < aW.innovationId || eq;
			++nC;
		}
		while (aC < a.weightCount)
		{
			auto& w = tempNNet.weights[tempNNet.weightCount++];
			auto& aW = a.weights[aC++];
			w = aW;
			w.from = a.neurons[aW.from].innovationId;
			w.to = a.neurons[aW.to].innovationId;
			w.next = w.next == UINT32_MAX ? w.next : a.weights[w.next].innovationId;
		}
			
		while (bC < b.weightCount)
		{
			auto& w = tempNNet.weights[tempNNet.weightCount++];
			auto& bW = b.weights[bC++];
			w = bW;
			w.from = b.neurons[bW.from].innovationId;
			w.to = b.neurons[bW.to].innovationId;
			w.next = w.next == UINT32_MAX ? w.next : b.weights[w.next].innovationId;
		}

		// Now change the neuron weight starts AND the weight from/to's, as well as the nexts.
		for (uint32_t i = 0; i < tempNNet.neuronCount; i++)
			tempNNet.neurons[i].weightsId = UINT32_MAX;

		for (int32_t i = tempNNet.weightCount - 1; i >= 0; i--)
		{
			auto& weight = tempNNet.weights[i];

			// Connect to neurons.
			for (uint32_t j = 0; j < tempNNet.neuronCount; j++)
			{
				auto& neuron = tempNNet.neurons[j];
				if (neuron.innovationId == weight.from)
				{
					weight.from = j;
					neuron.weightsId = i;
				}
				if (neuron.innovationId == weight.to)
					weight.to = j;
			}

			// Connect all weights.
			if (weight.next == UINT32_MAX)
				continue;

			for (int32_t j = i - 1; j >= 0; j--)
			{
				auto& oWeight = tempNNet.weights[j];
				if (oWeight.innovationId == weight.next)
				{
					weight.next = j;
					break;
				}
			}
		}

		// temp
		for (size_t i = 0; i < tempNNet.weightCount; i++)
		{
			auto& weight = tempNNet.weights[i];
			assert(weight.from < tempNNet.neuronCount);
			assert(weight.to < tempNNet.neuronCount);
			assert(weight.next < tempNNet.weightCount || weight.next == UINT32_MAX);
		}

		// Copy and optimize size, but still make sure it can mutate once.
		createInfo.neuronCapacity = tempNNet.neuronCount + 1;
		createInfo.weightCapacity = tempNNet.weightCount + 3;
		auto childNNet = CreateNNet(createInfo, arena);
		Copy(tempNNet, childNNet);

		tempArena.DestroyScope(tempScope);
		return childNNet;
	}

	void Mutate(NNet& nnet, const Mutations mutations, uint32_t& gId)
	{
		auto& weightMut = mutations.weight;
		if (weightMut.chance > 0)
		{
			for (size_t i = 0; i < nnet.weightCount; i++)
			{
				if (RandF(0, 1) > weightMut.chance)
					continue;

				auto& weight = nnet.weights[i];
				// 1 = new value, 2 = percent wise, 3 = linear addition/subtraction.
				uint32_t type = rand() % 3;
				weight.value = type != 0 || !mutations.weight.canRandomize ? weight.value : RandF(-1, 1);
				weight.value = type != 1 ? weight.value : weight.value * 
					RandF(1.f - weightMut.pctAlpha, 1.f + weightMut.pctAlpha);
				weight.value = type != 2 ? weight.value : weight.value + RandF(-1, 1) * weightMut.linAlpha;
			}
		}
		auto& thresholdMut = mutations.threshold;
		if (thresholdMut.chance > 0)
		{
			for (size_t i = 0; i < nnet.neuronCount; i++)
			{
				if (RandF(0, 1) > thresholdMut.chance)
					continue;

				auto& neuron = nnet.neurons[i];
				uint32_t type = rand() % 3;
				neuron.threshold = type != 0 || !mutations.threshold.canRandomize ? neuron.threshold : RandF(0, 1);
				neuron.threshold = type != 1 ? neuron.threshold : neuron.threshold *
					RandF(1.f - thresholdMut.pctAlpha, 1.f + thresholdMut.pctAlpha);
				neuron.threshold = type != 2 ? neuron.threshold : neuron.threshold + RandF(-1, 1) * thresholdMut.linAlpha;
				neuron.threshold = Max<float>(neuron.threshold, .1);
			}
		}
		auto& decayMut = mutations.decay;
		if (decayMut.chance > 0)
		{
			for (size_t i = 0; i < nnet.neuronCount; i++)
			{
				if (RandF(0, 1) > decayMut.chance)
					continue;

				auto& neuron = nnet.neurons[i];
				uint32_t type = rand() % 3;
				neuron.decay = type != 0 || !mutations.decay.canRandomize ? neuron.decay : RandF(0, 1);
				neuron.decay = type != 1 ? neuron.decay : neuron.decay *
					RandF(1.f - decayMut.pctAlpha, 1.f + decayMut.pctAlpha);
				neuron.decay = type != 2 ? neuron.decay : neuron.decay + RandF(-1, 1) * decayMut.linAlpha;
				neuron.decay = Clamp<float>(neuron.decay, 0, .9);
			}
		}
		if (RandF(0, 1) < mutations.newNodeChance && nnet.weightCount < nnet.createInfo.weightCapacity && nnet.weightCount > 0)
		{
			bool valid = AddNeuron(nnet, RandF(0, 1), RandF(0, 1), gId);
			if (valid)
			{
				const uint32_t weightId = rand() % nnet.weightCount;
				auto& weight = nnet.weights[weightId];
				weight.enabled = false;
				weight.next = UINT32_MAX;
				AddWeight(nnet, weight.from, nnet.neuronCount - 1, weight.value, gId);
				AddWeight(nnet, nnet.neuronCount - 1, weight.to, 1, gId);
			}
		}
		if (RandF(0, 1) < mutations.newWeightChance)
			// Minimize the impact this weight has on the network itself, making it mostly a topology based evolution.
			AddWeight(nnet, rand() % nnet.neuronCount, nnet.createInfo.inputSize + rand() % 
				(nnet.neuronCount - nnet.createInfo.inputSize), RandF(-.1, .1), gId);
	}

	void Copy(NNet& org, NNet& dst, Arena* arena)
	{
		if (arena)
		{
			dst.scope = arena->CreateScope();
			dst.createInfo = org.createInfo;
			dst.createInfo.neuronCapacity = org.neuronCount + 1;
			dst.createInfo.weightCapacity = org.weightCount + 3;
			dst.neurons = arena->New<Neuron>(dst.createInfo.neuronCapacity);
			dst.weights = arena->New<Weight>(org.createInfo.weightCapacity);
		}

		dst.neuronCount = org.neuronCount;
		dst.weightCount = org.weightCount;
		memcpy(dst.neurons, org.neurons, sizeof(Neuron) * org.neuronCount);
		memcpy(dst.weights, org.weights, sizeof(Weight) * org.weightCount);
	}
}