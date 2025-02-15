#pragma once


/*
STBT* gSTBT;
int gId;

int gGetSymbolIndex(lua_State* L)
{
	const char* symbol = lua_tolstring(L, 0, nullptr);

	uint32_t index = 0;
	for (uint32_t i = 0; i < gSTBT->loadedSymbols.length; i++)
	{
		if (!gSTBT->enabledSymbols[i])
			continue;
		if (gSTBT->loadedSymbols[i] == symbol)
		{
			lua_pushnumber(L, index);
			return 1;
		}
		index++;
	}
	lua_pushnumber(L, -1);
	return 1;
}

int gSetIndex(lua_State* L)
{
	gId = lua_tonumber(L, 0);
	return 0;
}

int gGet(lua_State* L, float* TimeSeries::* arr)
{
	const auto& active = gSTBT->timeSeriesArr[gId];
	lua_createtable(L, active.length, 0);
	int newTable = lua_gettop(L);

	for (uint32_t i = 0; i < active.length; i++)
	{
		const auto n = (active.*arr)[i];
		lua_pushnumber(L, n);
		lua_rawseti(L, newTable, i);
	}

	return 1;
}

int gGetOpen(lua_State* L)
{
	return gGet(L, &TimeSeries::open);
}

int gGetClose(lua_State* L)
{
	return gGet(L, &TimeSeries::close);
}

int gGetHigh(lua_State* L)
{
	return gGet(L, &TimeSeries::high);
}

int gGetLow(lua_State* L)
{
	return gGet(L, &TimeSeries::low);
}

int gGetLength(lua_State* L)
{
	lua_pushnumber(L, gSTBT->timeSeriesArr[gId].length);
	return 1;
}

int gGetVolume(lua_State* L)
{
	const auto& active = gSTBT->timeSeriesArr[gId];
	lua_createtable(L, active.length, 0);
	int newTable = lua_gettop(L);

	for (uint32_t i = 0; i < active.length; i++)
	{
		const auto n = (active.volume)[i];
		lua_pushnumber(L, n);
		lua_rawseti(L, newTable, i);
	}

	return 1;
}

int gBuy(lua_State* L)
{
	return 0;
}

int gSell(lua_State* L)
{
	return 0;
}

int gGetNumInPort(lua_State* L)
{
	return 1;
}

int gGetLiquidity(lua_State* L)
{
	return 1;
}

void CloseLua(STBT& stbt)
{
	if (!stbt.L)
		return;
	lua_close(stbt.L);
	stbt.L = nullptr;
}

void OpenLua(STBT& stbt)
{
	// Needed for global functions.
	gSTBT = &stbt;
	CloseLua(stbt);
	stbt.L = luaL_newstate();
	luaL_openlibs(stbt.L);
	std::string f = "Scripts/" + stbt.activeScript + ".lua";
	luaL_dofile(stbt.L, f.c_str());
	lua_register(stbt.L, "GetSymbolId", gGetSymbolIndex);
	lua_register(stbt.L, "SetId", gSetIndex);
	lua_register(stbt.L, "GetOpen", gGetOpen);
	lua_register(stbt.L, "GetClose", gGetClose);
	lua_register(stbt.L, "GetHigh", gGetHigh);
	lua_register(stbt.L, "GetLow", gGetLow);
	lua_register(stbt.L, "GetLength", gGetLength);
	lua_register(stbt.L, "GetVolume", gGetVolume);
	lua_register(stbt.L, "Buy", gBuy);
	lua_register(stbt.L, "Sell", gSell);
	lua_register(stbt.L, "GetLiquidity", gGetLiquidity);
	lua_register(stbt.L, "GetNumInPort", gGetNumInPort);
}

static void ExecuteRun(STBT& stbt)
{
	--stbt.runsQueued;

	ImGui::Begin("Active Run", nullptr, WIN_FLAGS);
	ImGui::SetWindowPos({ 300, 225 });
	ImGui::SetWindowSize({ 200, 150 });

	ImGui::Text("Running...");

	time_t current;
	uint32_t length;
	uint32_t buffer = std::atoi(stbt.buffBuffer);
	const auto days = std::atoi(stbt.dayBuffer);
	GetMaxLength(stbt, current, length, buffer);

	if (stbt.randomizeDate)
	{
		uint32_t startOffset = rand() % (length - days);
		auto a = GetT(startOffset);
		auto b = GetT(startOffset + days);
		stbt.from = *std::gmtime(&a);
		stbt.to = *std::gmtime(&b);
	}

	stbt.liquidity = std::atof(stbt.buffArr[0]);
	for (uint32_t i = 0; i < stbt.portfolio.length; i++)
		int32_t n = std::atoi(stbt.buffArr[i + 1]);

	//lua_getglobal(stbt.L, "PRE");

	for (uint32_t i = 0; i < length - buffer; i++)
	{

	}

	ImGui::End();
}
*/