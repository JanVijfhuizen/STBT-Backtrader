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

int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_dofile(L, "Scripts/hello.lua");

	lua_getglobal(L, "fib");
	lua_pushnumber(L, 13);
	lua_call(L, 1, 1);
	int result = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	lua_close(L);

	auto stui = jv::ai::CreateSTBT();

	while (!stui.Update())
		continue;
	return 0;
}
