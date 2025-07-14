#include "pch.h"
#include "JLib/LinearSort.h"

namespace jv
{
	void CreateSortableIndices(uint32_t* arr, uint32_t length)
	{
		for (uint32_t i = 0; i < length; i++)
			arr[i] = i;
	}
}