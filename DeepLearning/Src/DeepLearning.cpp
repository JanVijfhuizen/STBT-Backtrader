#include "pch.h"
#include <STBT.h>

bool SimpleTradeBot(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades, uint32_t current, void* userPtr)
{
	for (uint32_t i = 0; i < 4; i++)
	{
		trades[i].change = rand() % 3;
		trades[i].change -= 1;
	}

	return true;
}

int main()
{
	bool bools[2];
	const char* boolNames[2]
	{
		"finishOnValleyReached",
		"testing bool"
	};

	char buffer[16]{};
	char* buffers[1];
	buffers[0] = buffer;
	const char* bufferName = "some buffer";
	uint32_t bufferSize = 16;

	jv::bt::STBTBot bots[3];
	bots[0].name = "empty test bot";
	bots[2].name = "other bot";
	bots[1].name = "Basic Trade Bot";
	bots[1].description = "Does some basic trading.";
	bots[1].author = "jv";
	bots[1].update = SimpleTradeBot;
	bots[1].bools = bools;
	bots[1].boolsNames = boolNames;
	bots[1].boolsLength = 2;
	bots[1].buffers = buffers;
	bots[1].buffersNames = &bufferName;
	bots[1].bufferSizes = &bufferSize;
	bots[1].buffersLength = 1;

	auto stbt = jv::bt::CreateSTBT(bots, 3);

	while (!stbt.Update())
		continue;
	return 0;
}
