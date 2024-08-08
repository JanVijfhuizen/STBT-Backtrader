#include "pch.h"

#include <random>
#include "BackTrader.h"
#include "JLib/Arena.h"
#include "curl/curl.h"

void* Alloc(const uint32_t size)
{
	return malloc(size);
}
void Free(void* ptr)
{
	return free(ptr);
}


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

struct WebSocket final
{
	void Init()
	{
		_curl = curl_easy_init();
		assert(_curl);
	}

	[[nodiscard]] std::string GetData(jv::Arena& tempArena, const char* symbol, const jv::bt::Date date)
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

private:
	CURL* _curl;
	CURLcode _res;
	std::string _readBuffer;

	[[nodiscard]] static std::string CreateUrl(jv::Arena& tempArena, const char* symbol, const jv::bt::Date date)
	{
		const char* key = "7HIFX74MVML11CUF";

		const auto scope = tempArena.CreateScope();

		std::string str = "https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=";
		str.append(symbol);
		str.append("&date=");
		str.append(date.ToStr(tempArena));
		str.append("&apikey=");
		str.append(key);
		tempArena.DestroyScope(scope);
		return str;
	}
};

int main()
{
	// BACK TRADER

	// Goal: Make a training ground where you can test various algorithms, including ET-RNNs.
	// Also has to be able to be both used for training, testing and practical

	// start out by basic helper functions, like getting a timeseries for a single stock.
	// Timeseries object, returns everything vectorized (but only things that were requested, enum bitwise). Append to add new data queue wise.

	// Use queues a lot to implement continuious profiling.
	// Basically update old timeseries with new spot

	// Do testing frame based (which is why the queueing is important)
	// simd?

	// include portfolio management
	
	jv::ArenaCreateInfo arenaCreateInfo{};
	arenaCreateInfo.alloc = Alloc;
	arenaCreateInfo.free = Free;
	auto arena = jv::Arena::Create(arenaCreateInfo);
	auto tempArena = jv::Arena::Create(arenaCreateInfo);

	{
		jv::bt::Date date{};
		date.SetToToday();
		date.Adjust(-120);

		WebSocket webSocket{};
		webSocket.Init();

		std::cout << webSocket.GetData(tempArena, "AAPL", date) << std::endl;
		return 0;
	}

	jv::bt::Date date{};
	date.SetToToday();
	date.Adjust(-120);

	jv::bt::Init();
	const auto gspc = jv::bt::AddQuote("^GSPC");
	Explore(gspc, date, 120);
	
	auto timeline = jv::bt::Timeline::Create(arena, 40);
	timeline.Fill(tempArena, date, gspc);
	for (int i = 0; i < 20; ++i)
		timeline.Next(tempArena, gspc);

	timeline.Draw();

	std::cin.get();

	jv::bt::Shutdown();
	return 0;
}
