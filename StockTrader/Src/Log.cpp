#include "pch.h"
#include "Log.h"

namespace jv::bt
{
	Log Log::Create(Arena& arena, const STBTScope& scope, const uint32_t start, const uint32_t end)
	{
		const uint32_t length = end - start;
		const uint32_t timeSeriesCount = scope.GetTimeSeriesCount();
		return Create(arena, length, timeSeriesCount);
	}

	Log Log::Create(Arena& arena, const uint32_t length, const uint32_t portLength)
	{
		Log log{};
		log.scope = arena.CreateScope();
		log.portValues = arena.New<float>(length);
		log.liquidities = arena.New<float>(length);
		log.numsInPort = arena.New<uint32_t*>(portLength);
		for (uint32_t i = 0; i < portLength; i++)
			log.numsInPort[i] = arena.New<uint32_t>(length);
		log.stockCloses = arena.New<float*>(portLength);
		for (uint32_t i = 0; i < portLength; i++)
			log.stockCloses[i] = arena.New<float>(length);
		log.marktAvr = arena.New<float>(length);
		log.marktPct = arena.New<float>(length);
		log.length = length;
		log.portLength = portLength;
		return log;
	}

	void Log::Destroy(Arena& arena, const Log& log)
	{
		arena.DestroyScope(log.scope);
	}

	void Log::Save(const Log& log, const char* path)
	{
		std::ofstream fout(path);
		assert(fout.is_open());
		fout << log.length << std::endl;
		fout << log.portLength << std::endl;

		for (uint32_t i = 0; i < log.length; i++)
		{
			for (uint32_t j = 0; j < log.portLength; j++)
				fout << log.numsInPort[j][i] << std::endl;
			for (uint32_t j = 0; j < log.portLength; j++)
				fout << log.stockCloses[j][i] << std::endl;
			fout << log.liquidities[i] << std::endl;
			fout << log.portValues[i] << std::endl;
			fout << log.marktAvr[i] << std::endl;
			fout << log.marktPct[i] << std::endl;
		}

		fout.close();
	}
}