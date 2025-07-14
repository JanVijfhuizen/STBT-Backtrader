#include "pch.h"
#include "Traders/NNetTrader.h"
#include <Jlib/ArrayUtils.h>

namespace jv
{
	void Propagate(const bt::STBTBotInfo& info, const uint32_t index, Array<float>& output)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);
		auto& tempArena = *ptr->tempArena;
		const auto tempScope = tempArena.CreateScope();

		const auto ts = info.scope->GetTimeSeries(ptr->stockId);
		auto input = CreateArray<float>(tempArena, 1);

		input[0] = ts.open[index + 1];
		ptr->nnet.Propagate(tempArena, input, output);
		tempArena.DestroyScope(tempScope);
	}

	bool NNTraderInit(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		auto& arena = *ptr->arena;
		auto& tempArena = *ptr->tempArena;
		auto& nnet = ptr->nnet;
		
		ptr->runScope = arena.CreateScope();
		ptr->stockId = rand() % info.scope->GetTimeSeriesCount();

		auto current = nnet.GetCurrent();

		if (ptr->currentEpoch == 0)
		{
			nnet.Construct(arena, tempArena, current);
			nnet.CreateParameters(arena);
		}
		
		nnet.ConstructParameters(current, nnet.GetCurrentParameters());
		nnet.Flush(current);

		// Warmup period.
		float o[2]{};
		Array<float> output{};
		output.length = 2;
		output.ptr = o;
		for (uint32_t i = 0; i < info.buffer; i++)
			Propagate(info, info.start - (info.buffer - i), output);

		ptr->tester = {};
		ptr->rating = 0;
		return true;
	}

	bool NNTraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		auto& tempArena = *ptr->tempArena;
		const auto tempScope = tempArena.CreateScope();

		auto output = CreateArray<float>(tempArena, 2);
		Propagate(info, info.current, output);

		const bool o1 = static_cast<bool>(output[0]);
		const bool o2 = static_cast<bool>(output[1]);
		const bool validOutput = o1 != o2;

		ptr->rating += validOutput;
		ptr->tester.AddResult(!o1, o2);

		tempArena.DestroyScope(tempScope);
		return true;
	}

	void NNTraderCleanup(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		auto& arena = *ptr->arena;
		auto& tempArena = *ptr->tempArena;
		auto& nnet = ptr->nnet;

		nnet.RateParameters(arena, tempArena, ptr->tester.GetRating());
		ptr->genRating = Max(ptr->genRating, ptr->rating);

		if (++ptr->currentEpoch == ptr->epochs * nnet.ga.info.length)
		{
			nnet.DestroyParameters(arena);
			nnet.Deconstruct(arena, nnet.GetCurrent());
			nnet.Rate(arena, tempArena);
			ptr->currentEpoch = 0;
			
			if (nnet.currentId == 0)
			{
				std::string s = "Generation ";
				s += std::to_string(nnet.generationId);
				s += ": ";
				s += std::to_string(ptr->genRating);
				info.output->Add() = bt::OutputMsg::Create(s.c_str());
				info.progress->Add() = nnet.rating;
				ptr->genRating = 0;
			}
		}

		arena.DestroyScope(ptr->runScope);
	}

	void NNTraderRender(const bt::STBTBotInfo& info, gr::RenderProxy renderer, glm::vec2 center)
	{
	}

	NNetTrader NNetTrader::Create(Arena& arena, Arena& tempArena)
	{
		NNetTrader trader{};
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.scope = arena.CreateScope();

		jv::ai::DynNNetCreateInfo info{};
		info.inputCount = 1;
		info.outputCount = 2;
		info.generationSize = 50;
		auto& nnet = trader.nnet = jv::ai::DynNNet::Create(arena, tempArena, info);
		nnet.alpha = 10;
		nnet.kmPointCount = 3;
		nnet.gaLength = 40;
		nnet.gaKmPointCount = 3;

		return trader;
	}
	void NNetTrader::Destroy(Arena& arena, NNetTrader& trader)
	{
		arena.DestroyScope(trader.scope);
	}
	jv::bt::STBTBot NNetTrader::GetBot()
	{
		bt::STBTBot bot{};
		bot.name = "NNet Trader";
		bot.author = "jannie";
		bot.description = "uses evolving topologies.";
		bot.init = NNTraderInit;
		bot.update = NNTraderUpdate;
		bot.cleanup = NNTraderCleanup;
		bot.customRender = NNTraderRender;
		bot.userPtr = this;
		return bot;
	}
}