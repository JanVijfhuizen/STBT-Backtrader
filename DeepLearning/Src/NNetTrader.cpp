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
			mod.update(info, ptr->stockId, index, &input.ptr[l], mod.userPtr);
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
			for (auto& tester : ptr->testers)
				tester = {};
			ptr->rating = 0;
			nnet.ConstructParameters(current, nnet.GetCurrentParameters());
		}

		nnet.Flush(current);

		uint32_t warmup = 1;
		for (auto& mod : ptr->mods)
			if (mod.getMinBufferSize)
				warmup = Max(warmup, mod.getMinBufferSize(info, ptr->stockId, mod.userPtr));

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

		uint32_t cooldown = 0;
		for (const uint32_t timeFrame : ptr->timeFrames)
			cooldown = Max(cooldown, timeFrame);

		if (cooldown >= info.start - info.end)
		{
			std::string s = "Timeframe too large! Minimum run size required: ";
			s += std::to_string(cooldown);
			info.output->Add() = bt::OutputMsg::Create(s.c_str(), bt::OutputMsg::error);
			info.output->Add() = bt::OutputMsg::Create(
				"It's recommended to make the run size significantly larger than what is needed.");
			return false;
		}

		for (const auto& mod : ptr->mods)
			mod.init(info, ptr->stockId, info.buffer - warmup, mod.userPtr);
		
		auto output = CreateArray<float>(tempArena, 2 * ptr->timeFrames.length);
		for (uint32_t i = warmup; i < info.buffer; i++)
			Propagate(info, info.start + (info.buffer - i), output);
		DestroyArray(tempArena, output);

		return true;
	}

	bool NNTraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);

		// Ignore everything above the cooldown threshold.
		uint32_t cooldown = 0;
		for (const uint32_t timeFrame : ptr->timeFrames)
			cooldown = Max(cooldown, timeFrame);
		if (info.current <= info.end + cooldown)
			return true;

		auto& tempArena = *ptr->tempArena;
		const auto tempScope = tempArena.CreateScope();

		const uint32_t c = ptr->timeFrames.length;
		const auto ts = info.scope->GetTimeSeries(ptr->stockId);
		auto output = CreateArray<float>(tempArena, 2 * c);

		Propagate(info, info.current, output);

		for (uint32_t i = 0; i < c; i++)
		{
			const bool o1 = static_cast<bool>(output[i * 2]);
			const bool o2 = static_cast<bool>(output[i * 2 + 1]);
			const bool validOutput = o1 != o2;

			auto& tester = ptr->testers[i];

			if (!validOutput)
			{
				tester.AddResult(true, false);
				tester.AddResult(false, true);
				continue;
			}

			const uint32_t timeFrame = ptr->timeFrames[i];
			const bool wanted = ts.close[info.current + timeFrame] > ts.close[info.current + 1];

			ptr->rating += wanted == o1;
			ptr->rating += !wanted == o2;
			
			tester.AddResult(o1, wanted);
			tester.AddResult(!o2, wanted);
		}

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
			float r = 0;
			for (auto& tester : ptr->testers)
				r += tester.GetRating();

			// Not ideal that it can only show a single one atm.
			if (r > nnet.rating)
				*info.fpfnTester = ptr->testers[0];

			nnet.RateParameters(arena, tempArena, r);
			ptr->currentBatch = 0;
			ptr->genRating = Max(ptr->genRating, ptr->rating);
			++ptr->currentEpoch;

			// If a new generation was just created.
			if (nnet.ga.trainId == 0)
			{
				const uint32_t genId = nnet.ga.genId;
				if (nnet.rating > ptr->cycleHighestRating)
				{
					ptr->cycleHighestRating = nnet.rating;
					ptr->lastCycleWithProgress = genId;
				}
				// If the network hasn't had a better score for a while already, stop the training for this instance.
				else if ((ptr->lastCycleWithProgress + ptr->maxCyclesWithoutProgress) <= genId)
					ptr->currentEpoch = maxEpochs;
			}
		}

		if (ptr->currentEpoch == maxEpochs)
		{
			nnet.DestroyParameters(arena);
			nnet.Deconstruct(arena, nnet.GetCurrent());
			nnet.Rate(arena, tempArena);

			ptr->cycleHighestRating = 0;
			ptr->currentEpoch = 0;
			ptr->lastCycleWithProgress = 0;
			
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

		NNetTraderCreateInfo nnetInfo{};
		nnetInfo.mods = Copy(*tempArena, ptr->mods);
		nnetInfo.timeFrames = Copy(*tempArena, ptr->timeFrames);

		arena->DestroyScope(ptr->scope);
		*ptr = NNetTrader::Create(*arena, *tempArena, nnetInfo);

		tempArena->DestroyScope(tempScope);
	}

	void NNTraderRender(const bt::STBTBotInfo& info, gr::RenderProxy renderer, glm::vec2 center)
	{
	}

	NNetTrader NNetTrader::Create(Arena& arena, Arena& tempArena, const NNetTraderCreateInfo& info)
	{
		NNetTrader trader{};
		trader.arena = &arena;
		trader.tempArena = &tempArena;
		trader.scope = arena.CreateScope();

		trader.mods = Copy(arena, info.mods);
		trader.timeFrames = Copy(arena, info.timeFrames);
		trader.testers = CreateArray<jv::FPFNTester>(arena, info.timeFrames.length);

		uint32_t outputCount = 0;
		for (auto& mod : trader.mods)
			outputCount += mod.outputCount;

		jv::ai::DynNNetCreateInfo dynInfo{};
		dynInfo.inputCount = outputCount;
		dynInfo.outputCount = 2 * info.timeFrames.length;
		dynInfo.generationSize = 80;
		auto& nnet = trader.nnet = jv::ai::DynNNet::Create(arena, tempArena, dynInfo);
		nnet.alpha = 10;
		nnet.kmPointCount = 3;
		nnet.gaLength = 60;
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

	void ModInit(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t warmup, void* userPtr)
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);
		auto modPtr = reinterpret_cast<NNetTraderDefaultMod*>(userPtr);
		const auto ts = info.scope->GetTimeSeries(ptr->stockId);
		modPtr->multiplier = 1.f / ts.close[info.start + warmup];
	}
	void ModUpdate(const bt::STBTBotInfo& info, const uint32_t stockId, const uint32_t current, float* out, void* userPtr) 
	{
		auto ptr = reinterpret_cast<NNetTrader*>(info.userPtr);
		auto modPtr = reinterpret_cast<NNetTraderDefaultMod*>(userPtr);
		const auto ts = info.scope->GetTimeSeries(ptr->stockId);
		const float mul = modPtr->multiplier;

		out[0] = ts.open[current + 1] * mul;
		out[1] = ts.close[current + 1] * mul;
		out[2] = ts.high[current + 1] * mul;
		out[3] = ts.low[current + 1] * mul;
		out[4] = ts.dates[current + 1].day / 30;
		out[5] = ts.dates[current + 1].month / 12;
	}

	NNetTraderMod NNetGetDefaultMod(NNetTraderDefaultMod& out)
	{
		NNetTraderMod mod{};
		mod.outputCount = 6;
		mod.init = ModInit;
		mod.update = ModUpdate;
		mod.userPtr = &out;
		out = {};
		return mod;
	}
}