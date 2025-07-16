#pragma once
#include "Array.h"

namespace jv
{
	template <typename T>
	Array<T> CreateArray(Arena& arena, const uint32_t length)
	{
		Array<T> array{};
		array.ptr = arena.New<T>(length);
		array.length = length;
		return array;
	}

	template <typename T>
	void DestroyArray(Arena& arena, const Array<T>& array)
	{
		arena.Free(array.ptr);
	}

	template <typename T>
	[[nodiscard]] T& Rand(const Array<T>& array)
	{
		return array[rand() % array.length];
	}

	template <typename T>
	[[nodiscard]] Array<T> Copy(Arena& arena, const Array<T>& array)
	{
		auto ret = CreateArray<T>(arena, array.length);
		memcpy(ret.ptr, array.ptr, sizeof(T) * array.length);
		return ret;
	}
}
