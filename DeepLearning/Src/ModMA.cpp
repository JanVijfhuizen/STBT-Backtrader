#include "pch.h"
#include <Traders/Modules/ModMA.h>
#include <TraderUtils.h>

namespace jv
{
	bool ModMA::Init(Arena& arena, const tmm::Info& info, const bt::STBTScope& scope, Queue<bt::OutputMsg>& output)
	{
		start = info.start;
		end = info.end;

		const uint32_t count = scope.GetTimeSeriesCount();
		mas1 = arena.New<float*>(count);
		mas2 = arena.New<float*>(count);

		for (uint32_t i = 0; i < count; i++)
		{
			auto close = scope.GetTimeSeries(i).close;
			mas1[i] = TraderUtils::CreateMA(arena, start, end, mas1Len, close);
			mas2[i] = TraderUtils::CreateMA(arena, start, end, mas2Len, close);
		}

		return true;
	}

	bool ModMA::Update(Arena& tempArena, const bt::STBTScope& scope, 
		float* values, Queue<bt::OutputMsg>& output, const uint32_t current)
	{
		auto tempScope = tempArena.CreateScope();

		const uint32_t count = scope.GetTimeSeriesCount();
		auto buys = CreateVector<uint32_t>(tempArena, count);
		auto sells = CreateVector<uint32_t>(tempArena, count);

		const uint32_t index = start - current;

		for (uint32_t i = 0; i < count; i++)
		{
			const float pct = mas2[i][index] / mas1[i][index];
			if (pct > (1.f + buyThreshPct))
				buys.Add() = i;
			else if (pct < (1.f - sellThreshPct))
				sells.Add() = i;
		}

		for (auto& buy : buys)
			values[buy] = 1;
		for (auto& sell : sells)
			values[sell] = -1;
		
		tempArena.DestroyScope(tempScope);
		return true;
	}

	void ModMA::Cleanup(Arena& arena, Queue<bt::OutputMsg>& output)
	{
	}
	uint32_t ModMA::GetValuesLength(const bt::STBTScope& scope)
	{
		return scope.GetTimeSeriesCount();
	}
}

