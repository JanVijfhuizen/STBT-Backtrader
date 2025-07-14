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
		const ImVec4 col = { 0, 0, 0, 0 };
		const ImVec4 col2 = { .2, .25, .2, 1 };
		const ImVec4 col3 = { .3, .35, .3, 1 };
		const ImVec4 col4 = { .45, .5, .45, 1 };
		const ImVec4 col5 = { 1, 1, 1, 1 };
		
		ImGui::PushStyleColor(ImGuiCol_FrameBg, col2);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, col3);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, col4);

		ImGui::PushStyleColor(ImGuiCol_Button, col2);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col4);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, col2);

		ImGui::PushStyleColor(ImGuiCol_Header, col3);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, col3);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, col);

		ImGui::PushStyleColor(ImGuiCol_CheckMark, col5);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, col);
		
		// Update (sub)menu(s).
		bool quit = menu.Update(arena, *this);
		if (!quit)
		{
			uint32_t mul = menu.index == 0 ? 1 : 2;
			mul = outputFocused ? 4 : mul;

			// Draw output window.
			ImGui::Begin("Output", nullptr, WIN_FLAGS | ImGuiWindowFlags_HorizontalScrollbar);
			ImGui::SetWindowPos({ 0, 400 });
			ImGui::SetWindowSize({ 200.f * mul, 200 });
			outputFocused = ImGui::IsWindowFocused();

			for (auto& a : output)
			{
				ImVec4 col = { a.color.r, a.color.g, a.color.b, a.color.a };
				ImGui::PushStyleColor(ImGuiCol_Text, col);
				ImGui::Text(a.buffer);
				ImGui::PopStyleColor();
			}

			ImGui::End();

			for (uint32_t i = 0; i < 11; i++)
				ImGui::PopStyleColor();
			quit = renderer.Render();
			frameArena.Clear();
		}
		
		return quit;
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
		stbt.output = CreateQueue<OutputMsg>(stbt.arena, 50);
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
		stbt.outputFocused = true;

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