#pragma once

namespace jv
{
	// False Positive / False Negative Tester
	/* The problem with percentage based rating is that if you, for instance, have a set
	cats and dogs, and 98% of that set is just dogs, that an algorithm quickly figures out to "just always say dogs".
	This looks great because you can say that your test is 98% accurate but in reality it's just shit that doesn't do anything.
	So this test aims to negate that issue by punishing that behaviour. */ 
	struct FPFNTester final 
	{
		uint32_t falsePositives = 0;
		uint32_t falseNegatives = 0;
		uint32_t positives = 0;
		uint32_t negatives = 0;

		__declspec(dllexport) void AddResult(bool result, bool expectation);
		__declspec(dllexport) [[nodiscard]] float GetRating();
	};
}