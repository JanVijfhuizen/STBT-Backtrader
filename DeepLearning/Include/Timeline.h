#pragma once

class Quote;

namespace jv
{
	struct Arena;

	struct Date
	{
		time_t value;

		void Set(uint32_t year, uint32_t month, uint32_t day);
		void SetToToday();
		void Adjust(int32_t days);
		[[nodiscard]] const char* ToStr(Arena& arena) const;
	};

	struct Timeline final
	{
		[[nodiscard]] double operator[](uint32_t i) const;

		double* values;
		uint32_t length;
		uint32_t count = 0;
		uint32_t start = 0;
		Date startDate;

		void Enqueue(double value);
		void Fill(Arena& tempArena, Date date, Quote* quote);
	};

	[[nodiscard]] Timeline CreateTimeline(Arena& arena, uint32_t length);
	void DestroyTimeline(Arena& arena, const Timeline& timeline);
}
