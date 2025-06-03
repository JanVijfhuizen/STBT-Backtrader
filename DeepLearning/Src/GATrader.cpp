#include "pch.h"
#include "Traders/GATrader.h"
#include <TraderUtils.h>

namespace jv 
{
	bool GATraderInit(const bt::STBTBotInfo& info)
	{
		auto& gt = *reinterpret_cast<GATrader*>(info.userPtr);
		gt.tempScope = gt.tempArena->CreateScope();

		gt.startV = info.scope->GetPortValue(info.start);
		gt.start = info.start;
		gt.end = info.end;
		gt.ma30 = TraderUtils::CreateMA(*gt.tempArena, info.start, info.end,
			Min<uint32_t>(info.buffer, 30), info.scope->GetTimeSeries(0).close);
		gt.score = 0;

		return true;
	}

	bool GATraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto& gt = *reinterpret_cast<GATrader*>(info.userPtr);
		float* algo = reinterpret_cast<float*>(info.training ? gt.ga.GetTrainee() : gt.ga.result); //  gt.ga.generation[0] works

		float v;
		v = algo[info.current];
		const int32_t change = v > 0 ? 1e9 : -1e9;
		info.trades[0].change = change;

		if (info.current > 1)
		{
			auto series = info.scope->GetTimeSeries(0);
			bool res = change > 0;
			bool exp = series.open[info.current - 1] < series.open[info.current - 2];
			info.fpfnTester->AddResult(res, exp);
			gt.score += res == exp;
		}

		gt.end = info.current;
		return true;
	}

	void GATraderCleanup(const bt::STBTBotInfo& info)
	{
		auto& gt = *reinterpret_cast<GATrader*>(info.userPtr);
		//const float diff = info.scope->GetPortValue(gt.end) - gt.startV;

		if (info.training)
		{
			//gt.ga.Rate(*gt.arena, *gt.tempArena, diff, *info.output);
			gt.ga.Rate(*gt.arena, *gt.tempArena, gt.score, *info.output);
			if (gt.ga.trainId == 0)
				info.progress->Add() = gt.ga.genRating;
		}
			
		gt.tempArena->DestroyScope(gt.tempScope);
	}

	void GATraderRender(const bt::STBTBotInfo& info, gr::RenderProxy renderer, glm::vec2 center)
	{
		auto ga = reinterpret_cast<GATrader*>(info.userPtr);
		float* algo = reinterpret_cast<float*>(ga->ga.result);

		gr::DrawDistributionGraphInfo drawInfo{};
		drawInfo.aspectRatio = renderer.GetAspectRatio();
		drawInfo.position = center + glm::vec2(.25, -.35);
		drawInfo.values = algo;
		drawInfo.length = ga->width;
		drawInfo.title = "Genes";
		drawInfo.scale = glm::vec2(.6);
		drawInfo.zoom = 3;
		renderer.DrawDistributionGraph(drawInfo);

		drawInfo.color = glm::vec4(0, 1, 0, 1);
		drawInfo.values = &algo[ga->width];
		drawInfo.noBackground = true;
		drawInfo.position -= glm::vec2(.5, 0);
		drawInfo.title = "Dominance";
		renderer.DrawDistributionGraph(drawInfo);
	}

	void* GACreate(Arena& arena, void* userPtr)
	{
		auto ga = reinterpret_cast<GATrader*>(userPtr);
		// Adding dominance variable.
		const uint32_t w = ga->width * 2;
		auto arr = arena.New<float>(w);
		for (uint32_t i = 0; i < ga->width; i++)
			arr[i] = RandF(-1, 1);
		for (uint32_t i = 0; i < ga->width; i++)
			arr[ga->width + i] = RandF(0, 1);
		return arr;
	}

	void* GACopy(Arena& arena, void* instance, void* userPtr)
	{
		auto ga = reinterpret_cast<GATrader*>(userPtr);
		auto arr = arena.New<float>(ga->width * 2);
		auto oArr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < ga->width * 2; i++)
			arr[i] = oArr[i];

		return arr;
	}

	void GAMutate(Arena& arena, void* instance, void* userPtr)
	{
		auto ga = reinterpret_cast<GATrader*>(userPtr);
		auto arr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < ga->width; i++)
		{
			if (RandF(0, 1) > ga->mutateChance)
				continue;

			float& f = arr[i];

			const uint32_t type = rand() % 3;
			switch (type)
			{
				// Add/Sub
			case 0:
				f += RandF(-ga->mutateAddition, ga->mutateAddition);
				break;
				// Mul/Div
			case 1:
				f *= 1.f + RandF(-ga->mutateMultiplier, ga->mutateMultiplier);
				break;
				// New
			case 2:
				f = RandF(-1, 1);
				break;
			}
		}
	}

	void* GABreed(Arena& arena, void* a, void* b, void* userPtr)
	{
		auto ga = reinterpret_cast<GATrader*>(userPtr);

		auto aArr = reinterpret_cast<float*>(a);
		auto bArr = reinterpret_cast<float*>(b);
		auto c = arena.New<float>(ga->width * 2);

		for (uint32_t i = 0; i < ga->width; i++)
		{
			auto& f = c[i];
			const float aDom = aArr[ga->width + i];
			const float bDom = bArr[ga->width + i];
			const bool choice = RandF(0, aDom + bDom) < aDom;

			// Apply gene based on random dominance factor.
			f = choice ? aArr[i] : bArr[i];
			c[ga->width + i] = choice ? aDom : bDom;
		}
		GAMutate(arena, c, userPtr);
		return c;
	}

	GATrader GATrader::Create(Arena& arena, Arena& tempArena)
	{
		GATrader trader{};
		GeneticAlgorithmCreateInfo info{};
		info.length = trader.length;
		info.userPtr = &trader;
		info.breed = GABreed;
		info.create = GACreate;
		info.mutate = GAMutate;
		info.copy = GACopy;
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.ga = GeneticAlgorithm::Create(arena, info);
		return trader;
	}

	bt::STBTBot GATrader::GetBot()
	{
		bt::STBTBot bot;
		bot.name = "GA trader";
		bot.description = "Genetic Algorithm Trading.";
		bot.author = "jannie";
		bot.init = GATraderInit;
		bot.update = GATraderUpdate;
		bot.cleanup = GATraderCleanup;
		bot.customRender = GATraderRender;
		bot.userPtr = this;
		return bot;
	}
}