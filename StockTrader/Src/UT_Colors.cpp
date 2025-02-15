#include "pch.h"
#include "Utils/UT_Colors.h"
#include <Jlib/ArrayUtils.h>

namespace jv::bt
{
	Array<glm::vec4> LoadRandColors(STBT& stbt, const uint32_t length)
	{
		auto randColors = CreateArray<glm::vec4>(stbt.arena, length);

		glm::vec4 predetermined[5]
		{
			glm::vec4(1, 0, 0, 1),
			glm::vec4(0, 0, 1, 1),
			glm::vec4(0, 1, 1, 1),
			glm::vec4(1, 1, 0, 1),
			glm::vec4(1, 0, 1, 1)
		};

		for (uint32_t i = 0; i < length; i++)
		{
			if (i < (sizeof(predetermined) / sizeof(glm::vec4)))
				randColors[i] = predetermined[i];
			else
				randColors[i] = glm::vec4(RandF(0, 1), RandF(0, 1), RandF(0, 1), 1);
		}

		return randColors;
	}
}