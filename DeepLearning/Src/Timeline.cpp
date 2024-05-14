#include "pch.h"
#include "Timeline.h"
#include <quote.hpp>
#include "JLib/Arena.h"

constexpr int32_t DAY_MUL = 60 * 60 * 24;

void jv::Date::Set(const uint32_t year, const uint32_t month, const uint32_t day)
{
	std::tm tm{};
	tm.tm_mday = year;
	tm.tm_mon = month;
	tm.tm_year = day;
	value = mktime(&tm);
}

void jv::Date::SetToToday()
{
	value = time(nullptr);
}

void jv::Date::Adjust(const int32_t days)
{
	value += static_cast<long long>(days) * DAY_MUL;
}

const char* jv::Date::ToStr(Arena& arena) const
{
	tm tm{};
	localtime_s(&tm, &value);

	char* c = static_cast<char*>(arena.Alloc(sizeof(char) * 11));
	snprintf(c, sizeof c, "date: %d-%d-%d", tm.tm_year, tm.tm_mon, tm.tm_mday);
	return c;
}

double jv::Timeline::operator[](const uint32_t i) const
{
	return values[(start + i) % length];
}

void jv::Timeline::Enqueue(const double value)
{
	if(count < length)
	{
		values[count++] = value;
		return;
	}

	startDate.Adjust(1);
	++start;
	start %= length;
	values[(start - 1) % length] = value;
}

void jv::Timeline::Fill(Arena& tempArena, const Date date, Quote* quote)
{
	startDate = date;
	count = 0;
	start = 0;

	for (uint32_t i = 0; i < length; ++i)
	{
		const time_t t = date.value + DAY_MUL * static_cast<long long>(i);
		Date iDate{};
		iDate.value = t;

		const auto s = tempArena.CreateScope();
		const auto str = iDate.ToStr(tempArena);

		try {
			auto spot = quote->getSpot(str);
			values[count++] = spot.getClose();
		}
		catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}

		tempArena.DestroyScope(s);
	}
}

jv::Timeline jv::CreateTimeline(Arena& arena, const uint32_t length)
{
	Timeline timeline{};
	timeline.values = static_cast<double*>(arena.Alloc(sizeof(double) * length));
	timeline.length = length;
	return timeline;
}

void jv::DestroyTimeline(Arena& arena, const Timeline& timeline)
{
	arena.Free(timeline.values);
}
