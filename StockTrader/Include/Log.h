#pragma once

namespace jv::bt
{
	struct Log final
	{
		uint64_t scope;
		uint32_t length;
		uint32_t portLength;

		uint32_t** numsInPort;
		float* liquidities;
		float* portValues;
		float** stockCloses;
		float* marktAvr;
		float* marktPct;

		[[nodiscard]] static Log Create(Arena& arena, const STBTScope& scope, uint32_t start, uint32_t end);
		[[nodiscard]] static Log Create(Arena& arena, uint32_t length, uint32_t portLength);
		static void Destroy(Arena& arena, const Log& log);

		static void Save(const Log& log, const char* path);
	};
}