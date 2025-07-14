#pragma once

namespace jv
{
	struct CorrolationTrader final
	{
		jv::Arena* arena;
		jv::Arena* tempArena;

		uint64_t runScope;
		float* corrolations;

		[[nodiscard]] static CorrolationTrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] bt::STBTBot GetBot();
	};
}

