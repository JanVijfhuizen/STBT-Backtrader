#include "pch.h"
#include "Log.h"

namespace jv::bt
{
	Log Log::Create(Arena& arena, const STBTScope& scope, const uint32_t start, const uint32_t end)
	{
		const uint32_t length = end - start;
		const uint32_t timeSeriesCount = scope.GetTimeSeriesCount();

		Log log{};
		log.scope = arena.CreateScope();
		log.portValues = arena.New<float>(length);
		log.liquidities = arena.New<float>(length);
		log.numsInPort = arena.New<uint32_t*>(timeSeriesCount);
		for (uint32_t i = 0; i < timeSeriesCount; i++)
			log.numsInPort[i] = arena.New<uint32_t>(length);
		log.marktAvr = arena.New<float>(length);
		log.marktPct = arena.New<float>(length);
		return log;
	}
	void Log::Destroy(Arena& arena, const Log& log)
	{
		arena.DestroyScope(log.scope);
	}
}