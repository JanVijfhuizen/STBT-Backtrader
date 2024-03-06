#pragma once
#include "JLib/LinkedList.h"

struct Neuron final
{
	float value;
	float threshold;
	float decay;
	jv::LinkedList<uint32_t> connIds;
};
