#pragma once
#include "JLib/Arena.h"

namespace jv::bt
{
	template <typename T>
	struct Limit final
	{
		T min, max;
	};

	struct GeneticAlgorithm final
	{
		uint64_t scope;
		Limit<float>* fLimits;
		Limit<int32_t>* iLimits;
		uint32_t fLength;
		uint32_t iLength;
		float* fValues;
		int32_t* iValues;
		uint32_t length;
		float* ratings;

		// i = length will give the best performing instance.
		void Get(float* fValues, float* iValues, uint32_t i) const;
		void Rate(uint32_t i, float rating) const;
	};

	[[nodiscard]] GeneticAlgorithm CreateGeneticAlgorithm(Arena& arena, const Limit<float>* fLimits, 
		const Limit<int32_t>* iLimits, uint32_t fLength, uint32_t iLength, uint32_t length);
	void DestroyGeneticAlgorithm(Arena& arena, const GeneticAlgorithm& ga);
}
