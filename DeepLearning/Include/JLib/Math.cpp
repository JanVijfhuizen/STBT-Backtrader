#include "pch.h"
#include "Math.h"

float jv::RandF(const float min, const float max)
{
    float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    r = fmodf(r, max - min);
    return min + r;
}
