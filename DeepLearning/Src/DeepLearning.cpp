#include "pch.h"
#include <STBT.h>

void SimpleTradeBot(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades, uint32_t current, void* userPtr)
{
	for (uint32_t i = 0; i < 4; i++)
	{
		trades[i].change = rand() % 3;
		trades[i].change -= 1;
	}
}

int main()
{
	jv::bt::STBTBot bots[2];
	bots[0].name = "empty test bot";
	bots[1].name = "Basic Trade Bot";
	bots[1].description = "Does some basic trading.";
	bots[1].author = "jv";
	bots[1].update = SimpleTradeBot;

	auto stbt = jv::bt::CreateSTBT(bots, 2);

	while (!stbt.Update())
		continue;
	return 0;
}
