#pragma once
#include <Traders/TMM.h>

namespace jv
{
	class ModMA final : public tmm::Module
	{
	public:
		uint32_t mas1Len = 5;
		uint32_t mas2Len = 30;
		float buyThreshPct = .01f;
		float sellThreshPct = .01f;

		uint32_t start;
		uint32_t end;
		float** mas1;
		float** mas2;

		bool Init(Arena& arena, const tmm::Info& info,
			const jv::bt::STBTScope& scope, Queue<bt::OutputMsg>& output) override;
		bool Update(Arena& tempArena, const bt::STBTScope& scope, 
			float* values, Queue<bt::OutputMsg>& output, uint32_t current) override;
		void Cleanup(Arena& arena, Queue<bt::OutputMsg>& output) override;
		uint32_t GetValuesLength(const bt::STBTScope& scope) override;
	};
}