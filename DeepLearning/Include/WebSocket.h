#pragma once
#include <curl/curl.h>

#include "BackTrader.h"

namespace jv
{
	struct Arena;
}

namespace jv::bt
{
	struct WebSocket final
	{
		void Init();

		[[nodiscard]] std::string GetData(jv::Arena& tempArena, const char* symbol, Date date);

	private:
		CURL* _curl = nullptr;
		CURLcode _res{};
		std::string _readBuffer;

		[[nodiscard]] static std::string CreateUrl(Arena& tempArena, const char* symbol, Date date);
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
	};
}
