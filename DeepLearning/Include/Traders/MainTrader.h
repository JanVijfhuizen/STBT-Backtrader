#pragma once

namespace jv
{
	struct MainTrader final
	{
		Arena* arena;
		Arena* tempArena;

		[[nodiscard]] static MainTrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}
