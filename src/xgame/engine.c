
#include "engine.h"
extern GameConfig g_Config;

LUALIB_API int luaopen_engine(lua_State *L) {
	lua_newtable(L);
	

	return 1;
}
