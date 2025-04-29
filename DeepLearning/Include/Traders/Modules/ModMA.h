#pragma once
#include <Traders/TMM.h>

namespace jv
{
	class ModMA final : public tmm::Module
	{
	public:
		uint32_t mas1Len = 30;
		uint32_t mas2Len = 100;
		float buyThreshPct = .1f;
		float sellThreshPct = .1f;

		uint32_t start;
		uint32_t end;
		float** mas1;
		float** mas2;

		bool Init(Arena& arena, const tmm::Info& info,
			const jv::bt::STBTScope& scope, Queue<const char*>& output) override;
		bool Update(Arena& tempArena, const bt::STBTScope& scope, 
			float* values, Queue<const char*>& output, uint32_t current) override;
		void Cleanup(Arena& arena, Queue<const char*>& output) override;
		uint32_t GetValuesLength(const bt::STBTScope& scope) override;
	};
}