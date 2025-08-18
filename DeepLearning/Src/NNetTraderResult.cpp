#include "pch.h"
#include "Traders/NNetTraderResult.h"
#include <Jlib/ArrayUtils.h>

namespace jv
{
	void Propagate(const bt::STBTBotInfo& info, const uint32_t stockId, const uint32_t index, Array<float>& output)
	{
		auto ptr = reinterpret_cast<NNetTraderResult*>(info.userPtr);
		auto& tempArena = *ptr->tempArena;
		const auto tempScope = tempArena.CreateScope();

		uint32_t l = 0;
		for (const auto& mod : ptr->mods)
			l += mod.outputCount;

		auto input = CreateArray<float>(tempArena, l);

		l = 0;
		for (const auto& mod : ptr->mods)
		{
			mod.update(info, stockId, index, &input.ptr[l], mod.userPtr);
			l += mod.outputCount;
		}

		ptr->nnet.Propagate(tempArena, input, output);
		tempArena.DestroyScope(tempScope);
	}

	bool NNRTraderInit(const bt::STBTBotInfo& info)
	{
		return true;
	}

	bool NNRTraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTraderResult*>(info.userPtr);
		const auto tempScope = info.tempArena->CreateScope();

		const uint32_t tsCount = info.scope->GetTimeSeriesCount();
		auto results = CreateArray<float>(*info.tempArena, tsCount * 2);

		for (uint32_t i = 0; i < tsCount; i++)
		{
			const auto ts = info.scope->GetTimeSeries(i);

			float outputV[2]{ 0, 0 };
			Array<float> output{};
			output.ptr = outputV;
			output.length = 2;

			uint32_t warmup = 1;
			for (auto& mod : ptr->mods)
				if (mod.getMinBufferSize)
					warmup = Max(warmup, mod.getMinBufferSize(info, i, mod.userPtr));

			if (warmup > info.buffer)
			{
				std::string s = "Buffer too small! Minimum size required: ";
				s += std::to_string(warmup);
				info.output->Add() = bt::OutputMsg::Create(s.c_str(), bt::OutputMsg::error);
				info.output->Add() = bt::OutputMsg::Create(
					"It's recommended to make the buffer larger than what is needed.");
				info.output->Add() = bt::OutputMsg::Create(
					"Any excess from the buffer is used as a warmup period, which stabilizes the algorithm.");
				return false;
			}

			// Warmup.
			ptr->nnet.Flush(ptr->nnet.result);
			for (const auto& mod : ptr->mods)
				mod.init(info, i, info.buffer - warmup, mod.userPtr);
			for (uint32_t j = warmup; j < info.buffer; j++)
				Propagate(info, i, info.current + (info.buffer - j), output);

			for (auto& f : output)
				f = 0;
			Propagate(info, i, info.current, output);
			for (uint32_t j = 0; j < 2; j++)
				results[i * 2 + j] = output[j];
		}

		float remaining = info.scope->GetLiquidity();
		uint32_t rounds = 0;
		while (remaining > 0)
		{
			for (uint32_t i = 0; i < tsCount; i++)
			{
				const auto c = info.scope->GetTimeSeries(i).close[info.current];
				remaining -= c;
			}
			rounds++;
		}

		for (uint32_t i = 0; i < tsCount; i++)
		{
			const float buy = results[i * 2];
			const float sell = results[i * 2 + 1];
			const bool result = buy > sell;
			info.trades[i].change = result ? rounds : -10000;
		}

		info.tempArena->DestroyScope(tempScope);
		return true;
	}

	NNetTraderResult NNetTraderResult::Create(Arena& arena, Arena& tempArena, const NNetTraderCreateInfo& info)
	{
		NNetTraderResult trader{};
		trader.scope = arena.CreateScope();
		trader.mods = Copy(arena, info.mods);
		trader.tempArena = &tempArena;

		uint32_t outputCount = 0;
		for (auto& mod : trader.mods)
			outputCount += mod.outputCount;

		jv::ai::DynNNetCreateInfo dynInfo{};
		dynInfo.inputCount = outputCount;
		dynInfo.outputCount = 2;
		dynInfo.generationSize = NNET_GEN_SIZE;

		auto& nnet = trader.nnet = jv::ai::DynNNet::Create(arena, tempArena, dynInfo);
		nnet.Load(info.loadFile, arena);
		nnet.Construct(arena, tempArena, nnet.result);
		nnet.CreateParameters(arena);

		return trader;
	}
	void NNetTraderResult::Destroy(Arena& arena, NNetTraderResult& trader)
	{
		arena.DestroyScope(trader.scope);
	}
	jv::bt::STBTBot NNetTraderResult::GetBot()
	{
		bt::STBTBot bot{};
		bot.name = "NNet Result";
		bot.author = "jannie";
		bot.description = "uses evolving topologies.";
		bot.init = NNRTraderInit;
		bot.update = NNRTraderUpdate;
		bot.userPtr = this;
		return bot;
	}
}