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
	
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d");
	const auto str = oss.str();

	char* c = static_cast<char*>(arena.Alloc(sizeof str.length()));
	memcpy(c, str.c_str(), str.length());
	return c;
}

double& jv::TimelineIterator::operator*() const
{
	assert(timeline);
	assert(index <= timeline->length);
	return (*timeline)[index];
}

double& jv::TimelineIterator::operator->() const
{
	assert(timeline);
	assert(index <= timeline->length);
	return (*timeline)[index];
}

const jv::TimelineIterator& jv::TimelineIterator::operator++()
{
	++index;
	return *this;
}

jv::TimelineIterator jv::TimelineIterator::operator++(int)
{
	const TimelineIterator temp{ timeline, index };
	++index;
	return temp;
}

double& jv::Timeline::operator[](const uint32_t i) const
{
	return ptr[(start + i) % length];
}

void jv::Timeline::Next(const double value)
{
	endDate.Adjust(1);
	if (count < length)
	{
		ptr[count++] = value;
		return;
	}

	++start;
	start %= length;
	ptr[(start - 1) % length] = value;
}

bool jv::Timeline::Next(Arena& tempArena, Quote* quote)
{
	Date d = endDate;
	std::cout << d.ToStr(tempArena) << std::endl;
	d.Adjust(1);

	try {
		const auto value = quote->getSpot(d.ToStr(tempArena)).getClose();
		Next(value);
		return true;
	}
	catch (const std::exception& e) {
		endDate.Adjust(1);
		return false;
	}
}

void jv::Timeline::Fill(Arena& tempArena, const Date date, Quote* quote)
{
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
			ptr[count++] = spot.getClose();
		}
		catch (const std::exception& e){}

		endDate = iDate;
		tempArena.DestroyScope(s);
	}
}

jv::TimelineIterator jv::Timeline::begin() const
{
	assert(ptr || length == 0);
	TimelineIterator it{};
	it.timeline = this;
	return it;
}

jv::TimelineIterator jv::Timeline::end() const
{
	assert(ptr || length == 0);
	TimelineIterator it{};
	it.timeline = this;
	it.index = count;
	return it;
}

jv::Timeline jv::CreateTimeline(Arena& arena, const uint32_t length)
{
	Timeline timeline{};
	timeline.ptr = static_cast<double*>(arena.Alloc(sizeof(double) * length));
	timeline.length = length;
	return timeline;
}

void jv::DestroyTimeline(Arena& arena, const Timeline& timeline)
{
	arena.Free(timeline.ptr);
}
