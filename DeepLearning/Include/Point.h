#pragma once
#include <cstdint>

struct Point final
{
	float open;
	float high;
	float low;
	float close;
	uint32_t volume;
};