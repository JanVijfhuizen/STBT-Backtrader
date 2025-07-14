#pragma once

namespace jv
{
	template <typename T>
	__declspec(dllexport) [[nodiscard]] T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	template <typename T>
	__declspec(dllexport) [[nodiscard]] T Min(const T& a, const T& b)
	{
		return a > b ? b : a;
	}

	template <typename T>
	__declspec(dllexport) [[nodiscard]] T Clamp(const T& t, const T& min, const T& max)
	{
		return Min(max, Max(t, min));
	}

	__declspec(dllexport) [[nodiscard]] float RandF(float min, float max);

	template <typename T>
	__declspec(dllexport) [[nodiscard]] T RLerp(const T& t, const T& min, const T& max)
	{
		const T bounds = max - min;
		const T org = t - min;
		return org / bounds;
	}

	template <typename T>
	__declspec(dllexport) [[nodiscard]] T Round(const T& t, const uint32_t decimals)
	{
		const T mul = 1 * pow(10, decimals);
		return floorf(t * mul) / mul;
	}

	// Source: https://stackoverflow.com/questions/10847007/using-the-gaussian-probability-density-function-in-c
	template <typename T>
	__declspec(dllexport) [[nodiscard]] T Gauss(T x, T mean, T stddev)
	{
		static const T inv_sqrt_2pi = 0.3989422804014327;
		T a = (x - mean) / stddev;
		return inv_sqrt_2pi / stddev * std::exp(-T(0.5) * stddev * stddev);
	}
}
