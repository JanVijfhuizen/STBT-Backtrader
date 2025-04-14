#include "pch.h"
#include "Utils/UT_Time.h"

namespace jv::bt
{
	bool CompDates(const time_t a, const time_t b)
	{
		auto at = std::localtime(&a);
		auto bt = std::localtime(&b);

		return at->tm_year == bt->tm_year && at->tm_mon == bt->tm_mon && at->tm_mday == bt->tm_mday;
	}

	std::time_t GetTime(const uint32_t days)
	{
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

		using namespace std::chrono_literals;
		auto offset = 60s * 60 * 24 * days;
		now -= offset;

		std::time_t tCurrent = std::chrono::system_clock::to_time_t(now);

		auto lt = std::localtime(&tCurrent);
		lt->tm_hour = 0;
		lt->tm_min = 0;
		lt->tm_sec = 0;
		tCurrent = mktime(lt);
		return tCurrent;
	}

	bool GetMaxTimeLength(STBT& stbt, std::time_t& tCurrent,
		const Array<TimeSeries>& timeSeries, uint32_t& length, const uint32_t buffer)
	{
		length = UINT32_MAX;

		for (uint32_t i = 0; i < timeSeries.length; i++)
		{
			const auto& series = timeSeries[i];
			std::time_t current = series.date;
			if (i == 0)
				tCurrent = current;
			else if (tCurrent != current)
			{
				stbt.output.Add() = "ERROR: Some symbol data is outdated.";
				return false;
			}
			length = Min<uint32_t>(length, series.length);
		}
		length = Max(buffer, length);
		length -= buffer;

		return true;
	}

	void ClampDates(STBT& stbt, std::time_t& tFrom, std::time_t& tTo,
		std::time_t& tCurrent, const Array<TimeSeries>& timeSeries, uint32_t& length, const uint32_t buffer)
	{
		if (!GetMaxTimeLength(stbt, tCurrent, timeSeries, length, buffer))
			return;

		tFrom = mktime(&stbt.from);
		tTo = mktime(&stbt.to);

		auto minTime = tTo > tFrom ? tTo : tFrom;
		minTime -= (60 * 60 * 24) * length;
		auto& floor = tTo > tFrom ? tFrom : tTo;
		if (floor < minTime)
		{
			floor = minTime;
			(tTo > tFrom ? stbt.from : stbt.to) = *std::gmtime(&floor);
		}

		if (tFrom >= tCurrent)
		{
			tFrom = tCurrent;
			stbt.from = *std::gmtime(&tCurrent);
		}
		if (tTo >= tCurrent)
		{
			tTo = tCurrent;
			stbt.to = *std::gmtime(&tCurrent);
		}
		if (tFrom > tTo)
		{
			auto tTemp = tTo;
			tTo = tFrom;
			tFrom = tTemp;
		}
	}
	std::string ConvertSecondsToHHMMSS(int value)
	{
		{
			std::string result;
			// compute h, m, s
			std::string h = std::to_string(value / 3600);
			std::string m = std::to_string((value % 3600) / 60);
			std::string s = std::to_string(value % 60);
			// add leading zero if needed
			std::string hh = std::string(2 - h.length(), '0') + h;
			std::string mm = std::string(2 - m.length(), '0') + m;
			std::string ss = std::string(2 - s.length(), '0') + s;
			// return mm:ss if hh is 00
			if (hh.compare("00") != 0) {
				result = hh + ':' + mm + ":" + ss;
			}
			else {
				result = mm + ":" + ss;
			}
			return result;
		}
	}
}