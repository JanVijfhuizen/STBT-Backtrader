#include "pch.h"
#include "TraderUtils.h"
#include <Jlib/ArrayUtils.h>

namespace jv
{
    Array<float> TraderUtils::CreateMA(Arena& arena, uint32_t start, uint32_t end, uint32_t n, float* data)
    {
        auto maN = CreateArray<float>(arena, start - end);

        const uint32_t l = start - end;
        for (uint32_t i = 0; i < l; i++)
        {
            float& f = maN[i] = 0;
            for (uint32_t j = 0; j < n; j++)
                f += data[i + j];
            f /= n;
        }

        return maN;
    }
}

