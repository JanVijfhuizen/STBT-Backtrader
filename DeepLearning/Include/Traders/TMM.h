#pragma once
#include <STBT.h>

namespace jv::tmm
{
	struct Info final
	{
		uint32_t start;
		uint32_t end;
		uint32_t runIndex; 
		uint32_t nRuns;
		uint32_t buffer;
	};

	class Module
	{
	public:
		virtual bool Init(Arena& arena, const Info& info,
			const bt::STBTScope& scope, Queue<const char*>& output) = 0;
		virtual bool Update(Arena& tempArena, bt::STBTScope& scope, 
			float* values, Queue<const char*>& output, uint32_t current) = 0;
		virtual void Cleanup(Arena& arena, Queue<const char*>& output) = 0;
		virtual uint32_t GetValuesLength(const bt::STBTScope& scope) = 0;
	};

	struct Manager final
	{
		uint64_t scope;
		Module** modules;
		uint32_t length;
		uint64_t runScope;

		void (*trader)(Arena& tempArena, bt::STBTScope& scope, float** values, 
			bt::STBTTrade* trades, Queue<const char*>& output, uint32_t current) = nullptr;

		[[nodiscard]] static Manager Create(Arena& arena, uint32_t length);
		void Set(uint32_t index, Module* module);
		[[nodiscard]] bool Init(Arena& arena, const Info& info, const bt::STBTScope& scope, Queue<const char*>& output);
		[[nodiscard]] bool Update(Arena& tempArena, bt::STBTScope& scope, 
			bt::STBTTrade* trades, Queue<const char*>& output, uint32_t current);
		[[nodiscard]] void Cleanup(Arena& arena, Queue<const char*>& output);
		static void Destroy(Arena& arena, const Manager& manager);
	};

	static void DefaultTrader(Arena& tempArena, const bt::STBTScope& scope, float** values, 
		bt::STBTTrade* trades, Queue<const char*>& output, uint32_t current);
}