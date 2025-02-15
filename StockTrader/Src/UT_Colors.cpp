#include "pch.h"
#include "Utils/UT_Colors.h"
#include <Jlib/ArrayUtils.h>

namespace jv::bt
{
	void LoadRandColors(STBT& stbt)
	{
		/*
		const uint32_t l = stbt.timeSeriesArr.length;
		stbt.randColors = CreateArray<glm::vec4>(stbt.arena, l);

		glm::vec4 predetermined[5]
		{
			glm::vec4(1, 0, 0, 1),
			glm::vec4(0, 0, 1, 1),
			glm::vec4(0, 1, 1, 1),
			glm::vec4(1, 1, 0, 1),
			glm::vec4(1, 0, 1, 1)
		};

		for (uint32_t i = 0; i < l; i++)
		{
			if (i < (sizeof(predetermined) / sizeof(glm::vec4)))
				stbt.randColors[i] = predetermined[i];
			else
				stbt.randColors[i] = glm::vec4(RandF(0, 1), RandF(0, 1), RandF(0, 1), 1);
		}
		*/
	}
}