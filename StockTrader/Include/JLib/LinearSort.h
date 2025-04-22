#pragma once

namespace jv
{
	// Linear sort.
	template <typename T>
	__declspec(dllexport) void LinearSort(T* arr, const size_t length, bool (*comparer)(T& a, T& b))
	{
		for (size_t i = 1; i < length; ++i)
		{
			size_t idx = i;
			while (idx > 0)
			{
				auto& current = arr[idx];
				auto& other = arr[idx - 1];

				if (!comparer(current, other))
					break;

				T temp = current;
				current = other;
				other = temp;

				--idx;
			}
		}
	}

	// Linear sort.
	template <typename T>
	__declspec(dllexport) void ApplyExtLinearSort(Arena& tempArena, T* arr, uint32_t* indexes, const size_t length)
	{
		auto tempScope = tempArena.CreateScope();
		auto tArr = tempArena.New<T>(length);

		for (size_t i = 0; i < length; ++i)
			tArr[i] = arr[indexes[i]];
		for (size_t i = 0; i < length; ++i)
			arr[i] = tArr[i];

		tempArena.DestroyScope(tempScope);
	}

	// Linear sort.
	template <typename T>
	__declspec(dllexport) void ExtLinearSort(T* arr, uint32_t* indexes, const size_t length, bool (*comparer)(T& a, T& b))
	{
		for (size_t i = 1; i < length; ++i)
		{
			size_t idx = i;
			while (idx > 0)
			{
				auto& current = arr[indexes[idx]];
				auto& other = arr[indexes[idx - 1]];

				if (!comparer(current, other))
					break;

				const auto temp = indexes[idx];
				indexes[idx] = indexes[idx - 1];
				indexes[idx - 1] = temp;

				--idx;
			}
		}
	}

	__declspec(dllexport) void CreateSortableIndices(uint32_t* arr, uint32_t length)
	{
		for (uint32_t i = 0; i < length; i++)
			arr[i] = i;
	}
}