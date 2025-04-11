#pragma once

namespace jv
{
	struct TraderUtils final
	{
		[[nodiscard]] static Array<float> CreateMA(Arena& arena, 
			uint32_t start, uint32_t end, uint32_t n, float* data);
		[[nodiscard]] static float GetCorrolation(float* a, float* b, uint32_t n);
		[[nodiscard]] static float GetVariance(float* a, uint32_t n);
	};
}