#include "pch.h"

#include "JLib/ArrayUtils.h"
#include "JLib/Math.h"
#include "JLib/Queue.h"
#include <Renderer.h>
#include <STBT.h>

int main()
{
	srand(time(nullptr));

	auto stui = jv::ai::CreateSTBT();

	while (!stui.Update())
		continue;
	return 0;
}
