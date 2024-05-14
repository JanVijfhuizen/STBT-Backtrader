#include "pch.h"
#include "BackTrader.h"

#include <quote.hpp>
#include "JLib/Arena.h"
#include "JLib/LinkedListUtils.h"

namespace jv::bt
{
	void* Alloc(const uint32_t size)
	{
		return malloc(size);
	}
	void Free(void* ptr)
	{
		return free(ptr);
	}

	Gnuplot gp("\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\"");
	struct BackTrader final
	{
		Arena arena, tempArena;
		LinkedList<Quote*> quotes{};
	} trader{};

	void Date::Set(const uint32_t year, const uint32_t month, const uint32_t day)
	{
		std::tm tm{};
		tm.tm_mday = year;
		tm.tm_mon = month;
		tm.tm_year = day;
		value = mktime(&tm);
	}

	void Date::SetToToday()
	{
		value = time(nullptr);
	}

	void Date::Adjust(const int32_t days)
	{
		value += static_cast<long long>(days) * DAY_MUL;
	}

	const char* Date::ToStr(Arena& arena) const
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

	double& TimelineIterator::operator*() const
	{
		assert(timeline);
		assert(index <= timeline->_length);
		return (*timeline)[index];
	}

	double& TimelineIterator::operator->() const
	{
		assert(timeline);
		assert(index <= timeline->_length);
		return (*timeline)[index];
	}

	const TimelineIterator& TimelineIterator::operator++()
	{
		++index;
		return *this;
	}

	TimelineIterator TimelineIterator::operator++(int)
	{
		const TimelineIterator temp{ timeline, index };
		++index;
		return temp;
	}

	double& Timeline::operator[](const uint32_t i) const
	{
		return _ptr[(_start + i) % _length];
	}

	void Timeline::Next(const double value)
	{
		_endDate.Adjust(1);
		if (_count < _length)
		{
			_ptr[_count++] = value;
			return;
		}

		++_start;
		_start %= _length;
		_ptr[(_start - 1) % _length] = value;
	}

	bool Timeline::Next(Arena& tempArena, const uint32_t quote)
	{
		Date d = _endDate;
		d.Adjust(1);

		auto q = trader.quotes[quote];

		try {
			const auto value = q->getSpot(d.ToStr(tempArena)).getClose();
			Next(value);
			return true;
		}
		catch (const std::exception& e) {
			_endDate.Adjust(1);
			return false;
		}
	}

	void Timeline::Fill(Arena& tempArena, const Date date, const uint32_t quote)
	{
		_count = 0;
		_start = 0;

		auto q = trader.quotes[quote];

		for (uint32_t i = 0; i < _length; ++i)
		{
			const time_t t = date.value + DAY_MUL * static_cast<long long>(i);
			Date iDate{};
			iDate.value = t;

			const auto s = tempArena.CreateScope();
			const auto str = iDate.ToStr(tempArena);

			try {
				auto spot = q->getSpot(str);
				_ptr[_count++] = spot.getClose();
			}
			catch (const std::exception& e) {}

			_endDate = iDate;
			tempArena.DestroyScope(s);
		}
	}

	void Timeline::Draw()
	{
		std::vector<double> v;
		for (auto d : *this)
			v.push_back(d);
		gp << "set title 'timeline'\n";
		gp << "plot '-' with lines title 'v'\n";
		gp.send(v);
		std::cin.get();
	}

	TimelineIterator Timeline::begin() const
	{
		assert(_ptr || _length == 0);
		TimelineIterator it{};
		it.timeline = this;
		return it;
	}

	TimelineIterator Timeline::end() const
	{
		assert(_ptr || _length == 0);
		TimelineIterator it{};
		it.timeline = this;
		it.index = _count;
		return it;
	}

	Timeline Timeline::Create(Arena& arena, const uint32_t length)
	{
		Timeline timeline{};
		timeline._ptr = static_cast<double*>(arena.Alloc(sizeof(double) * length));
		timeline._length = length;
		return timeline;
	}

	void Timeline::Destroy(Arena& arena, const Timeline& timeline)
	{
		arena.Free(timeline._ptr);
	}

	void Init()
	{
		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = Alloc;
		arenaCreateInfo.free = Free;
		trader.arena = Arena::Create(arenaCreateInfo);
		trader.tempArena = Arena::Create(arenaCreateInfo);
	}

	void Shutdown()
	{
		for (const auto quote : trader.quotes)
			delete quote;
		Arena::Destroy(trader.arena);
		Arena::Destroy(trader.tempArena);
		trader = {};
	}

	uint32_t AddQuote(const char* str)
	{
		Add(trader.arena, trader.quotes) = new Quote(str);
		return trader.quotes.GetCount() - 1;
	}

	void Explore(const uint32_t quote, Date startDate, const uint32_t days)
	{
		const auto quoteInstance = trader.quotes[quote];
		const auto scope = trader.tempArena.CreateScope();
		const auto start = startDate.ToStr(trader.tempArena);
		startDate.Adjust(days);
		const auto end = startDate.ToStr(trader.tempArena);
		quoteInstance->getHistoricalSpots(start, end, "1d");
		trader.tempArena.DestroyScope(scope);
	}

	double GetPortfolioLiquidity(const Stock* portfolio, const uint32_t length, const Date date)
	{
		double value = 0;
		for (int i = 0; i < length; ++i)
		{
			auto& stock = portfolio[i];
			Explore(stock.quote, date, 1);
			auto q = trader.quotes[stock.quote];

			try {
				const auto scope = trader.tempArena.CreateScope();
				const auto spot = q->getSpot(date.ToStr(trader.tempArena)).getClose();
				value += spot * stock.count;
				trader.tempArena.DestroyScope(scope);
			}
			catch (const std::exception& e)
			{
				return -1;
			}
		}
		return value;
	}
}
