#pragma once

namespace jv
{
	template <typename T>
	[[nodiscard]] T Max(const T& a, const T& b)
	{
		return a > b ? a : b;
	}

	template <typename T>
	[[nodiscard]] T Min(const T& a, const T& b)
	{
		return a > b ? b : a;
	}

	template <typename T>
	[[nodiscard]] T Clamp(const T& t, const T& min, const T& max)
	{
		return Min(max, Max(t, min));
	}

	[[nodiscard]] float RandF(float min, float max);

	template <typename T>
	[[nodiscard]] T RLerp(const T& t, const T& min, const T& max)
	{
		const T bounds = max - min;
		const T org = t - min;
		return org / bounds;
	}
}
