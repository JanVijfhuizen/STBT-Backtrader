#pragma once
#include <STBT.h>

namespace jv::bt
{
	class MI_Backtrader final : public MenuItem<STBT>
	{
	public:
		void Load(STBT& t) override;
		void Update(STBT& t) override;
		void Unload(STBT& t) override;
	};
}