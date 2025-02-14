#pragma once

namespace jv::bt
{
	class MI_Licensing final : public MenuItem<STBT>
	{
	public:
		void Load(STBT& t) override;
		void Update(STBT& t) override;
		void Unload(STBT& t) override;
	};
}

