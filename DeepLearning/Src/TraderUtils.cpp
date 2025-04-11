#include "pch.h"
#include "TraderUtils.h"
#include <Jlib/ArrayUtils.h>

namespace jv
{
    float* TraderUtils::CreateMA(Arena& arena, const uint32_t start, 
        const uint32_t end, const uint32_t n, float* data)
    {
        const uint32_t l = start - end;
        auto maN = arena.New<float>(l);
        
        for (uint32_t i = 0; i < l; i++)
        {
            float& f = maN[i] = 0;
            for (uint32_t j = 0; j < n; j++)
                f += data[i + j];
            f /= n;
        }

        return maN;
    }

    float TraderUtils::GetCorrolation(float* a, float* b, const uint32_t n)
    {
        float aSum = 0, bSum = 0, abSum = 0;
        float aSqrtSum = 0, bSqrtSum = 0;
        for (uint32_t i = 0; i < n; i++)
        {
            aSum += a[i];
            bSum += b[i];
            abSum += a[i] * b[i];
            aSqrtSum += a[i] * a[i];
            bSqrtSum += b[i] * b[i];
        }

        float corr = (abSum * n - aSum * bSum) / sqrt((aSqrtSum * n - aSum * aSum) * (bSqrtSum * n - bSum * bSum));
        return corr;
    }

    float TraderUtils::GetVariance(float* a, const uint32_t n)
    {
        float avr = 0;
        for (uint32_t i = 0; i < n; i++)
            avr += a[i];
        avr /= n;

        float variance = 0;
        for (uint32_t i = 0; i < n; i++)
            variance += pow(a[i] - avr, 2);
        variance /= n;
        return variance;
    }
    float TraderUtils::GetStandardDeviation(float* a, const uint32_t n)
    {
        float sum = 0, mean, standardDeviation = 0;
        uint32_t i;

        for (i = 0; i < 10; ++i)
            sum += a[i];
        mean = sum / 10;

        for (i = 0; i < 10; ++i)
            standardDeviation += pow(a[i] - mean, 2);
        return sqrt(standardDeviation / 10);
    }
}

