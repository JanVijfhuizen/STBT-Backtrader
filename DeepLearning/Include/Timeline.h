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
	
	struct TimelineIterator final
	{
		struct Timeline const* timeline = nullptr;
		uint32_t index = 0;

		double& operator*() const;
		double& operator->() const;

		const TimelineIterator& operator++();
		TimelineIterator operator++(int);

		friend bool operator==(const TimelineIterator& a, const TimelineIterator& b)
		{
			return a.index == b.index;
		}

		friend bool operator!= (const TimelineIterator& a, const TimelineIterator& b)
		{
			return !(a == b);
		}
	};

	struct Timeline final
	{
		[[nodiscard]] double& operator[](uint32_t i) const;

		double* ptr;
		uint32_t length;
		uint32_t count = 0;
		uint32_t start = 0;
		Date startDate;

		void Enqueue(double value);
		void Fill(Arena& tempArena, Date date, Quote* quote);

		[[nodiscard]] TimelineIterator begin() const;
		[[nodiscard]] TimelineIterator end() const;
	};

	[[nodiscard]] Timeline CreateTimeline(Arena& arena, uint32_t length);
	void DestroyTimeline(Arena& arena, const Timeline& timeline);
}
