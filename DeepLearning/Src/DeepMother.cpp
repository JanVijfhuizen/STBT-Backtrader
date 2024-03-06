#include "pch.h"
#include "DeepMother.h"

#include "DeepInstance.h"
#include "JLib/Arena.h"
#include "JLib/LinkedList.h"
#include "JLib/LinkedListUtils.h"
#include "JLib/Math.h"
#include "JLib/VectorUtils.h"

void RandomizeNeuron(DeepInstance::NeuronInfo& neuron)
{
	neuron.threshold = static_cast<float>(rand()) / (RAND_MAX / 1.f);
	neuron.decay = static_cast<float>(rand()) / (RAND_MAX / 1.f);
}

uint32_t DeepMother::AddNeuron(jv::Arena& arena, const DeepMotherMetaData* metaData, DeepInstance& instance, const bool fromConnection)
{
	_neurons.Add();
	const uint32_t nId = _neurons.count - 1;
	auto& iNeuron = Add(arena, instance.neurons);
	iNeuron.id = nId;
	RandomizeNeuron(iNeuron);

	if(fromConnection && metaData->connectionIds.length > 0)
	{
		const uint32_t rId = rand() % metaData->connectionIds.length;
		const auto& conn = _connections[metaData->connectionIds[rId]];

		auto& conn1 = _connections.Add();
		conn1.from = conn.from;
		conn1.to = nId;
		conn1.weight = conn.weight;
		auto& conn2 = _connections.Add();
		conn2.from = nId;
		conn2.to = conn.to;
		conn2.weight = conn.weight;

		instance.connections[rId].id = _connections.length - 2;
		Add(arena, instance.connections).id = _connections.length - 1;
	}

	return nId;
}

DeepMother DeepMother::Create(jv::Arena& arena, const DeepMotherCreateInfo& info)
{
	DeepMother mother{};
	mother._neurons = jv::CreateVector<Neuron>(arena, info.neuronCapacity);
	mother._connections = jv::CreateVector<Connection>(arena, info.connectionCapacity);
	return mother;
}

void DeepMother::UpdateInputValue(const uint32_t id, const float value, const float delta) const
{
	_neurons[id].value += value * delta;
}

float DeepMother::ReadValue(const uint32_t id) const
{
	return _neurons[id].value;
}

uint32_t DeepMother::AddNode(jv::Arena& arena, DeepInstance& instance)
{
	return AddNeuron(arena, nullptr, instance, false);
}

void DeepMother::Mutate(jv::Arena& arena, const DeepMotherMetaData& metaData, DeepInstance& instance)
{
	// Randomly adjust neuron values.
	for (auto& neuron : instance.neurons)
	{
		if (rand() % thresholdMutationChance == 0)
		{
			const float mutation = static_cast<float>(rand()) / (RAND_MAX / 2.f) - 1;
			neuron.threshold += mutation;
		}
		if (rand() % decayMutationChance == 0)
		{
			const float mutation = static_cast<float>(rand()) / (RAND_MAX / 2.f) - 1;
			neuron.decay += mutation;
		}
	}
	// Randomly adjust weight values.
	for (auto& connection : instance.connections)
	{
		if (rand() % weightMutationChance != 0)
			continue;
		
		const float mutation = static_cast<float>(rand()) / (RAND_MAX / 2.f) - 1;
		connection.weight += mutation;
	}
	// Randomly spawn new neurons.
	if(rand() % neuronSpawnChance == 0)
	{
		_neurons.Add();
		auto& nInfo = Add(arena, instance.neurons);
		nInfo.id = _neurons.count - 1;
		RandomizeNeuron(nInfo);
	}
	// Randomly spawn new connections.
	if (rand() % connectionSpawnChance == 0)
	{
		auto& conn = _connections.Add();
		const uint32_t from = rand() % metaData.neuronIds.length;
		uint32_t to = from;
		while(to == from)
			to = rand() % metaData.neuronIds.length;
		conn.from = metaData.neuronIds[from];
		conn.to = metaData.neuronIds[to];

		auto& cInfo = Add(arena, instance.connections);
		cInfo.id = _connections.count - 1;
	}
}

DeepMotherMetaData DeepMother::Apply(jv::Arena& arena, const DeepInstance& instance) const
{
	DeepMotherMetaData metaData{};
	const auto nIds = jv::CreateArray<uint32_t>(arena, instance.neurons.GetCount());
	const auto cIds = jv::CreateArray<uint32_t>(arena, instance.connections.GetCount());

	uint32_t i = 0;
	for (const auto& neuron : instance.neurons)
	{
		auto& mNeuron = _neurons[neuron.id];
		mNeuron.connIds = {};
		mNeuron.threshold = neuron.threshold;
		mNeuron.decay = neuron.decay;
		nIds[i++] = neuron.id;
	}

	uint32_t j = 0;
	for (const auto& connection : instance.connections)
	{
		auto& mConnection = _connections[connection.id];
		mConnection.weight = connection.weight;
		auto& mNeuron = _neurons[mConnection.from];
		Add(arena, mNeuron.connIds) = mConnection.to;
		nIds[j++] = connection.id;
	}

	metaData.neuronIds = nIds;
	metaData.connectionIds = cIds;
	return metaData;
}

void DeepMother::Update(const DeepMotherMetaData& metaData, const float delta) const
{
	for (const auto& id : metaData.neuronIds)
	{
		auto& neuron = _neurons[id];
		neuron.value = jv::Min<float>(neuron.value, 1);
		for (const auto& connId : neuron.connIds)
		{
			const auto& connection = _connections[connId];
			auto& cNeuron = _neurons[connection.to];
			cNeuron.value += connection.weight;
		}
		neuron.value -= neuron.decay * delta;
		neuron.value = jv::Max<float>(neuron.value, 0);
	}
}
