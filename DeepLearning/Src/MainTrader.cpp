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

		[[nodiscard]] static GAResult Get(MainTrader& mt, const bool training) 
		{
			GAResult result;

			auto instance = training ? mt.ga.GetTrainee() : mt.ga.result;
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

	bool MainTraderInit(const bt::STBTBotInfo& info)
	{
		auto mt = reinterpret_cast<MainTrader*>(info.userPtr);
		mt->isFinalRun = info.runIndex == info.nRuns - 1;

		// Replace GA result if there is a savefile that it can be loaded from.
		if(!info.training)
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

		auto res = GAResult::Get(*mt, info.training);
		mt->modMA.mas1Len = res.mas1Len;
		mt->modMA.mas2Len = res.mas2Len;
		mt->modMA.buyThreshPct = res.buyThresh;
		mt->modMA.sellThreshPct = res.sellThresh;

		mt->startV = info.scope->GetPortValue(info.start);

		const uint32_t min = Max(mt->modMA.mas1Len, mt->modMA.mas2Len);
		if (info.buffer < min)
		{
			info.output->Add() = bt::OutputMsg::Create("Buffer too small.", bt::OutputMsg::error);
			auto c = "Minimum size needed: " + std::to_string(min);
			info.output->Add() = bt::OutputMsg::Create(c.c_str());
			return false;
		}

		tmm::Info tmmInfo{};
		tmmInfo.start = info.start;
		tmmInfo.end = info.end;
		tmmInfo.runIndex = info.runIndex;
		tmmInfo.nRuns = info.nRuns;
		tmmInfo.buffer = info.buffer;
		return mt->manager.Init(*mt->arena, tmmInfo, *info.scope, *info.output);
	}

	bool MainTraderUpdate(const bt::STBTBotUpdateInfo& info)
	{
		auto mt = reinterpret_cast<MainTrader*>(info.userPtr);
		mt->end = info.current;
		return mt->manager.Update(*mt->tempArena, *info.scope, info.trades, *info.output, info.current);
	}

	void MainTraderCleanup(const bt::STBTBotInfo& info)
	{
		auto mt = reinterpret_cast<MainTrader*>(info.userPtr);

		const float diff = info.scope->GetPortValue(mt->end) - mt->startV;

		if (info.training)
		{
			mt->rating += diff;
			if (++mt->currentInstanceRun >= mt->runsPerInstance)
			{
				mt->ga.debug = true;
				mt->ga.Rate(*mt->arena, *mt->tempArena, mt->rating, *info.output);
				if (mt->ga.trainId == 0)
					info.progress->Add() = mt->ga.genRating;
				mt->rating = 0;
				mt->currentInstanceRun = 0;
			}
		}

		// Save result of training to a file, so that it can be used later.
		if (mt->isFinalRun && info.training)
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
		bot.buffers = buffers;
		bot.bufferNames = bufferNames;
		bot.buffersLength = 2;
		bot.bufferSizes = bufferSizes;
		return bot;
	}
	void MainTrader::InitGA()
	{
		GeneticAlgorithmCreateInfo info{};
		info.width = 30;
		info.userPtr = this;
		info.kmPointCount = 6;
		ga = GeneticAlgorithm::Create(*arena, info);

		currentInstanceRun = 0;
	}
}