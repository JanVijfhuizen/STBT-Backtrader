#pragma once
#include <Algorithms/DynNNet.h>

namespace jv
{
	constexpr uint32_t NNET_GEN_SIZE = 80;

	struct NNetTraderDefaultMod final
	{
		float multiplier;
	};

	struct NNetTraderMod final
	{
		void (*init)(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t warmup, void* userPtr);
		void (*update)(const bt::STBTBotInfo& info, uint32_t stockId, uint32_t current, float* out, void* userPtr);
		uint32_t(*getMinBufferSize)(const bt::STBTBotInfo& info, uint32_t stockId, void* userPtr) = nullptr;

		uint32_t outputCount = 1;
		void* userPtr = nullptr;
	};

	struct NNetTraderCreateInfo
	{
		Array<NNetTraderMod> mods;
		const char* loadFile = "loadfile";
	};

	struct NNetTrader
	{
		Array<NNetTraderMod> mods;
	};

	[[nodiscard]] NNetTraderMod NNetGetDefaultMod(NNetTraderDefaultMod& out);
}


