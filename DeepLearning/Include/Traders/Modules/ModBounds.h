#pragma once
#include <Traders/TMM.h>

namespace jv
{
	struct ModBounds final : public tmm::Module
	{
		uint32_t masLen = 20;
		uint32_t stdLen = 20;
		float stdMul = 2;

		uint32_t start;
		uint32_t end;
		float** mas;

		bool Init(Arena& arena, const tmm::Info& info,
			const jv::bt::STBTScope& scope, Queue<bt::OutputMsg>& output) override;
		bool Update(Arena& tempArena, const bt::STBTScope& scope,
			float* values, Queue<bt::OutputMsg>& output, uint32_t current) override;
		void Cleanup(Arena& arena, Queue<bt::OutputMsg>& output) override;
		uint32_t GetValuesLength(const bt::STBTScope& scope) override;
	};
}
