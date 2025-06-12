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
		gt.correctness = gt.tempArena->New<float>(info.start - info.end);
		gt.running = true;
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
			gt.correctness[info.start - info.current] = res == exp ? 1 : -1;
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
		gt.running = false;
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
		drawInfo.position -= glm::vec2(.5, 0);
		drawInfo.title = "Dominance";
		renderer.DrawDistributionGraph(drawInfo);

		if (ga->running)
		{
			drawInfo.color = glm::vec4(0, 0, 1, 1);
			drawInfo.values = ga->correctness;
			drawInfo.position = center + glm::vec2(0, .7);
			drawInfo.title = "Correctness";
			drawInfo.zoom *= 2.4;
			drawInfo.overrideCeiling = 4;
			renderer.DrawDistributionGraph(drawInfo);
		}
	}

	GATrader GATrader::Create(Arena& arena, Arena& tempArena)
	{
		GATrader trader{};
		GeneticAlgorithmCreateInfo info{};
		info.length = trader.length;
		info.userPtr = &trader;
		info.width = trader.width;
		info.length = trader.length;
		info.mutateChance = trader.mutateChance;
		info.mutateAddition = trader.mutateAddition;
		info.mutateMultiplier = trader.mutateMultiplier;
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.ga = GeneticAlgorithm::Create(arena, info);
		trader.running = false;
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