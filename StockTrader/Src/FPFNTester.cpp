#include "pch.h"
#include "JLib/FPFNTester.h"

namespace jv
{
	void FPFNTester::AddResult(const bool result, const bool expectation)
	{
		falsePositives += (result != expectation) && result;
		falseNegatives += (result != expectation) && !result;
		positives += expectation;
		negatives += !expectation;
	}
	float FPFNTester::GetRating()
	{
		// Will crash out if positives + fPositives == 0.
		float p = 1.f - float(positives - falseNegatives) / positives;
		float n = 1.f - float(negatives - falsePositives) / negatives;

		// Pow2 to exponentially punish higher offsets.
		// Multiply with eachother to make sure it doesn't try to optimize one side only.
		// Reverse to make the first few steps much more impactful than later optimization.
		const float r = (1.f - pow(p, 2)) * (1.f - pow(n, 2));
		return r;
	}
	void FPFNTester::Reset()
	{
		positives = 0;
		negatives = 0;
		falsePositives = 0;
		falseNegatives = 0;
	}
}