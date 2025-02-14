#pragma once

namespace jv::bt
{
	class MI_Backtrader final : public MenuItem<STBT>
	{
	public:
		void Load(STBT& stbt) override;
		void Update(STBT& stbt) override;
		void Unload(STBT& stbt) override;
	};
}