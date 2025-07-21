#include "pch.h"
#include "Tracker.h"
#include "TimeSeries.h"
#include "JLib/Arena.h"

namespace jv::bt
{
	Gnuplot gp("\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\"");

	std::string Tracker::GetData(Arena& tempArena, const char* symbol, const char* path, const char* key)
	{
		typedef std::chrono::system_clock Clock;
		auto now = Clock::now();

		std::time_t now_c = Clock::to_time_t(now);
		tm* parts = std::localtime(&now_c);
		uint32_t year = 1900 + parts->tm_year;
		uint32_t month = 1 + parts->tm_mon;
		uint32_t day = parts->tm_mday;

		uint32_t pYear, pMonth, pDay;

		const std::string symbolName = symbol;
		const std::string extension = ".sym";
		const auto fileName = path + symbolName + extension;

		std::string errorMsg = "{\nUnknown issue.\n}";
		
		{
			bool validButOutdated = false;
			std::ifstream f(fileName);
			if (f.good())
			{
				std::string line;
				bool upToDate = false;
				getline(f, line);

				if (line[0] != '{')
				{
					pDay = std::stoi(line);
					getline(f, line);
					pMonth = std::stoi(line);
					getline(f, line);
					pYear = std::stoi(line);
					upToDate = day == pDay;
					upToDate = upToDate && month == pMonth;
					upToDate = upToDate && year == pYear;
					validButOutdated = !upToDate;
				}

				f.clear();
				f.seekg(0);

				if (upToDate)
				{
					std::ostringstream buf;
					buf << f.rdbuf();
					return buf.str();
				}
			}
			bool getDataCompact = validButOutdated;
			if (validButOutdated)
			{
				std::tm tm = { 0 };
				tm.tm_year = pYear - 1900;
				tm.tm_mon = pMonth - 1;
				tm.tm_mday = pDay;

				std::time_t time2 = std::mktime(&tm);
				const int seconds_per_day = 60 * 60 * 24;
				std::time_t difference = (now_c - time2) / seconds_per_day;
				// 100 is the compact version of alpha vantage's API call.
				if (difference > 100)
					getDataCompact = false;
			}

			_curl = curl_easy_init();
			assert(_curl);
			const auto url = CreateUrl(tempArena, symbol, key, getDataCompact);
			curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			std::string readBuffer;
			curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &readBuffer);
			const auto res = curl_easy_perform(_curl);
			curl_easy_cleanup(_curl);

			errorMsg = readBuffer;

			// If the downloaded data is invalid / no internet connection.
			if (res != 0)
			{
				// If there is no old data, return nothing.
				if (!f.good())
					return "{\nNo Internet connection!\n}";

				// Return old data.
				std::ostringstream buf;
				buf << f.rdbuf();
				return buf.str();
			}

			// Save newly downloaded symbol data.
			if (readBuffer[0] != '{')
			{
				if (!getDataCompact)
				{
					std::ofstream outFile(fileName);
					outFile << day << std::endl;
					outFile << month << std::endl;
					outFile << year << std::endl;
					outFile << readBuffer;
				}
				// Otherwise append to existing data.
				else 
				{
					std::ofstream outFile("temp.txt");
					outFile << day << std::endl;
					outFile << month << std::endl;
					outFile << year << std::endl;

					// Contains newest line from old data.
					std::string line;

					// Remove meta data.
					for (uint32_t i = 0; i < 3; i++)
						getline(f, line);
					// Add default metadata.
					getline(f, line);
					outFile << line << std::endl;
					getline(f, line);

					// Copy new data, but first see how much needs to be copied.
					std::stringstream ss(readBuffer);
					std::string newLine;

					uint32_t lineIndex = 0;
					bool found = false;
					while (getline(ss, newLine))
					{
						if (newLine == line)
						{
							found = true;
							break;
						}
						++lineIndex;
					}

					ss.clear();
					ss.seekg(0);
					// Ignore meta data.
					getline(ss, newLine);
					for (uint32_t i = 0; i < lineIndex; i++)
					{
						getline(ss, newLine);
						outFile << newLine << std::endl;
					}

					// Move remaining data to file.
					std::string rLine;
					while (getline(f, rLine))
						outFile << rLine << std::endl;

					f.close();
					outFile.close();
					std::filesystem::copy("temp.txt", fileName.c_str(), std::filesystem::copy_options::update_existing);
				}
			}	
		}

