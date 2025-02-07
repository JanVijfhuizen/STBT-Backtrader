#include "pch.h"
#include "BackTrader.h"

#include "JLib/Arena.h"
#include "JLib/ArrayUtils.h"
#include "JLib/VectorUtils.h"

namespace jv::bt
{
	void* Alloc(const uint32_t size)
	{
		return malloc(size);
	}
	void Free(void* ptr)
	{
		return free(ptr);
	}

	void Portfolio::Copy(const Portfolio& other)
	{
		liquidity = other.liquidity;
		memcpy(stocks.ptr, other.stocks.ptr, sizeof(uint32_t) * stocks.length);
	}

	float BackTrader::RunTestEpochs(Arena& arena, Arena& tempArena, const TestInfo& testInfo) const
	{
		RunInfo runInfo{};
		runInfo.bot = testInfo.bot;
		runInfo.preProcessBot = testInfo.preProcessBot;
		runInfo.userPtr = testInfo.userPtr;
		runInfo.warmup = testInfo.warmup;
		Log log;

		float average = 0;

		for (uint32_t i = 0; i < testInfo.epochs; ++i)
		{
			const auto scope = arena.CreateScope();
			const auto tempScope = tempArena.CreateScope();
			auto portfolio = CreatePortfolio(arena, *this);
			
			portfolio.liquidity = testInfo.liquidity;
			runInfo.offset = testInfo.length + rand() % testInfo.maxOffset;
			runInfo.length = testInfo.length;

			const auto endPortfolio = Run(arena, tempArena, portfolio, log, runInfo);
			const float startLiquidity = GetLiquidity(portfolio, runInfo.offset);
			const float delta = GetLiquidity(endPortfolio, runInfo.offset - runInfo.length) - startLiquidity;
			average += delta / startLiquidity;

			tempArena.DestroyScope(tempScope);
			arena.DestroyScope(scope);
		}

		// also debug volatility
		return average / testInfo.epochs;
	}

	Portfolio BackTrader::Run(Arena& arena, Arena& tempArena, const Portfolio& portfolio, Array<Array<Call>>& outLog, const RunInfo& runInfo) const
	{
		const auto tempScope = tempArena.CreateScope();

		auto cpyPortfolio = CreatePortfolio(arena, *this);
		cpyPortfolio.Copy(portfolio);

		auto calls = CreateVector<Call>(tempArena, portfolio.stocks.length);
		outLog = CreateArray<Array<Call>>(arena, runInfo.length);
		
		if (runInfo.preProcessBot)
			runInfo.preProcessBot(tempArena, world, runInfo.offset + runInfo.warmup, runInfo.length, runInfo.userPtr);

		// Make sure the algorithm, if temporal, gets a warming up.
		for (uint32_t i = 0; i < runInfo.warmup; i++)
		{
			const uint32_t index = runInfo.offset - i - runInfo.warmup;
			calls.Clear();
			runInfo.bot(tempArena, world, cpyPortfolio, calls, index, runInfo.userPtr);
		}

		for (uint32_t i = 0; i < runInfo.length; ++i)
		{
			const uint32_t index = runInfo.offset - i;
			calls.Clear();
			runInfo.bot(tempArena, world, cpyPortfolio, calls, index, runInfo.userPtr);
			if (calls.count == 0)
				continue;

			const auto arr = CreateArray<Call>(arena, calls.count);
			memcpy(arr.ptr, calls.ptr, sizeof(Call) * calls.count);
			outLog[i] = arr;

			for (const auto& call : arr)
			{
				const auto close = world.timeSeries[call.symbolId].close[index];
				auto& stock = cpyPortfolio.stocks[call.symbolId];
				assert(world.timeSeries[call.symbolId].length > index);

				const float fee = world.fee * close * call.amount;
				cpyPortfolio.liquidity -= fee;
				assert(cpyPortfolio.liquidity > -1e-5f);

				switch (call.type)
				{
					case CallType::Buy: 
						stock += call.amount;
						cpyPortfolio.liquidity -= close * call.amount;
						break;
					case CallType::Sell:
						assert(stock >= call.amount);
						stock -= call.amount;
						cpyPortfolio.liquidity += close * call.amount;
						break;
					default: 
						;
				}
				
				assert(cpyPortfolio.liquidity > -1e-5f);
			}
		}

		tempArena.DestroyScope(tempScope);
		return cpyPortfolio;
	}

	float BackTrader::GetLiquidity(const Portfolio& portfolio, const uint32_t offset) const
	{
		float liquidity = portfolio.liquidity;
		for (uint32_t i = 0; i < portfolio.stocks.length; ++i)
			if(portfolio.stocks[i] > 0)
				liquidity += world.timeSeries[i].close[offset] * portfolio.stocks[i];
		return liquidity;
	}

