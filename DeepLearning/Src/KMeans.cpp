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

	float KMDist(float* a, float* b, const uint32_t width)
	{
		float ret = 0;
		for (uint32_t i = 0; i < width; i++)
			ret += abs(a[i] - b[i]);
		return ret;
	}

	void KMAdd(float* a, float* b, const uint32_t width)
	{
		for (uint32_t i = 0; i < width; i++)
			a[i] += b[i];
	}

	void KMDiv(float* a, uint32_t n, const uint32_t width)
	{
		for (uint32_t i = 0; i < width; i++)
			a[i] /= n;
	}

	void KMClear(float* a, const uint32_t width)
	{
		for (uint32_t i = 0; i < width; i++)
			a[i] = 0;
	}

	void KMSet(float* a, float* b, const uint32_t width)
	{
		for (uint32_t i = 0; i < width; i++)
			a[i] = b[i];
	}

	Array<uint32_t> ApplyKMeans(KMeansInfo info)
	{
		const uint32_t width = info.width;

		auto& arena = *info.arena;
		auto& tempArena = *info.tempArena;

		auto arr = CreateArray<uint32_t>(arena, info.count);

		auto tempScope = tempArena.CreateScope();
		auto points = CreateArray<float*>(tempArena, info.pointCount);
		for (uint32_t i = 0; i < info.pointCount; i++)
		{
			points[i] = tempArena.New<float>(width);
			const uint32_t rInd = (rand() % info.count);
			KMSet(points[i], info.instances[rInd], width);
		}		

		uint32_t i = 0;
		for (; i < info.cycles; i++)
		{
			bool changed = false;

			// Assign instances to closest points.
			for (uint32_t j = 0; j < info.count; j++)
			{
				float minDis = FLT_MAX;
				uint32_t p = 0;

				for (uint32_t k = 0; k < info.pointCount; k++)
				{
					float dst = KMDist(info.instances[j], points[k], width);
					if (dst < minDis)
					{
						minDis = dst;
						p = k;
					}
				}

				// Assign point to instance.
				changed = changed ? true : arr[j] != p;
				arr[j] = p;
			}

			if (!changed)
				break;

			// Set points to the average of their positions.
			if (i < info.cycles - 1)
			{
				// Reset the points.
				for (uint32_t j = 0; j < info.pointCount; j++)
					KMClear(points[j], width);

				auto tScope = tempArena.CreateScope();
				auto counts = CreateArray<uint32_t>(tempArena, info.pointCount);

				// Get average of point instances.
				for (uint32_t j = 0; j < info.count; j++)
				{
					const uint32_t ind = arr[j];
					++counts[ind];
					KMAdd(points[ind], info.instances[j], width);
				}
				for (uint32_t k = 0; k < info.pointCount; k++)
					KMDiv(points[k], counts[k], width);

				tempArena.DestroyScope(tScope);
			}
		}

		tempArena.DestroyScope(tempScope);
		if (info.outCycleCount)
			*info.outCycleCount = i;
		return arr;
	}
}
