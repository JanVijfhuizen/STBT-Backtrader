#include "pch.h"
#include "Algorithms/KMeans.h"

namespace jv
{
	Array<Array<uint32_t>> jv::ConvKMeansRes(Arena& arena, Arena& tempArena, Array<uint32_t>& res, uint32_t pointCount)
	{
		auto arr = CreateArray<Array<uint32_t>>(arena, pointCount);

		auto tempScope = tempArena.CreateScope();
		auto lengths = CreateArray<uint32_t>(tempArena, pointCount);

		for (uint32_t i = 0; i < res.length; i++)
			++lengths[res[i]];
		for (uint32_t i = 0; i < arr.length; i++)
			arr[i] = CreateArray<uint32_t>(arena, lengths[i]);

		for (uint32_t i = 0; i < pointCount; i++)
			lengths[i] = 0;
		for (uint32_t i = 0; i < res.length; i++)
		{
			const uint32_t ind = res[i];
			arr[ind][lengths[ind]++] = i;
		}

		tempArena.DestroyScope(tempScope);
		return arr;
	}
}
