#pragma once
#include "Connection.h"
#include "DeepInstance.h"
#include "Neuron.h"
#include "JLib/Array.h"
#include "JLib/Vector.h"

struct DeepInstance;

namespace jv
{
	struct Arena;
}

struct DeepMotherCreateInfo final
{
	uint32_t neuronCapacity = 2048;
	uint32_t connectionCapacity = 4096;
};

struct DeepMotherMetaData final
{
	jv::Array<uint32_t> neuronIds;
	jv::Array<uint32_t> connectionIds;
};

struct DeepMother final
{
	uint32_t neuronSpawnChance = 100;
	uint32_t connectionSpawnChance = 100;
	uint32_t thresholdMutationChance = 10;
	uint32_t decayMutationChance = 10;
	uint32_t weightMutationChance = 10;

	[[nodiscard]] static DeepMother Create(jv::Arena& arena, const DeepMotherCreateInfo& info);
	void InputValue(uint32_t id, float value, float delta) const;
	[[nodiscard]] float ReadValue(uint32_t id) const;
	uint32_t AddNode(jv::Arena& arena, DeepInstance& instance);
	void Mutate(jv::Arena& arena, const DeepMotherMetaData& metaData, DeepInstance& instance);
	[[nodiscard]] DeepMotherMetaData Apply(jv::Arena& arena, const DeepInstance& instance) const;
	void Update(const DeepMotherMetaData& metaData, float delta) const;

private:
	jv::Vector<Neuron> _neurons;
	jv::Vector<Connection> _connections;

	uint32_t AddNeuron(jv::Arena& arena, const DeepMotherMetaData* metaData, DeepInstance& instance, bool fromConnection);
};
