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
		[[nodiscard]] std::string GetData(Arena& tempArena, const char* symbol, const char* path, const char* key = "7HIFX74MVML11CUF");
		[[nodiscard]] TimeSeries ConvertDataToTimeSeries(Arena& arena, std::string str) const;
		[[nodiscard]] static TimeSeries GetTimeSeriesSubSet(Arena& arena, const TimeSeries& timeSeries, uint32_t depth, uint32_t length);
		static void Debug(const float* ptr, uint32_t length, bool reverse);
		static void DebugCandles(const TimeSeries& timeSeries, uint32_t offset, uint32_t length);

	private:
		CURL* _curl = nullptr;
		CURLcode _res{};

		[[nodiscard]] static std::string CreateUrl(Arena& tempArena, const char* symbol, const char* key);
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
	};
}
