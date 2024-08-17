#pragma once
#include <curl/curl.h>

#include "TimeSeries.h"

namespace jv
{
	struct Arena;
}

namespace jv::bt
{
	class Tracker final
	{
	public:
		[[nodiscard]] std::string GetData(Arena& tempArena, const char* symbol);
		[[nodiscard]] TimeSeries ConvertDataToTimeSeries(Arena& arena, std::string str) const;
		[[nodiscard]] static TimeSeries GetTimeSeriesSubSet(Arena& arena, const TimeSeries& timeSeries, uint32_t depth, uint32_t length);
		static void Debug(const float* ptr, uint32_t length, bool reverse);
		static void DebugCandles(const TimeSeries& timeSeries, uint32_t offset, uint32_t length);

	private:
		CURL* _curl = nullptr;
		CURLcode _res{};
		std::string _readBuffer;
		uint32_t _valid = 0;

		[[nodiscard]] static std::string CreateUrl(Arena& tempArena, const char* symbol);
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
	};
}
