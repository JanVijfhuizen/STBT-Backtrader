#include "pch.h"
#include "Utils/UT_Time.h"

namespace jv::bt
{
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
}