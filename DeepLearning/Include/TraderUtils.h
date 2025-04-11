#pragma once

namespace jv
{
	struct TraderUtils final
	{
		[[nodiscard]] static Array<float> CreateMA(Arena& arena, 
			uint32_t start, uint32_t end, uint32_t n, float* data);
	};
}