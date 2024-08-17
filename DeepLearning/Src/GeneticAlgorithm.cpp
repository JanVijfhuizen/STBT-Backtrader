#include "pch.h"
#include "GeneticAlgorithm.h"

namespace jv::bt
{
	void GeneticAlgorithm::Get(float* fValues, float* iValues, const uint32_t i) const
	{
		memcpy(fValues, &this->fValues[i * sizeof(float) * fLength], sizeof(float) * fLength);
		memcpy(iValues, &this->iValues[i * sizeof(int32_t) * iLength], sizeof(int32_t) * iLength);
	}

	void GeneticAlgorithm::Rate(const uint32_t i, const float rating) const
	{
		ratings[i] = rating;
	}

	GeneticAlgorithm CreateGeneticAlgorithm(Arena& arena, 
		const Limit<float>* fLimits, const Limit<int32_t>* iLimits,
		const uint32_t fLength, const uint32_t iLength, const uint32_t length)
	{
		GeneticAlgorithm ga{};
		ga.scope = arena.CreateScope();
		ga.fLength = fLength;
		ga.iLength = iLength;
		ga.fLimits = arena.New<Limit<float>>(fLength);
		ga.iLimits = arena.New<Limit<int32_t>>(iLength);
		memcpy(ga.fLimits, fLimits, sizeof(Limit<float>) * fLength);
		memcpy(ga.iLimits, iLimits, sizeof(Limit<int32_t>) * fLength);
		ga.fValues = arena.New<float>(fLength * (length + 1));
		ga.iValues = arena.New<int32_t>(iLength * (length + 1));
		ga.ratings = arena.New<float>(length + 1);
		return ga;
	}

	void DestroyGeneticAlgorithm(Arena& arena, const GeneticAlgorithm& ga)
	{
		arena.DestroyScope(ga.scope);
	}
}
