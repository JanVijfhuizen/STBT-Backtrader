#include "pch.h"
#include "DeepMother.h"

#include "DeepInstance.h"
#include "JLib/Arena.h"
#include "JLib/LinkedList.h"
#include "JLib/LinkedListUtils.h"
#include "JLib/Math.h"

void DeepMother::Apply(jv::Arena& arena, const DeepInstance& instance, jv::Array<uint32_t>* outIds,
                       uint64_t* outScope) const
{
	*outScope = arena.CreateScope();
	*outIds = jv::CreateArray<uint32_t>(arena, instance.neurons.GetCount());

	uint32_t i = 0;
	for (const auto& neuron : instance.neurons)
	{
		auto& mNeuron = neurons[neuron.id];
		mNeuron.connIds = {};
		mNeuron.threshold = neuron.threshold;
		mNeuron.decay = neuron.decay;
		(*outIds)[i++] = neuron.id;
	}

	for (const auto& connection : instance.connections)
	{
		auto& mConnection = connections[connection.id];
		mConnection.weight = connection.weight;
		auto& mNeuron = neurons[mConnection.from];
		Add(arena, mNeuron.connIds) = mConnection.to;
	}
}

void DeepMother::Update(const jv::Array<uint32_t>& ids, const float dt) const
{
	for (const auto& id : ids)
	{
		auto& neuron = neurons[id];
		neuron.value = jv::Min<float>(neuron.value, 1);
		for (const auto& connId : neuron.connIds)
		{
			const auto& connection = connections[connId];
			auto& cNeuron = neurons[connection.to];
			cNeuron.value += connection.weight;
		}
		neuron.value -= neuron.decay * dt;
		neuron.value = jv::Max<float>(neuron.value, 0);
	}
}
