#pragma once
#include <Jlib/ArrayUtils.h>
#include <Jlib/Arena.h>

namespace jv
{
	template <typename T>
	struct KMeansInfo final
	{
		Arena* arena;
		Arena* tempArena;

		T* instances;
		uint32_t instanceCount;
		uint32_t pointCount;
		uint32_t cycles;

		float (*dist)(T& a, T& b);
		void (*add)(T& a, T& b);
		void (*div)(T& a, uint32_t n);
		void (*clear)(T& a);

		uint32_t* outCycleCount = nullptr;
	};

	template <typename T>
	[[nodiscard]] Array<uint32_t> ApplyKMeans(KMeansInfo<T> info);
	[[nodiscard]] Array<Array<uint32_t>> ConvKMeansRes(Arena& arena, Arena& tempArena, Array<uint32_t>& res, uint32_t pointCount);

	template<typename T>
	Array<uint32_t> ApplyKMeans(KMeansInfo<T> info)
	{
		auto& arena = *info.arena;
		auto& tempArena = *info.tempArena;

		auto arr = CreateArray<uint32_t>(arena, info.instanceCount);

		auto tempScope = tempArena.CreateScope();
		auto points = CreateArray<T>(tempArena, info.pointCount);
		for (uint32_t i = 0; i < info.pointCount; i++)
			points[i] = info.instances[rand() % info.instanceCount];

		uint32_t i = 0;
		for (; i < info.cycles; i++)
		{
			bool changed = false;

			// Assign instances to closest points.
			for (uint32_t j = 0; j < info.instanceCount; j++)
			{
				float minDis = FLT_MAX;
				uint32_t p = 0;

				for (uint32_t k = 0; k < info.pointCount; k++)
				{
					float dst = info.dist(info.instances[j], points[k]);
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
					info.clear(points[j]);

				auto tScope = tempArena.CreateScope();
				auto counts = CreateArray<uint32_t>(tempArena, info.pointCount);

				// Get average of point instances.
				for (uint32_t j = 0; j < info.instanceCount; j++)
				{
					const uint32_t ind = arr[j];
					++counts[ind];
					info.add(points[ind], info.instances[j]);
				}
				for (uint32_t k = 0; k < info.pointCount; k++)
					info.div(points[k], counts[k]);

				tempArena.DestroyScope(tScope);
			}	
		}

		tempArena.DestroyScope(tempScope);
		if (info.outCycleCount)
			*info.outCycleCount = i;
		return arr;
	}
}
