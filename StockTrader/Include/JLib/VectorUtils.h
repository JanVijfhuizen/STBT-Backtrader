#pragma once
#include "Vector.h"

namespace jv
{
	template <typename T>
	[[nodiscard]] Vector<T> CreateVector(Arena& arena, const uint32_t length)
	{
		Vector<T> vector{};
		vector.ptr = arena.New<T>(length);
		vector.length = length;
		return vector;
	}

	template <typename T>
	void DestroyVector(Arena& arena, const Vector<T>& vector)
	{
		arena.Free(vector.ptr);
	}

	template <typename T>
	[[nodiscard]] uint32_t Contains(const Vector<T>& vector, const T& t)
	{
		for (uint32_t i = 0; i < vector.count; i++)
			if (vector[i] == t)
				return i;
		return -1;
	}
}
