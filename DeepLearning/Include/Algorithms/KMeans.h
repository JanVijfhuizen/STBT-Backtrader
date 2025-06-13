#pragma once
#include <Jlib/ArrayUtils.h>
#include <Jlib/Arena.h>

namespace jv
{
	struct KMeansInfo final
	{
		Arena* arena;
		Arena* tempArena;

		float** instances;
		uint32_t count;
		uint32_t width;
		uint32_t pointCount;
		uint32_t cycles;

		uint32_t* outCycleCount = nullptr;
	};

	[[nodiscard]] Array<uint32_t> ApplyKMeans(KMeansInfo info);
	[[nodiscard]] Array<Array<uint32_t>> ConvKMeansRes(Arena& arena, Arena& tempArena, 
		Array<uint32_t>& res, uint32_t pointCount);
}
