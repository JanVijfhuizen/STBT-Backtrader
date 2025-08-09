#include "pch.h"
#include "Traders/NNetTraderResult.h"

namespace jv
{
	NNetTraderResult NNetTraderResult::Create(Arena& arena, Arena& tempArena, const NNetTraderCreateInfo& info)
	{
		return NNetTraderResult();
	}
	void NNetTraderResult::Destroy(Arena& arena, NNetTraderResult& trader)
	{
	}
	jv::bt::STBTBot NNetTraderResult::GetBot()
	{
		return jv::bt::STBTBot();
	}
}