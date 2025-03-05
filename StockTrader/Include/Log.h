#pragma once

namespace jv::bt
{
	struct Log final
	{
		uint64_t scope;
		uint32_t** numsInPort;
		float* liquidities;
		float* portValues;

		[[nodiscard]] static Log Create(Arena& arena, const STBTScope& scope, uint32_t start, uint32_t end);
		static void Destroy(Arena& arena, const Log& log);
	};
}