#include "pch.h"
#include "Traders/GATrader.h"
#include <TraderUtils.h>

namespace jv 
{
	bool GATraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output)
	{
		auto& gt = *reinterpret_cast<GATrader*>(userPtr);
		gt.tempScope = gt.tempArena->CreateScope();

		gt.startV = scope.GetPortValue(start);
		gt.start = start;
		gt.end = end;
		gt.ma30 = jv::TraderUtils::CreateMA(*gt.tempArena, start, end,
			jv::Min<uint32_t>(buffer, 30), scope.GetTimeSeries(0).close);
		return true;
	}

	bool GATraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		uint32_t current, void* userPtr, jv::Queue<const char*>& output)
	{
		auto& gt = *reinterpret_cast<GATrader*>(userPtr);
		float* algo = reinterpret_cast<float*>(gt.training ? gt.ga.GetTrainee() : gt.ga.result); //  gt.ga.generation[0] works

		float v;
		v = algo[current];
		trades[0].change = v > 0 ? 1e9 : -1e9;
		gt.end = current;
		return true;
	}

	void GATraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
	{
		auto& gt = *reinterpret_cast<GATrader*>(userPtr);
		const float diff = scope.GetPortValue(gt.end) - gt.startV;
		if (gt.training)
			gt.ga.Rate(*gt.arena, *gt.tempArena, diff);
		gt.tempArena->DestroyScope(gt.tempScope);
	}

	void* GACreate(jv::Arena& arena, void* userPtr)
	{
		auto ga = reinterpret_cast<GATrader*>(userPtr);
		auto arr = arena.New<float>(ga->width);
		for (uint32_t i = 0; i < ga->width; i++)
			arr[i] = jv::RandF(-1, 1);
		return arr;
	}

	void* GACopy(jv::Arena& arena, void* instance, void* userPtr)
	{
		auto ga = reinterpret_cast<GATrader*>(userPtr);
		auto arr = arena.New<float>(ga->width);
		auto oArr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < ga->width; i++)
			arr[i] = oArr[i];

		return arr;
	}

	void GAMutate(jv::Arena& arena, void* instance, void* userPtr)
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

	void* GABreed(jv::Arena& arena, void* a, void* b, void* userPtr)
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
		GAMutate(arena, c, userPtr);
		return c;
	}

	GATrader GATrader::Create(Arena& arena, Arena& tempArena)
	{
		jv::GATrader trader{};
		jv::GeneticAlgorithmCreateInfo info{};
		info.length = trader.length;
		info.userPtr = &trader;
		info.breed = jv::GABreed;
		info.create = jv::GACreate;
		info.mutate = jv::GAMutate;
		info.copy = jv::GACopy;
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.ga = jv::GeneticAlgorithm::Create(arena, info);
		return trader;
	}

	jv::bt::STBTBot GATrader::GetBot()
	{
		jv::bt::STBTBot bot;
		bot.name = "GA trader";
		bot.description = "Genetic Algorithm Trading.";
		bot.author = "jannie";
		bot.init = jv::GATraderInit;
		bot.update = jv::GATraderUpdate;
		bot.cleanup = jv::GATraderCleanup;
		bot.bools = &training;
		bot.boolsNames = &boolsNames;
		bot.boolsLength = 1;
		bot.userPtr = this;
		return bot;
	}
}