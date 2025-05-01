#include "pch.h"
#include "Traders/MainTrader.h"
#include "TraderUtils.h"
#include "Jlib/VectorUtils.h"
#include <Traders/Modules/ModMA.h>

namespace jv
{
	struct GAResult final
	{
		uint32_t mas1Len;
		uint32_t mas2Len;
		float buyThresh;
		float sellThresh;

		[[nodiscard]] static GAResult Get(MainTrader& mt) 
		{
			GAResult result;

			auto instance = mt.training ? mt.ga.GetTrainee() : mt.ga.result;
			float* output = reinterpret_cast<float*>(instance);

			result.mas1Len = Clamp<int32_t>(round(output[0] * 30), 1, 30);
			result.mas2Len = Clamp<int32_t>(round(output[1] * 100), 1, 100);
			result.buyThresh = output[2];
			result.sellThresh = output[3];

			return result;
		}
	};

	void* MTCreate(jv::Arena& arena, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto arr = arena.New<float>(mt->width);
		for (uint32_t i = 0; i < mt->width; i++)
			arr[i] = jv::RandF(-1, 1);
		return arr;
	}

	void* MTCopy(jv::Arena& arena, void* instance, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto arr = arena.New<float>(mt->width);
		auto oArr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < mt->width; i++)
			arr[i] = oArr[i];

		return arr;
	}

	void MTMutate(jv::Arena& arena, void* instance, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto arr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < mt->width; i++)
		{
			if (jv::RandF(0, 1) > mt->mutateChance)
				continue;

			float& f = arr[i];

			const uint32_t type = rand() % 3;
			switch (type)
			{
				// Add/Sub
			case 0:
				f += jv::RandF(-mt->mutateAddition, mt->mutateAddition);
				break;
				// Mul/Div
			case 1:
				f *= 1.f + jv::RandF(-mt->mutateMultiplier, mt->mutateMultiplier);
				break;
				// New
			case 2:
				f = jv::RandF(-1, 1);
				break;
			}
		}
	}

	void* MTBreed(jv::Arena& arena, void* a, void* b, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);

		auto aArr = reinterpret_cast<float*>(a);
		auto bArr = reinterpret_cast<float*>(b);
		auto c = arena.New<float>(mt->width);

		for (uint32_t i = 0; i < mt->length; i++)
		{
			auto& f = c[i];
			f = rand() % 2 == 0 ? aArr[i] : bArr[i];
		}
		MTMutate(arena, c, userPtr);
		return c;
	}

	bool MainTraderInit(const jv::bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);

		mt->modMA = {};
		mt->manager = tmm::Manager::Create(*mt->arena, 1);
		mt->manager.Set(0, &mt->modMA);

		auto res = GAResult::Get(*mt);
		mt->modMA.mas1Len = res.mas1Len;
		mt->modMA.mas2Len = res.mas2Len;
		mt->modMA.buyThreshPct = res.buyThresh;
		mt->modMA.sellThreshPct = res.sellThresh;

		mt->startV = scope.GetPortValue(start);

		const uint32_t min = Max(mt->modMA.mas1Len, mt->modMA.mas2Len);
		if (buffer < min)
		{
			output.Add() = "ABORTED: Buffer too small.";
			return false;
		}

		tmm::Info info{};
		info.start = start;
		info.end = end;
		info.runIndex = runIndex;
		info.nRuns = nRuns;
		info.buffer = buffer;
		return mt->manager.Init(*mt->arena, info, scope, output);
	}

	bool MainTraderUpdate(const jv::bt::STBTScope& scope, jv::bt::STBTTrade* trades,
		const uint32_t current, void* userPtr, jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		mt->end = current;
		return mt->manager.Update(*mt->tempArena, scope, trades, output, current);
	}

	void MainTraderCleanup(const jv::bt::STBTScope& scope, void* userPtr, jv::Queue<const char*>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);

		const float diff = scope.GetPortValue(mt->end) - mt->startV;

		if (mt->training)
		{
			mt->rating += diff;
			if (++mt->currentInstanceRun >= mt->runsPerInstance)
			{
				mt->ga.debug = true;
				mt->ga.Rate(*mt->arena, *mt->tempArena, mt->rating);
				mt->rating = 0;
			}
		}

		tmm::Manager::Destroy(*mt->arena, mt->manager);
	}

	MainTrader MainTrader::Create(Arena& arena, Arena& tempArena)
	{
		MainTrader mt{};
		mt.arena = &arena;
		mt.tempArena = &tempArena;
		mt.scope = arena.CreateScope();
		return mt;
	}
	void MainTrader::Destroy(Arena& arena, MainTrader& trader)
	{
		arena.DestroyScope(trader.scope);
	}
	bt::STBTBot MainTrader::GetBot()
	{
		bt::STBTBot bot{};
		bot.name = "Main trader";
		bot.description = "First Big Attempt.";
		bot.author = "jannie";
		bot.init = MainTraderInit;
		bot.update = MainTraderUpdate;
		bot.cleanup = MainTraderCleanup;
		bot.userPtr = this;
		bot.bools = &training;
		bot.boolsNames = &boolsNames;
		bot.boolsLength = 1;
		return bot;
	}
	void MainTrader::InitGA()
	{
		jv::GeneticAlgorithmCreateInfo info{};
		info.length = 80;
		info.userPtr = this;
		info.breed = MTBreed;
		info.create = MTCreate;
		info.mutate = MTMutate;
		info.copy = MTCopy;
		ga = jv::GeneticAlgorithm::Create(*arena, info);

		currentInstanceRun = 0;
	}
}