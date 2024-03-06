#pragma once
#include "JLib/LinkedList.h"

struct DeepInstance final
{
	struct NeuronInfo final
	{
		uint32_t id;
		float threshold;
		float decay;
	};

	struct ConnectionInfo final
	{
		uint32_t id;
		float weight = 1;
	};

	jv::LinkedList<NeuronInfo> neurons{};
	jv::LinkedList<ConnectionInfo> connections{};
};
