#include "pch.h"
#include <STBT.h>
#include <Algorithms/GeneticAlgorithm.h>
#include <TraderUtils.h>
#include <Traders/GATrader.h>
#include <Traders/TradTrader.h>
#include <Traders/MainTrader.h>
#include <Traders/CorrolationTrader.h>
#include <Algorithms/KMeans.h>

#include <Algorithms/NNet.h>
#include <Algorithms/HyperNNet.h>

void* MAlloc(const uint32_t size)
{
	return malloc(size);
}

void MFree(void* ptr)
{
	return free(ptr);
}

int main()
{
	srand(time(NULL));

	jv::Arena arena, tempArena;
	{
		jv::ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		arena = jv::Arena::Create(arenaCreateInfo);
		tempArena = jv::Arena::Create(arenaCreateInfo);
	}

	{
		jv::ai::HyperNNetCreateInfo info{};
		info.inputCount = 1;
		info.outputCount = 2;
		info.hiddenCount = 10;
		info.hiddenPropagations = 5;
		auto in = jv::ai::HyperNNetInfo::Create(info);

		auto a = jv::ai::HyperNNet::Create(arena, info);
		auto b = jv::ai::HyperNNet::Create(arena, info);
		a.Randomize(in);
		b.Randomize(in);

		while (true)
		{
			a.Flush(in);
			b.Flush(in);

			uint32_t scores[2]{};
			for (uint32_t i = 0; i < 2; i++)
			{
				auto& score =  scores[i];
				for (uint32_t j = 0; j < 100; j++)
				{
					float s = sin(.12f * j);
					const uint32_t moreThanZero = s > 0;

					uint32_t output[2]{};
					a.Propagate(tempArena, in, &s, output);

					for (uint32_t k = 0; k < 2; k++)
						output[k] = output[k] > 0 ? 1 : 0;

					// Discourage uncertainty.
					if (output[0] == output[1])
						continue;
					// Reward for at least choosing one.
					++score;
					score += (output[0] == moreThanZero) * 2;
				}
			}

			auto& best = scores[0] > scores[1] ? a : b;
			auto& worst = &best == &a ? b : a;

			jv::ai::HyperNNet::Copy(info, best, worst);
			worst.Mutate(info);

			std::cout << jv::Max<uint32_t>(scores[0], scores[1]) << std::endl;
		}
	}
	
	auto tradTrader = jv::TradTrader::Create(arena, tempArena);
	auto gaTrader = jv::GATrader::Create(arena, tempArena);
	auto mainTrader = jv::MainTrader::Create(arena, tempArena);
	auto corTrader = jv::CorrolationTrader::Create(arena, tempArena);
	mainTrader.InitGA();

	jv::bt::STBTBot bots[4];
	bots[0] = gaTrader.GetBot();
	bots[1] = tradTrader.GetBot();
	bots[2] = mainTrader.GetBot();
	bots[3] = corTrader.GetBot();

	auto stbt = jv::bt::CreateSTBT(bots, sizeof(bots) / sizeof(jv::bt::STBTBot));
	while (!stbt.Update())
		continue;

	jv::Arena::Destroy(arena);
	return 0;
}
