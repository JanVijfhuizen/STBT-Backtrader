#include "pch.h"
#include "WebSocket.h"
#include "JLib/Arena.h"
#include "JLib/ArrayUtils.h"

namespace jv::bt
{
	void WebSocket::Init()
	{
		_curl = curl_easy_init();
		assert(_curl);
	}

	std::string WebSocket::GetData(Arena& tempArena, const char* symbol)
	{
		const auto url = CreateUrl(tempArena, symbol);
		curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_readBuffer);
		const auto res = curl_easy_perform(_curl);
		assert(res == 0);
		curl_easy_cleanup(_curl);
		return _readBuffer;
	}

	Array<Point> WebSocket::ConvertDataToPoints(Arena& arena, std::string str) const
	{
		std::istringstream f(str);
		std::string line;

		auto lineCount = std::count(str.begin(), str.end(), '\n');
		lineCount += !str.empty() && str.back() != '\n';

		const auto points = CreateArray<Point>(arena, lineCount - 1);

		// Remove first line with meta info.
		std::getline(f, line);
		uint32_t i = 0;
		while (std::getline(f, line)) 
		{
			std::stringstream ss{line};
			std::string subStr;

			auto& point = points[i++];

			// Skip first line.
			getline(ss, subStr, ',');

			getline(ss, subStr, ',');
			point.open = std::stof(subStr);
			getline(ss, subStr, ',');
			point.high = std::stof(subStr);
			getline(ss, subStr, ',');
			point.low = std::stof(subStr);
			getline(ss, subStr, ',');
			point.close = std::stof(subStr);
			getline(ss, subStr, ',');
			point.volume = std::stoi(subStr);
		}

		return points;
	}

	std::string WebSocket::CreateUrl(Arena& tempArena, const char* symbol)
	{
		const char* key = "7HIFX74MVML11CUF";
		const auto scope = tempArena.CreateScope();

		std::string str = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=";
		str.append(symbol);
		str.append("&outputsize=full&apikey=");
		str.append(key);
		str.append("&datatype=csv");
		tempArena.DestroyScope(scope);
		return str;
	}

	size_t WebSocket::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}
}
