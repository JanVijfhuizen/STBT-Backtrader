#pragma once
#include <Traders/TMM.h>
#include <Traders/Modules/ModMA.h>

namespace jv
{
	struct MainTrader final
	{
		Arena* arena;
		Arena* tempArena;

		uint64_t scope;
		ModMA modMA;
		tmm::Manager manager;

		[[nodiscard]] static MainTrader Create(Arena& arena, Arena& tempArena);
		static void Destroy(Arena& arena, MainTrader& trader);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}
