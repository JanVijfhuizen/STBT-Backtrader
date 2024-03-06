#pragma once
#include "JLib/LinkedList.h"

struct DeepInstance final
{
	struct NeuronInfo final
	{
		uint32_t id;
		float threshold = 0;
		float decay = 1;
	};

	struct ConnectionInfo final
	{
		uint32_t id;
		float weight = 0;
	};

	jv::LinkedList<NeuronInfo> neurons;
	jv::LinkedList<ConnectionInfo> connections;
};
