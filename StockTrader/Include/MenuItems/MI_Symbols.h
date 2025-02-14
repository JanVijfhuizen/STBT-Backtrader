#pragma once

namespace jv::bt
{
	class MI_Symbols final : public MenuItem<STBT>
	{
	public:
		void Load(STBT& stbt) override;
		void Update(STBT& stbt) override;
		void Unload(STBT& stbt) override;

		static void LoadSymbols(STBT& stbt);
		static void LoadEnabledSymbols(STBT& stbt);
	};
}