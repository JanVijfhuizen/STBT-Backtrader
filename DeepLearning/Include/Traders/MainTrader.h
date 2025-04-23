#pragma once

namespace jv
{
	struct MainTrader final
	{
		Arena* arena;
		Arena* tempArena;

		uint64_t runScope;

		[[nodiscard]] static MainTrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}
