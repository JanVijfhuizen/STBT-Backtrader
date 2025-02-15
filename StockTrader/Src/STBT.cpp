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
		bool quit = menu.Update(arena, *this);
		if (quit)
			return true;

		ImGui::Begin("Output", nullptr, WIN_FLAGS);
		ImGui::SetWindowPos({ 0, 400 });
		ImGui::SetWindowSize({ 400, 200 });

		for (auto& a : output)
			ImGui::Text(a);

		ImGui::End();

		const bool ret = renderer.Render();
		frameArena.Clear();
		return ret;
	}
	STBT CreateSTBT()
	{
		STBT stbt{};
		stbt.tracker = {};

		jv::gr::RendererCreateInfo createInfo{};
		createInfo.title = "STBT (Stock Trading Back Tester)";
		stbt.renderer = jv::gr::CreateRenderer(createInfo);

		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = MAlloc;
		arenaCreateInfo.free = MFree;
		stbt.arena = Arena::Create(arenaCreateInfo);
		stbt.tempArena = Arena::Create(arenaCreateInfo);
		stbt.frameArena = Arena::Create(arenaCreateInfo);

		stbt.output = CreateQueue<const char*>(stbt.arena, 50);

		auto& menu = stbt.menu = Menu<STBT>::CreateMenu(stbt.arena, 4);
		menu.Add() = stbt.arena.New<MI_MainMenu>();
		menu.Add() = stbt.arena.New<MI_Symbols>();
		menu.Add() = stbt.arena.New<MI_Backtrader>();
		menu.Add() = stbt.arena.New<MI_Licensing>();
		menu.Init(stbt.arena, stbt);
		menu.SetIndex(stbt.arena, stbt, 0);

		auto t = GetTime();
		stbt.to = *std::gmtime(&t);
		t = GetTime(DAYS_DEFAULT);
		stbt.from = *std::gmtime(&t);
		stbt.graphType = 0;
		stbt.ma = 0;
		stbt.days = DAYS_DEFAULT;

		/*
		
		stbt.L = nullptr;
		snprintf(stbt.feeBuffer, sizeof(stbt.feeBuffer), "%f", 1e-3f);
		snprintf(stbt.buffBuffer, sizeof(stbt.buffBuffer), "%i", 0);
		snprintf(stbt.runBuffer, sizeof(stbt.runBuffer), "%i", 1);
		stbt.randomizeDate = false;
		stbt.log = true;
		stbt.runsQueued = 0;
		*/
		return stbt;
	}
	void DestroySTBT(STBT& stbt)
	{
		Arena::Destroy(stbt.frameArena);
		Arena::Destroy(stbt.tempArena);
		Arena::Destroy(stbt.arena);
		DestroyRenderer(stbt.renderer);
	}
}