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

	[[nodiscard]] std::string ConvToFilePath(const char* file)
	{
		return "Bots/" + std::string(file) + ".bot";
	}

	void* MTCreate(Arena& arena, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto arr = arena.New<float>(mt->width);
		for (uint32_t i = 0; i < mt->width; i++)
			arr[i] = RandF(-1, 1);
		return arr;
	}

	void* MTCopy(Arena& arena, void* instance, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto arr = arena.New<float>(mt->width);
		auto oArr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < mt->width; i++)
			arr[i] = oArr[i];

		return arr;
	}

	void MTMutate(Arena& arena, void* instance, void* userPtr)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		auto arr = reinterpret_cast<float*>(instance);

		for (uint32_t i = 0; i < mt->width; i++)
		{
			if (RandF(0, 1) > mt->mutateChance)
				continue;

			float& f = arr[i];

			const uint32_t type = rand() % 3;
			switch (type)
			{
				// Add/Sub
			case 0:
				f += RandF(-mt->mutateAddition, mt->mutateAddition);
				break;
				// Mul/Div
			case 1:
				f *= 1.f + RandF(-mt->mutateMultiplier, mt->mutateMultiplier);
				break;
				// New
			case 2:
				f = RandF(-1, 1);
				break;
			}
		}
	}

	void* MTBreed(Arena& arena, void* a, void* b, void* userPtr)
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

	bool MainTraderInit(const bt::STBTScope& scope, void* userPtr,
		const uint32_t start, const uint32_t end,
		const uint32_t runIndex, const uint32_t nRuns, const uint32_t buffer,
		Queue<bt::OutputMsg>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		mt->isFinalRun = runIndex == nRuns - 1;

		// Replace GA result if there is a savefile that it can be loaded from.
		if(!mt->training)
			if (!std::string(mt->loadFile).empty())
			{
				const auto path = ConvToFilePath(mt->loadFile);
				std::ifstream fin(path);
				if (fin.is_open())
				{
					float* result = reinterpret_cast<float*>(mt->ga.result);
					std::string line;
					for (uint32_t i = 0; i < mt->width; i++)
					{
						std::getline(fin, line);
						result[i] = std::stof(line);
					}
				}
			}

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
			output.Add() = bt::OutputMsg::Create("Buffer too small.", bt::OutputMsg::error);
			auto c = "Minimum size needed: " + std::to_string(min);
			output.Add() = bt::OutputMsg::Create(c.c_str());
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

	bool MainTraderUpdate(const bt::STBTScope& scope, bt::STBTTrade* trades,
		const uint32_t current, void* userPtr, Queue<bt::OutputMsg>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);
		mt->end = current;
		return mt->manager.Update(*mt->tempArena, scope, trades, output, current);
	}

	void MainTraderCleanup(const bt::STBTScope& scope, void* userPtr, Queue<bt::OutputMsg>& output)
	{
		auto mt = reinterpret_cast<MainTrader*>(userPtr);

		const float diff = scope.GetPortValue(mt->end) - mt->startV;

		if (mt->training)
		{
			mt->rating += diff;
			if (++mt->currentInstanceRun >= mt->runsPerInstance)
			{
				mt->ga.debug = true;
				mt->ga.Rate(*mt->arena, *mt->tempArena, mt->rating, output);	
				mt->rating = 0;
			}
		}

		// Save result of training to a file, so that it can be used later.
		if (mt->isFinalRun && mt->training)
		{
			if (!std::string(mt->saveFile).empty())
			{
				const auto path = ConvToFilePath(mt->saveFile);
				std::ofstream fout(path);
				assert(fout.is_open());

				float* result = reinterpret_cast<float*>(mt->ga.result);
				for (uint32_t i = 0; i < mt->width; i++)
					fout << result[i] << std::endl;

				fout.close();
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
		buffers[0] = loadFile;
		buffers[1] = saveFile;

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
		bot.buffers = buffers;
		bot.bufferNames = bufferNames;
		bot.buffersLength = 2;
		bot.bufferSizes = bufferSizes;
		return bot;
	}
	void MainTrader::InitGA()
	{
		GeneticAlgorithmCreateInfo info{};
		info.length = 80;
		info.userPtr = this;
		info.breed = MTBreed;
		info.create = MTCreate;
		info.mutate = MTMutate;
		info.copy = MTCopy;
		ga = GeneticAlgorithm::Create(*arena, info);

		currentInstanceRun = 0;
	}
}