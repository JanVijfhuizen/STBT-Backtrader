#include "pch.h"
#include "WebSocket.h"
#include "JLib/Arena.h"

namespace jv::bt
{
	void WebSocket::Init()
	{
		_curl = curl_easy_init();
		assert(_curl);
	}

	std::string WebSocket::GetData(Arena& tempArena, const char* symbol, const Date date)
	{
		const auto url = CreateUrl(tempArena, symbol, date);
		curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_readBuffer);
		const auto res = curl_easy_perform(_curl);
		assert(res == 0);
		curl_easy_cleanup(_curl);
		return _readBuffer;
	}

	std::string WebSocket::CreateUrl(Arena& tempArena, const char* symbol, const Date date)
	{
		const char* key = "7HIFX74MVML11CUF";

		const auto scope = tempArena.CreateScope();

		std::string str = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=";
		str.append(symbol);
		str.append("&date=");
		str.append(date.ToStr(tempArena));
		str.append("&apikey=");
		str.append(key);
		//str.append("&datatype=csv");
		tempArena.DestroyScope(scope);
		return str;
	}

	size_t WebSocket::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}
}
