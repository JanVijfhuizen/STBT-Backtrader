#include "pch.h"
#include "Traders/Modules/ModBounds.h"
#include <TraderUtils.h>

namespace jv
{
	bool ModBounds::Init(Arena& arena, const tmm::Info& info, const jv::bt::STBTScope& scope, Queue<bt::OutputMsg>& output)
	{
		start = info.start;
		end = info.end;

		const uint32_t count = scope.GetTimeSeriesCount();
		mas = arena.New<float*>(count);

		for (uint32_t i = 0; i < count; i++)
		{
			auto close = scope.GetTimeSeries(i).close;
			mas[i] = TraderUtils::CreateMA(arena, start, end, masLen, close);
		}

		return true;
	}
	bool ModBounds::Update(Arena& tempArena, const bt::STBTScope& scope, float* values, 
		Queue<bt::OutputMsg>& output, uint32_t current)
	{
		const uint32_t count = scope.GetTimeSeriesCount();
		const uint32_t index = start - current;

		for (uint32_t i = 0; i < count; i++)
		{
			auto series = scope.GetTimeSeries(i);
			const float point = series.close[current];
			
			const float ma = mas[i][index];
			const float std = TraderUtils::GetStandardDeviation(&series.close[current], stdLen);
			const float lower = ma - std * stdMul;

			// Get a dynamic, smooth value for the algorithm to use.
			// Percentage wise distance from center to lower, can be negative as well.
			const float r = RLerp(point, lower, ma);
			values[i] = r;
		}

		return true;
	}
	void ModBounds::Cleanup(Arena& arena, Queue<bt::OutputMsg>& output)
	{
	}
	uint32_t ModBounds::GetValuesLength(const bt::STBTScope& scope)
	{
		return scope.GetTimeSeriesCount();
	}
}