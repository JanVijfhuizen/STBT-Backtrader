#include "pch.h"
#include <STBT.h>

int main()
{
	jv::bt::STBTBot bots[2];
	bots[0].name = "empty test bot";
	bots[1].name = "Basic Trade Bot";
	bots[1].description = "Does some basic trading.";
	bots[1].author = "jv";

	auto stbt = jv::bt::CreateSTBT(bots, 2);

	while (!stbt.Update())
		continue;
	return 0;
}
