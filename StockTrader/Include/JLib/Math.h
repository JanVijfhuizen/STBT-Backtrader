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
}
