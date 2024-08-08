#pragma once
#include <curl/curl.h>

#include "BackTrader.h"
#include "Point.h"
#include "JLib/Array.h"

namespace jv
{
	struct Arena;
}

namespace jv::bt
{
	struct WebSocket final
	{
		void Init();

		[[nodiscard]] std::string GetData(Arena& tempArena, const char* symbol);
		[[nodiscard]] Array<Point> ConvertDataToPoints(Arena& arena, std::string str) const;

	private:
		CURL* _curl = nullptr;
		CURLcode _res{};
		std::string _readBuffer;

		[[nodiscard]] static std::string CreateUrl(Arena& tempArena, const char* symbol);
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
	};
}
