#pragma once
#include <Algorithms/GeneticAlgorithm.h>
#include <Algorithms/NNet.h>

namespace jv
{
	struct GATrader final
	{
		jv::Arena* arena;
		jv::Arena* tempArena;
		jv::GeneticAlgorithm ga;
		jv::nnet::Group group;

		// Train instance info.
		float startV;
		uint32_t start, end;

		uint64_t tempScope;
		float* ma30;
		uint32_t score;
		float* correctness;
		bool running;

		union
		{
			struct
			{
				const char* useSpeciationText;
				const char* useGroupText;
			};
			const char* boolTexts[2]
			{
				"Speciation",
				"Group algo"
			};
		};

		union
		{
			struct
			{
				bool useSpeciation;
				bool useGroup;
			};
			bool bools[2]
			{
				true,
				false
			};
		};

		// GE info.
		uint32_t width = 400;
		uint32_t length = 100;
		float mutateChance = .01f;
		float mutateAddition = 1;
		float mutateMultiplier = .1f;

		[[nodiscard]] static GATrader Create(Arena& arena, Arena& tempArena);
		[[nodiscard]] jv::bt::STBTBot GetBot();
	};
}
