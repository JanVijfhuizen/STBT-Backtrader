#include "pch.h"

#include "JLib/ArrayUtils.h"
#include "JLib/Math.h"
#include "JLib/Queue.h"
#include <Renderer.h>
#include <STBT.h>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

int Add(lua_State* L)
{
	int n = lua_gettop(L);

	double sum = 0;
	int i;

	for (i = 1; i <= n; i++)
	{
		sum += lua_tonumber(L, i);
	}

	lua_pushnumber(L, sum);
	return 1;
}

int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_dofile(L, "Scripts/hello.lua");

	lua_register(L, "cAdd", Add);

	lua_getglobal(L, "fib");
	lua_pushnumber(L, 13);
	lua_pushnumber(L, 17);
	lua_call(L, 2, 1);
	int result = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_close(L);

	auto stui = jv::bt::CreateSTBT();

	while (!stui.Update())
		continue;
	return 0;
}
