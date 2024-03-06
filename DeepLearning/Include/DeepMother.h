#pragma once
#include "Connection.h"
#include "Neuron.h"
#include "JLib/Array.h"

struct DeepInstance;

namespace jv
{
	struct Arena;
}

struct DeepMother final
{
	jv::Array<Neuron> neurons;
	jv::Array<Connection> connections;

	void Apply(jv::Arena& arena, const DeepInstance& instance, jv::Array<uint32_t>* outIds, uint64_t* outScope) const;
	void Update(const jv::Array<uint32_t>& ids, const float dt) const;
};