		std::ifstream f(fileName);
		if (!f.good())
			return errorMsg;

		std::ostringstream buf;
		buf << f.rdbuf();
		return buf.str();
	}

	TimeSeries Tracker::ConvertDataToTimeSeries(Arena& arena, std::string str) const
	{
		std::istringstream f(str);
		std::string line;

		auto lineCount = std::count(str.begin(), str.end(), '\n');
		lineCount += !str.empty() && str.back() != '\n';

		auto timeSeries = CreateTimeSeries(arena, lineCount - 4);

		// Get Date
		std::tm tm{};
		getline(f, line);
		tm.tm_mday = std::stoi(line);
		getline(f, line);
		tm.tm_mon = std::stoi(line) - 1;
		getline(f, line);
		tm.tm_year = std::stoi(line) - 1900;
		timeSeries.date = mktime(&tm);

		// Remove meta info.
		std::getline(f, line);

		uint32_t i = 0;
		while (std::getline(f, line)) 
		{
			std::stringstream ss{line};
			std::string subStr;
			
			// Skip first line.
			getline(ss, subStr, ',');

			{
				std::stringstream test(subStr);
				std::string segment;
				std::string strs[3];

				for (uint32_t j = 0; j < 3; j++)
				{
					std::getline(test, segment, '-');
					strs[j] = segment;
				}

				Date date{};
				date.day = std::stoi(strs[2]);
				date.month = std::stoi(strs[1]);
				timeSeries.dates[i] = date;
			}

			// Open and close are reversed in dataset for some reason.

			getline(ss, subStr, ',');
			timeSeries.close[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.high[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.low[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.open[i] = std::stof(subStr);
			getline(ss, subStr, ',');
			timeSeries.volume[i] = std::stof(subStr);

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

	void Tracker::Debug(const float* ptr, const uint32_t length, const bool reverse)
	{
		std::vector<double> v{};
		for (uint32_t i = 0; i < length; ++i)
			v.push_back(ptr[reverse ? length - 1 - i : i]);
		gp << "set title 'timeline'\n";
		gp << "plot '-' with lines title 'v'\n";
		gp.send(v);
		gp << "pause 1e9\n";
	}

	void Tracker::DebugCandles(const TimeSeries& timeSeries, const uint32_t offset, const uint32_t length)
	{
		{
			std::ofstream fout("candles.dat");

			for (uint32_t i = 0; i < length; ++i)
			{
				const uint32_t index = offset + length - 1 - i;
				fout << i + 1 << " ";
				fout << timeSeries.open[index] << " ";
				fout << timeSeries.low[index] << " ";
				fout << timeSeries.high[index] << " ";
				fout << timeSeries.close[index] << std::endl;
			}
		}

		gp << "set title 'candlesticks'\n";
		gp << "set xrange [0:";
		gp << length + 1;
		gp << "]\n";
		gp << "set boxwidth ";
		gp << .8f;
		gp << "\n";
		gp << "set style fill solid\n";
		gp << "set linetype 1 lc rgb 'red'\n";
		gp << "set linetype 2 lc rgb 'green'\n";
		gp << "plot 'candles.dat' using 1:2:3:4:5:($2 > $5 ? 1 : 2) linecolor variable with candlesticks title 'candlesticks'\n";
		gp << "pause 1e9\n";
	}

	std::string Tracker::CreateUrl(Arena& tempArena, const char* symbol, const char* key, const bool compact)
	{
		const auto scope = tempArena.CreateScope();

		std::string str = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=";
		str.append(symbol);
		str.append("&outputsize=full&apikey=");
		str.append(key);
		str.append("&datatype=csv");
		if (compact)
			str.append("&outputsize=compact");
		tempArena.DestroyScope(scope);
		return str;
	}

	size_t Tracker::WriteCallback(void* contents, const size_t size, const size_t nmemb, void* userp)
	{
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}
}
