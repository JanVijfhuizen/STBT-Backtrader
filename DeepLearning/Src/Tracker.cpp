#include "pch.h"
#include "Tracker.h"
#include "TimeSeries.h"
#include "JLib/Arena.h"

namespace jv::bt
{
	Gnuplot gp("\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\"");

	void Tracker::Init()
	{
		_curl = curl_easy_init();
		assert(_curl);
	}

	void Tracker::Destroy() const
	{
		curl_easy_cleanup(_curl);
	}

	std::string Tracker::GetData(Arena& tempArena, const char* symbol)
	{
		const auto url = CreateUrl(tempArena, symbol);
		curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
		const auto res = curl_easy_perform(_curl);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_readBuffer);
		assert(res == 0);
		return _readBuffer;
	}

	TimeSeries Tracker::ConvertDataToTimeSeries(Arena& arena, std::string str) const
	{
		std::istringstream f(str);
		std::string line;

		auto lineCount = std::count(str.begin(), str.end(), '\n');
		lineCount += !str.empty() && str.back() != '\n';

		const auto timeSeries = CreateTimeSeries(arena, lineCount - 1);

		// Remove first line with meta info.
		std::getline(f, line);
		uint32_t i = 0;
		while (std::getline(f, line)) 
		{
			std::stringstream ss{line};
			std::string subStr;
			
			// Skip first line.
			getline(ss, subStr, ',');

			getline(ss, subStr, ',');
			timeSeries.open[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.high[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.low[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.close[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.volume[i] = std::stoi(subStr);

			++i;
		}

		return timeSeries;
	}

	TimeSeries Tracker::GetTimeSeriesSubSet(Arena& arena, const TimeSeries& timeSeries, const uint32_t depth, const uint32_t length)
	{
		const auto subSet = CreateTimeSeries(arena, length);

		const uint32_t startIndex = timeSeries.length - depth - 1;
		for (uint32_t i = 0; i < length; ++i)
		{
			subSet.open[i] = timeSeries.open[startIndex + i];
			subSet.close[i] = timeSeries.close[startIndex + i];
			subSet.high[i] = timeSeries.high[startIndex + i];
			subSet.low[i] = timeSeries.low[startIndex + i];
			subSet.volume[i] = timeSeries.volume[startIndex + i];
		}

		return subSet;
	}

	void Tracker::Draw(const TimeSeries& timeSeries)
	{
		std::vector<double> v{};
		for (uint32_t i = 0; i < timeSeries.length; ++i)
			v.push_back(timeSeries.close[i]);
		gp << "set title 'timeline'\n";
		gp << "plot '-' with lines title 'v'\n";
		gp.send(v);
		std::cin.get();
	}

	std::string Tracker::CreateUrl(Arena& tempArena, const char* symbol)
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

	size_t Tracker::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}
}
