#pragma once

namespace jv
{
	struct Arena;
}

namespace jv::bt
{
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
		friend TimelineIterator;

		[[nodiscard]] double& operator[](uint32_t i) const;

		void Next(double value);
		bool Next(Arena& tempArena, uint32_t quote);
		void Fill(Arena& tempArena, Date date, uint32_t quote);
		void Draw();

		[[nodiscard]] TimelineIterator begin() const;
		[[nodiscard]] TimelineIterator end() const;

		[[nodiscard]] static Timeline Create(Arena& arena, uint32_t length);
		static void Destroy(Arena& arena, const Timeline& timeline);

	private:
		double* _ptr;
		uint32_t _length;
		uint32_t _count = 0;
		uint32_t _start = 0;
		Date _endDate;
	};

	void Init();
	void Shutdown();

	[[nodiscard]] uint32_t AddQuote(const char* str);
	void Explore(uint32_t quote, Date startDate, uint32_t days);

	struct Stock final
	{
		uint32_t quote = -1;
		uint32_t count = 0;
	};

	[[nodiscard]] double GetPortfolioLiquidity(const Stock* portfolio, uint32_t length, Date date);

	struct Box final
	{
		Stock stocks[256]{};
		Date date;
		double initialLiquidity;
		double liquidCash;
		double exchangeRates;
		void* userPtr;
		double tradeFee;

		void Buy(uint32_t count, uint32_t quote);
		void Sell(uint32_t count, uint32_t quote);

		void (*onInit)(Box& box) = nullptr;
		void (*onUpdate)(Box& box, Date date);
		void (*onExit)(Box& box) = nullptr;
	};
}
