#include "xgame.h"

unsigned char g_GameID = 0;
lua_State* g_Lua;
uv_loop_t* g_Loop;
FILE* g_LogFile = 0;


// 日志功能
int InitLog(const char* file)
{
	if (g_LogFile) return 0;
	g_LogFile = fopen(file, "w");
	if (g_LogFile == NULL)
	{
		printf("[InitLog] open file error! %s", file);
		return 0;
	}
	return 1;
}

void Log(const char* fmt, ...)
{
	if (!g_LogFile) return;

	static char logbuff[512];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(logbuff, fmt, ap);
	va_end(ap);

	static char timestr[32];
	time_t t = time(0);
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&t));

	fprintf(g_LogFile, "%s (%d)%s\n", timestr, g_GameID, logbuff);
	fflush(g_LogFile);
	printf("%s (%d)%s\n", timestr, g_GameID, logbuff);
}




static char g_ErrMsgBuff[1024] = { "\0" };
void lua_stackdump_impl(lua_State*L, int iLevel) {
	lua_Debug stDebug;
	int iRet = lua_getstack(L, iLevel, &stDebug);
	if (1 != iRet) {
		return;
	}
	lua_getinfo(L, "nfSlu", &stDebug);
	lua_pop(L, 1);
	int pos = strlen(g_ErrMsgBuff);
	if (stDebug.what[0] == 'C') {
		if (stDebug.name) {
			sprintf(g_ErrMsgBuff + pos, "%4d[C] : in %s\n", iLevel, stDebug.name);
		}
		else {
			sprintf(g_ErrMsgBuff + pos, "%4d[C] :\n", iLevel);
		}
	}
	else {
		if (stDebug.name) {
			sprintf(g_ErrMsgBuff + pos, "%4d %s:%d in %s\n", iLevel, stDebug.short_src, stDebug.currentline, stDebug.name);
		}
		else {
			sprintf(g_ErrMsgBuff + pos, "%4d %s:%d\n", iLevel, stDebug.short_src, stDebug.currentline);
		}
	}
	pos = strlen(g_ErrMsgBuff);

	int iIndex = 1;
	int iType;
	const char* sName = NULL;
	const char* sType = NULL;
	while ((sName = lua_getlocal(L, &stDebug, iIndex++)) != NULL) {
		if (strcmp(sName, "_ENV") == 0 || strcmp(sName, "_G") == 0) {
			continue;
		}
		iType = lua_type(L, -1);
		if (iType == LUA_TNIL) {
			continue;
		}
		sType = lua_typename(L, iType);
		switch (iType) {
		case LUA_TBOOLEAN:
			sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "\t%s = <%s> %s\n", sName, sType, lua_toboolean(L, -1) ? "true" : "false");
			break;
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
		case LUA_TTHREAD:
		case LUA_TFUNCTION:
		case LUA_TTABLE:
			sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "\t%s = <%s> 0x%8p\n", sName, sType, lua_topointer(L, -1));
			break;
		default:
			sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "\t%s = <%s> %s\n", sName, sType, lua_tostring(L, -1));
			break;
		}
		lua_pop(L, 1);
	}
	iIndex = 1;
	while ((sName = lua_getupvalue(L, -1, iIndex++)) != NULL) {
		if (strcmp(sName, "_ENV") == 0 || strcmp(sName, "_G") == 0) {
			continue;
		}
		iType = lua_type(L, -1);
		if (iType == LUA_TNIL) {
			continue;
		}
		sType = lua_typename(L, iType);
		switch (iType) {
		case LUA_TBOOLEAN:
			sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "\t%s = <%s> %s\n", sName, sType, lua_toboolean(L, -1) ? "true" : "false");
			break;
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
		case LUA_TTHREAD:
		case LUA_TFUNCTION:
		case LUA_TTABLE:
			sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "\t%s = <%s> 0x%8p\n", sName, sType, lua_topointer(L, -1));
			break;
		default:
			sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "\t%s = <%s> %s\n", sName, sType, lua_tostring(L, -1));
			break;
		}
		lua_pop(L, 1);
	}
	lua_stackdump_impl(L, iLevel + 1);
}

const char LUA_STACKDUMP_STR[] = "\n===========[lua_stackdump]==============\n";
int lua_stackdump(lua_State *L) {
	g_ErrMsgBuff[0] = '\0';
	strcpy(g_ErrMsgBuff, LUA_STACKDUMP_STR);
	int iTop = lua_gettop(L);
	if (iTop > 0) {
		sprintf(g_ErrMsgBuff + strlen(g_ErrMsgBuff), "%s\n", lua_tostring(L, -1));
	}
	lua_stackdump_impl(L, 1);
	fprintf(stdout, "%s\n", g_ErrMsgBuff);
	//lua_getglobal(L, "debug.excepthook");
	lua_pushstring(L, g_ErrMsgBuff);
	return 1;
}



int ExecFile(const char* file)
{
	lua_pushcfunction(g_Lua, lua_stackdump);
	int func = lua_gettop(g_Lua);
	int ret = luaL_loadfile(g_Lua, file);
	if (ret)
	{
		lua_stackdump(g_Lua);
		return 0;
	}
	lua_pcall(g_Lua, 0, 0, func);
	return 1;
}



int main(int argc, char* argv[]) {
	argv = uv_setup_args(argc, argv);

	// 初始化loop
	g_Loop = uv_default_loop();

	// 启动日志
	int ret = InitLog("xgame.log");
	if (ret != 1) return 0;

	// Lua虚拟机初始化
	g_Lua = luaL_newstate();
	if (g_Lua == NULL) {
		fprintf(stderr, "luaL_newstate has failed\n");
		return 1;
	}
	// 加载基础库
	luaL_openlibs(g_Lua);

	// 加载内建库
	const luaL_Reg libs[] = {
		{ NULL, NULL },
	};
	for (const luaL_Reg* lib = libs; lib->func; lib++) {
		luaL_requiref(g_Lua, lib->name, lib->func, 1);
		lua_pop(g_Lua, 1);
	}

	// 和网关通信
	ret = Connect2Net();
	if (ret != 1) return ret;

	// 加载框架层脚本
	ret = ExecFile("base/preload.lua");
	if (ret != 1) { return ret; }

	// 启动循环
	uv_run(g_Loop, UV_RUN_DEFAULT);

	if (g_Lua == NULL) return;
	luaL_dostring(g_Lua, "lengine.stop()");
	lua_close(g_Lua);
	g_Lua = NULL;
	uv_loop_close(g_Loop);
	g_Loop = NULL;
	return 1;
}