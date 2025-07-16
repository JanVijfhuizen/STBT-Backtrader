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

		uint32_t l = 0;
		for (const auto& mod : ptr->mods)
			l += mod.outputCount;

		auto input = CreateArray<float>(tempArena, l);

		l = 0;
		for (const auto& mod : ptr->mods)
		{
			mod.update(info, ptr->stockId, index, &input.ptr[l]);
			l += mod.outputCount;
		}

		ptr->nnet.Propagate(tempArena, input, output);
		tempArena.DestroyScope(tempScope);
	}

	bool NNTraderInit(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		auto& arena = *ptr->arena;
		auto& tempArena = *ptr->tempArena;
		auto& nnet = ptr->nnet;

		// Pick a random stock.
		ptr->stockId = rand() % info.scope->GetTimeSeriesCount();

		auto current = nnet.GetCurrent();

		if (ptr->currentEpoch == 0)
		{
			nnet.Construct(arena, tempArena, current);
			nnet.CreateParameters(arena);
		}

		if (ptr->currentBatch == 0)
		{
			ptr->tester = {};
			ptr->rating = 0;
			nnet.ConstructParameters(current, nnet.GetCurrentParameters());
		}

		nnet.Flush(current);

		for (const auto& mod : ptr->mods)
			mod.init(info, ptr->stockId);

		// Warmup period.
		float o[2]{};
		Array<float> output{};
		output.length = 2;
		output.ptr = o;
		for (uint32_t i = 1; i < info.buffer; i++)
			Propagate(info, info.start + (info.buffer - i), output);

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

		ptr->tester.AddResult(!o1, o2);

		const auto ts = info.scope->GetTimeSeries(ptr->stockId);
		const bool wanted = ts.open[info.current] > ts.open[info.current + 1];

		if (validOutput)
		{
			ptr->rating += wanted == o1;
			ptr->rating += !wanted == o2;
		}
		
		ptr->tester.AddResult(o1, wanted);
		ptr->tester.AddResult(!o2, wanted);

		tempArena.DestroyScope(tempScope);
		return true;
	}

	void NNTraderCleanup(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		auto& arena = *ptr->arena;
		auto& tempArena = *ptr->tempArena;
		auto& nnet = ptr->nnet;

		const float chunk = 1.f / nnet.generation.length;
		const uint32_t maxEpochs = ptr->epochs * nnet.ga.info.length;

		const float epochsPct = (static_cast<float>(ptr->currentEpoch) / maxEpochs) * chunk;
		*info.progressPct = chunk * nnet.currentId + epochsPct;

		if (++ptr->currentBatch == ptr->batchSize)
		{
			nnet.RateParameters(arena, tempArena, ptr->tester.GetRating());
			ptr->currentBatch = 0;
			ptr->genRating = Max(ptr->genRating, ptr->rating);
			++ptr->currentEpoch;
		}

		if (ptr->currentEpoch == maxEpochs)
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
				s += " / ";
				s += std::to_string(nnet.rating);
				info.output->Add() = bt::OutputMsg::Create(s.c_str());

				s = "n: ";
				s += std::to_string(nnet.result.neurons.length);
				s += " w: ";
				s += std::to_string(nnet.result.weights.length);
				s += " gn: ";
				s += std::to_string(nnet.neurons.count);
				s += " gw: ";
				s += std::to_string(nnet.weights.count);
				auto msg = bt::OutputMsg::Create(s.c_str());
				msg.color = glm::vec4(.8, .8, .8, 1);
				info.output->Add() = msg;

				info.progress->Add() = nnet.generationRating;
				ptr->genRating = 0;
			}
		}
	}

	void NNTraderReset(const bt::STBTBotInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		auto arena = ptr->arena;
		auto tempArena = ptr->tempArena;

		const auto tempScope = tempArena->CreateScope();
		auto cpy = Copy(*tempArena, ptr->mods);

		arena->DestroyScope(ptr->scope);
		*ptr = NNetTrader::Create(*arena, *tempArena, cpy);

		tempArena->DestroyScope(tempScope);
	}

	void NNTraderRender(const bt::STBTBotInfo& info, gr::RenderProxy renderer, glm::vec2 center)
	{
	}

	NNetTrader NNetTrader::Create(Arena& arena, Arena& tempArena, const Array<NNetTraderMod>& mods)
	{
		NNetTrader trader{};
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.scope = arena.CreateScope();
		trader.mods = Copy(arena, mods);

		uint32_t outputCount = 0;
		for (auto& mod : trader.mods)
			outputCount += mod.outputCount;

		jv::ai::DynNNetCreateInfo info{};
		info.inputCount = outputCount;
		info.outputCount = 2;
		info.generationSize = 60;
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
		bot.reset = NNTraderReset;
		bot.userPtr = this;
		return bot;
	}

	void ModInit(const bt::STBTBotInfo& info, uint32_t stockId)
	{

	}
	void ModUpdate(const bt::STBTBotInfo& info, const uint32_t stockId, const uint32_t current, float* out) 
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);
		const auto ts = info.scope->GetTimeSeries(ptr->stockId);

		out[0] = ts.open[current + 1];
		out[1] = ts.close[current + 1];
		out[2] = ts.high[current + 1];
		out[3] = ts.low[current + 1];
		out[4] = ts.dates[current + 1].day;
		out[5] = ts.dates[current + 1].month;
	}

	NNetTraderMod NNetGetDefaultMod()
	{
		NNetTraderMod mod{};
		mod.outputCount = 6;
		mod.init = ModInit;
		mod.update = ModUpdate;
		return mod;
	}
}