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
		float p = float(positives - falseNegatives) / float(positives + falsePositives);
		float n = float(negatives - falsePositives) / float(negatives + falseNegatives);

		// Pow2 to exponentially punish higher offsets.
		return (powf(p, 2) + powf(n, 2)) * .5;
	}
	void FPFNTester::Reset()
	{
		positives = 0;
		negatives = 0;
		falsePositives = 0;
		falseNegatives = 0;
	}
}