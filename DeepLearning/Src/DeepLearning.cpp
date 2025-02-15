#include "pch.h"
#include <STBT.h>

int main()
{
	auto stbt = jv::bt::CreateSTBT();

	while (!stbt.Update())
		continue;
	return 0;
}