	void BackTrader::PrintAdvice(Arena& arena, Arena& tempArena, const Bot bot, 
		const char* portfolioName, const bool apply, void* userPtr, const PreProcessBot preProcessBot) const
	{
		const auto portfolio = LoadPortfolio(arena, *this, portfolioName);
		RunInfo runInfo{};
		runInfo.bot = bot;
		runInfo.preProcessBot = preProcessBot;
		runInfo.userPtr = userPtr;
		Log log;
		const auto newPortfolio = Run(arena, tempArena, portfolio, log, runInfo);
		
		uint32_t i = 0;
		for (auto& calls : log)
		{
			std::cout << "day " << ++i << ":" << std::endl;
			for (const auto& call : calls)
			{
				if (call.type == CallType::Buy)
					std::cout << "buy " << symbols[call.symbolId] << " x " << call.amount << std::endl;
				else if(call.type == CallType::Sell)
					std::cout << "sell " << symbols[call.symbolId] << " x " << call.amount << std::endl;
			}
		}

		if (apply)
			SavePortfolio(portfolioName, newPortfolio);
	}

	float GetMA(const float* data, const uint32_t index, const uint32_t length)
	{
		float ret = 0;
		for (uint32_t i = 0; i < length; ++i)
			ret += data[index + i];
		return ret / static_cast<float>(length);
	}

	void Normalize(const float* src, float* dst, const uint32_t index, const uint32_t length)
	{
		float ret = 0;
		for (uint32_t i = 0; i < length; ++i)
			ret += src[index + i];
		ret /= static_cast<float>(length);
		memcpy(dst, &src[index], sizeof(float) * length);
		for (uint32_t i = 0; i < length; ++i)
			dst[i] /= ret;
	}

	Portfolio CreatePortfolio(Arena& arena, const BackTrader& backTrader)
	{
		Portfolio portfolio{};
		portfolio.stocks = CreateArray<uint32_t>(arena, backTrader.world.timeSeries.length);
		return portfolio;
	}

	std::string GetPortfolioPath(const char* name)
	{
		const std::string postfix = ".port";
		return name + postfix;
	}

	Portfolio LoadPortfolio(Arena& arena, const BackTrader& backTrader, const char* name)
	{
		const std::string path = GetPortfolioPath(name);
		std::ifstream fin(path);
		std::string line;
		assert(fin.good());

		auto portfolio = CreatePortfolio(arena, backTrader);

		getline(fin, line);
		portfolio.liquidity = std::stof(line);

		uint32_t i = 0;
		while(getline(fin, line))
			portfolio.stocks[i++] = std::stoi(line);

		return portfolio;
	}

	void DestroyPortfolio(Arena& arena, const Portfolio& portfolio)
	{
		DestroyArray<uint32_t>(arena, portfolio.stocks);
	}

	void SavePortfolio(const char* name, const Portfolio& portfolio)
	{
		const std::string path = GetPortfolioPath(name);
		std::ofstream fout(path);

		fout << portfolio.liquidity << std::endl;
		for (const auto& stock : portfolio.stocks)
			fout << stock << std::endl;

		fout.close();
	}

	BackTrader CreateBackTrader(Arena& arena, Arena& tempArena, const Array<const char*>& symbols, const float fee)
	{
		BackTrader backTrader{};
		backTrader.scope = arena.CreateScope();
		backTrader.world = {};
		backTrader.world.timeSeries = CreateArray<TimeSeries>(arena, symbols.length);
		backTrader.world.fee = fee;
		backTrader.tracker = {};
		backTrader.symbols = CreateArray<const char*>(arena, symbols.length);
		memcpy(backTrader.symbols.ptr, symbols.ptr, sizeof(const char*) * symbols.length);

		auto& tracker = backTrader.tracker;

		for (uint32_t i = 0; i < symbols.length; ++i)
		{
			const auto tempScope = tempArena.CreateScope();
			const auto str = tracker.GetData(tempArena, symbols[i]);
			const auto timeSeries = tracker.ConvertDataToTimeSeries(arena, str);
			backTrader.world.timeSeries[i] = timeSeries;
			tempArena.DestroyScope(tempScope);
		}
		
		return backTrader;
	}

	void DestroyBackTrader(const BackTrader& backTrader, Arena& arena)
	{
		arena.DestroyScope(backTrader.scope);
	}

	BackTraderEnvironment CreateBTE(const char** symbols, const uint32_t symbolsLength, const float fee)
	{
		BackTraderEnvironment bte{};

		Array<const char*> symbolsCpy{};
		symbolsCpy.ptr = symbols;
		symbolsCpy.length = symbolsLength;

		ArenaCreateInfo arenaCreateInfo{};
		arenaCreateInfo.alloc = Alloc;
		arenaCreateInfo.free = Free;
		bte.arena = Arena::Create(arenaCreateInfo);
		bte.tempArena = Arena::Create(arenaCreateInfo);
		bte.backTrader = CreateBackTrader(bte.arena, bte.tempArena, symbolsCpy, fee);
		return bte;
	}

	void DestroyBTE(const BackTraderEnvironment& bte)
	{
		Arena::Destroy(bte.tempArena);
		Arena::Destroy(bte.arena);
	}
}
