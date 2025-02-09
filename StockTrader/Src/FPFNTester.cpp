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
		float p = falsePositives;
		p /= negatives;
		float n = falseNegatives;
		n /= positives;

		// Pow2 to exponentially punish higher offsets.
		return 1.f - (powf(p, 2) + powf(n, 2)) * .5;
	}
}