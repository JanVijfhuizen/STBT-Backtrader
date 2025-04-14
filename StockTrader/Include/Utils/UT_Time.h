#pragma once

namespace jv::bt
{
	[[nodiscard]] bool CompDates(time_t a, time_t b);
	[[nodiscard]] std::time_t GetTime(const uint32_t days = 0);
	[[nodiscard]] bool GetMaxTimeLength(STBT& stbt, std::time_t& tCurrent,
		const Array<TimeSeries>& timeSeries, uint32_t& length, const uint32_t buffer);
	[[nodiscard]] std::string ConvertSecondsToHHMMSS(int value);
}