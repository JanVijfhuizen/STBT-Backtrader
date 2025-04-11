#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>

void* MAlloc(const uint32_t size)
{
	return malloc(size);
}

void MFree(void* ptr)
{
	return free(ptr);
}

struct GATrader final
{
	jv::Arena* arena;
	jv::Arena* tempArena;
	jv::GeneticAlgorithm ga;
	bool training = true;

	// Train instance info.
	float startV;
	uint32_t endId;

	// GE info.
	uint32_t width = 30;
	uint32_t length = 20;
	float mutateChance = .2f;
	float mutateAddition = 1;
	float mutateMultiplier = .1f;
};

bool GATraderInit(const jv::bt::STBTScope& scope, void* userPtr, uint32_t runIndex, 
	uint32_t runLength, jv::Queue<const char*>& output)
{
	auto& gt = *reinterpret_cast<GATrader*>(userPtr);
	gt.startV = scope.GetPortValue(runIndex);
	return true;
}

bool GATraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades, 
	uint32_t current, void* userPtr, jv::Queue<const char*>& output)
{
	auto& gt = *reinterpret_cast<GATrader*>(userPtr);
	float* algo = reinterpret_cast<float*>(gt.training ? gt.ga.GetTrainee() : gt.ga.result);

	float v = algo[current];
	trades[0].change = v > 0 ? 1e9 : -1e9;
	gt.endId = current;

	return true;
}

void GATraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
{
	auto& gt = *reinterpret_cast<GATrader*>(userPtr);
	if (gt.training)
		gt.ga.Rate(*gt.arena, *gt.tempArena, scope.GetPortValue(gt.endId) - gt.startV);
}

void* Create(jv::Arena& arena, void* userPtr)
{
	auto ga = reinterpret_cast<GATrader*>(userPtr);
	auto arr = arena.New<float>(ga->width);
	for (uint32_t i = 0; i < ga->width; i++)
		arr[i] = jv::RandF(-1, 1);
	return arr;
}

void* Copy(jv::Arena& arena, void* instance, void* userPtr)
{
	auto ga = reinterpret_cast<GATrader*>(userPtr);
	auto arr = arena.New<float>(ga->width);
	auto oArr = reinterpret_cast<float*>(instance);

	for (uint32_t i = 0; i < ga->width; i++)
		arr[i] = oArr[i];

	return arr;
}

void Mutate(jv::Arena& arena, void* instance, void* userPtr)
{
	auto ga = reinterpret_cast<GATrader*>(userPtr);
	auto arr = reinterpret_cast<float*>(instance);

	for (uint32_t i = 0; i < ga->width; i++)
	{
		if (jv::RandF(0, 1) > ga->mutateChance)
			continue;

		float& f = arr[i];

		const uint32_t type = rand() % 3;
		switch (type)
		{
			// Add/Sub
		case 0:
			f += jv::RandF(-ga->mutateAddition, ga->mutateAddition);
			break;
			// Mul/Div
		case 1:
			f *= 1.f + jv::RandF(-ga->mutateMultiplier, ga->mutateMultiplier);
			break;
			// New
		case 2:
			f = jv::RandF(-1, 1);
			break;
		}
	}
}

void* Breed(jv::Arena& arena, void* a, void* b, void* userPtr)
{
	auto ga = reinterpret_cast<GATrader*>(userPtr);

	auto aArr = reinterpret_cast<float*>(a);
	auto bArr = reinterpret_cast<float*>(b);
	auto c = arena.New<float>(ga->width);

	for (uint32_t i = 0; i < ga->length; i++)
	{
		auto& f = c[i];
		f = rand() % 2 == 0 ? aArr[i] : bArr[i];
	}
	Mutate(arena, c, userPtr);
	return c;
}

int main()
{
	jv::Arena arena, tempArena;
	{
		jv::ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		arena = jv::Arena::Create(arenaCreateInfo);
		tempArena = jv::Arena::Create(arenaCreateInfo);
	}

	GATrader trader{};
	{
		jv::GeneticAlgorithmCreateInfo info{};
		info.length = trader.length;
		info.userPtr = &trader;
		info.breed = Breed;
		info.create = Create;
		info.mutate = Mutate;
		info.copy = Copy;
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.ga = jv::GeneticAlgorithm::Create(arena, info);
	}

	const char* bName = "Training";
	jv::bt::STBTBot bot;
	bot.name = "GA trader";
	bot.description = "Genetic Algorithm Trading.";
	bot.author = "jannie";
	bot.init = GATraderInit;
	bot.update = GATraderUpdate;
	bot.cleanup = GATraderCleanup;
	bot.bools = &trader.training;
	bot.boolsNames = &bName;
	bot.boolsLength = 1;
	bot.userPtr = &trader;

	auto stbt = jv::bt::CreateSTBT(&bot, 1);
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
