#include "pch.h"
#include "Traders/GATrader.h"
#include <TraderUtils.h>
#include <Jlib/ArrayUtils.h>

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
		const uint32_t l = (info.start - info.end) * (gt.useGroup ? info.scope->GetTimeSeriesCount() : 1);
		gt.correctness = gt.tempArena->New<float>(l);
		gt.running = true;

		if (gt.useGroup)
		{
			if (info.training)
				gt.group.GetTrainee().Flush();
			else
				gt.group.result.Flush();
		}

		return true;
	}

	bool GATraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto& gt = *reinterpret_cast<GATrader*>(info.userPtr);

		if (gt.useGroup)
		{
			auto& algo = info.training ? gt.group.GetTrainee() : gt.group.result;
			const uint32_t l = info.scope->GetTimeSeriesCount();

			const auto tempScope = gt.tempArena->CreateScope();
			auto input = CreateArray<float>(*gt.tempArena, l);
			auto output = CreateArray<bool>(*gt.tempArena, l);

			if (info.current > 1)
			{
				for (uint32_t i = 0; i < l; i++)
				{
					auto series = info.scope->GetTimeSeries(i);
					input[i] = series.open[info.current - 2] / 1e3f;
				}
			
				algo.Propagate(*gt.tempArena, input, output);

				if (info.current > gt.nnetWarmupPeriod)
				{
					for (uint32_t i = 0; i < l; i++)
					{
						const bool res = output[i];
						auto series = info.scope->GetTimeSeries(i);
						bool exp = series.open[info.current - 1] < series.open[info.current - 2];
						info.fpfnTester->AddResult(res, exp);
						gt.score += res == exp;
						gt.correctness[(info.start - info.current) * l + i] = res == exp ? 1 : -1;
					}
				}
			}

			gt.tempArena->DestroyScope(tempScope);
		}
		else
		{
			float* algo = info.training ? gt.ga.GetTrainee() : gt.ga.result;

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
			if (gt.useGroup)
			{
				gt.group.Rate(*gt.arena, *gt.tempArena, gt.score, *info.output);
				if (gt.group.trainId == 0)
					info.progress->Add() = gt.group.genRating;
			}
			else
			{
				gt.ga.Rate(*gt.arena, *gt.tempArena, gt.score, *info.output);
				if (gt.ga.trainId == 0)
					info.progress->Add() = gt.ga.genRating;
			}	
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

		jv::nnet::GroupCreateInfo createInfo{};
		createInfo.inputCount = 20;
		createInfo.outputCount = 1;
		createInfo.length = 200;
		trader.group = jv::nnet::Group::Create(arena, tempArena, createInfo);
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
		bot.boolsNames = boolTexts;
		bot.boolsLength = sizeof(bools) / sizeof(bool);
		bot.bools = bools;
		return bot;
	}
}