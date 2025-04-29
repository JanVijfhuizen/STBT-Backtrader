#include "pch.h"
#include "Traders/TMM.h"

namespace jv::tmm
{
	Manager Manager::Create(Arena& arena, const uint32_t length)
	{
		Manager manager{};
		manager.scope = arena.CreateScope();
		manager.modules = arena.New<Module*>(length);
		manager.trader = DefaultTrader;
		return manager;
	}
	void Manager::Set(const uint32_t index, Module* module)
	{
		modules[index] = module;
	}
	bool Manager::Init(Arena& arena, const Info& info, const jv::bt::STBTScope& scope, jv::Queue<const char*>& output)
	{
		runScope = arena.CreateScope();
		for (uint32_t i = 0; i < length; i++)
			if (!modules[i]->Init(arena, info, scope, output))
			{
				for (int32_t j = i - 1; j >= 0; j--)
					modules[i]->Cleanup(arena, output);
				arena.DestroyScope(runScope);
				return false;
			}
		return true;
	}
	bool Manager::Update(Arena& tempArena, jv::bt::STBTScope& scope, 
		jv::bt::STBTTrade* trades, jv::Queue<const char*>& output, const uint32_t current)
	{
		auto tempScope = tempArena.CreateScope();
		float** values = tempArena.New<float*>(length);

		for (uint32_t i = 0; i < length; i++)
		{
			const uint32_t l = modules[i]->GetValuesLength(scope);
			values[i] = tempArena.New<float>(l);
		}

		for (uint32_t i = 0; i < length; i++)
			if (!modules[i]->Update(tempArena, scope, values[i], output, current))
			{
				tempArena.DestroyScope(tempScope);
				return false;
			}

		tempArena.DestroyScope(tempScope);
		return true;
	}
	void Manager::Cleanup(Arena& arena, jv::Queue<const char*>& output)
	{
		for (int32_t i = length - 1; i >= 0; i--)
			modules[i]->Cleanup(arena, output);
		arena.DestroyScope(runScope);
	}
	void Manager::Destroy(Arena& arena, const Manager& manager)
	{
		arena.DestroyScope(manager.scope);
	}
	void DefaultTrader(Arena& tempArena, bt::STBTScope& scope, float** values, 
		bt::STBTTrade* trades, Queue<const char*>& output, const uint32_t current)
	{
		return;

		/*
		// Buy with an even distribution.
		const auto liq = scope.GetLiquidity();
		float stackPrice = 0;

		for (auto& buy : buys)
			stackPrice += scope.GetTimeSeries(buy).close[current];

		const uint32_t stackAmount = Max<float>(liq / stackPrice, 1);
		for (auto& buy : buys)
			trades[buy].change = stackAmount;

		for (auto& sell : sells)
			trades[sell].change = -1e5;
			*/
	}
}