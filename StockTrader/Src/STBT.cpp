#include "pch.h"
#include "STBT.h"
#include <Jlib/ArrayUtils.h>
#include "JLib/QueueUtils.h"
#include <MenuItems/MI_MainMenu.h>
#include <MenuItems/MI_Symbols.h>
#include <MenuItems/MI_Licensing.h>
#include <MenuItems/MI_Backtrader.h>
#include <Utils/UT_Time.h>

namespace jv::bt
{
	void* MAlloc(const uint32_t size)
	{
		return malloc(size);
	}

	void MFree(void* ptr)
	{
		return free(ptr);
	}

	bool STBT::Update()
	{
		// Update (sub)menu(s).
		bool quit = menu.Update(arena, *this);
		if (quit)
			return true;

		// Draw output window.
		ImGui::Begin("Output", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 0, 400 });
		ImGui::SetWindowSize({ 200.f * (menu.index == 0 ? 1 : 2), 200});
		for (auto& a : output)
			ImGui::Text(a);
		
		ImGui::End();

		const bool ret = renderer.Render();
		frameArena.Clear();
		
		return ret;
	}

	STBT CreateSTBT(STBTBot* bots, const uint32_t botCount)
	{
		STBT stbt{};
		stbt.tracker = {};

		// Create renderer.
		jv::gr::RendererCreateInfo createInfo{};
		createInfo.title = "STBT (Stock Trading Back Tester)";
		stbt.renderer = jv::gr::CreateRenderer(createInfo);

		// Create memory management tools.
		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		stbt.arena = Arena::Create(arenaCreateInfo);
		stbt.tempArena = Arena::Create(arenaCreateInfo);
		stbt.frameArena = Arena::Create(arenaCreateInfo);

		// Create output log and copy bots.
		stbt.output = CreateQueue<const char*>(stbt.arena, 50);
		stbt.bots = CreateArray<STBTBot>(stbt.arena, botCount);
		for (uint32_t i = 0; i < botCount; i++)
			stbt.bots[i] = bots[i];

		// Create menu classes.
		auto& menu = stbt.menu = Menu<STBT>::CreateMenu(stbt.arena, 4);
		menu.Add() = stbt.arena.New<MI_MainMenu>();
		menu.Add() = stbt.arena.New<MI_Symbols>();
		menu.Add() = stbt.arena.New<MI_Backtrader>();
		menu.Add() = stbt.arena.New<MI_Licensing>();
		menu.Init(stbt.arena, stbt);
		menu.SetIndex(stbt.arena, stbt, 0);

		// Initialize basic settings.
		stbt.graphType = 0;
		stbt.range = DAYS_DEFAULT;

		return stbt;
	}

	void DestroySTBT(STBT& stbt)
	{
		Arena::Destroy(stbt.frameArena);
		Arena::Destroy(stbt.tempArena);
		Arena::Destroy(stbt.arena);
		DestroyRenderer(stbt.renderer);
	}

	float STBTScope::GetLiquidity() const
	{
		return portfolio->liquidity;
	}

	float STBTScope::GetPortValue(const uint32_t current) const
	{
		const uint32_t l = GetTimeSeriesCount();
		float ret = portfolio->liquidity;
		for (uint32_t i = 0; i < l; i++)
			ret += GetNInPort(i) * GetTimeSeries(i).close[current];
		return ret;
	}

	uint32_t STBTScope::GetNInPort(const uint32_t index) const
	{
		return portfolio->stocks[index].count;
	}

	TimeSeries STBTScope::GetTimeSeries(const uint32_t index) const
	{
		return timeSeries[index];
	}

	uint32_t STBTScope::GetLength() const
	{
		assert(timeSeries.length > 0);
		return timeSeries[0].length;
	}
	uint32_t STBTScope::GetTimeSeriesCount() const
	{
		return timeSeries.length;
	}
	STBTScope STBTScope::Create(Portfolio* portfolio, const Array<TimeSeries> timeSeries)
	{
		STBTScope scope{};
		scope.timeSeries = timeSeries;
		scope.portfolio = portfolio;
		return scope;
	}
}