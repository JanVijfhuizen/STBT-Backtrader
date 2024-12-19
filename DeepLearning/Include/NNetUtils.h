#pragma once
#include "NNet.h"

namespace jv::ai 
{
	enum class InitType 
	{
		flat,
		random
	};

	void Init(NNet& nnet, InitType initType);
	void ConnectIOLayers(NNet& nnet, InitType initType);

	// copy (with or without original capacity)
	// clone
}

