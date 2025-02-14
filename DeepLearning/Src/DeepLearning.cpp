#include "pch.h"

#include "JLib/ArrayUtils.h"
#include "JLib/Math.h"
#include "JLib/Queue.h"
#include <Graphics/Renderer.h>
#include <STBT.h>

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
	auto stui = jv::bt::CreateSTBT();

	while (!stui.Update())
		continue;
	return 0;
}
