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
		void Init();
		void Destroy() const;

		[[nodiscard]] std::string GetData(Arena& tempArena, const char* symbol);
		[[nodiscard]] TimeSeries ConvertDataToTimeSeries(Arena& arena, std::string str) const;
		[[nodiscard]] static TimeSeries GetTimeSeriesSubSet(Arena& arena, const TimeSeries& timeSeries, uint32_t depth, uint32_t length);
		static void Draw(const TimeSeries& timeSeries);

	private:
		CURL* _curl = nullptr;
		CURLcode _res{};
		std::string _readBuffer;

		[[nodiscard]] static std::string CreateUrl(Arena& tempArena, const char* symbol);
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
	};
}
