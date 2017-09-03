#include "xgame.h"
#include "engine.h"




byte g_GameID = 0;
lua_State* g_Lua;
uv_loop_t* g_Loop;
FILE* g_LogFile = 0;
GameConfig g_Config;

// 日志功能
int InitLog()
{
	if (g_LogFile) return 0;

	// 检查日志目录是否存在
	char buf[256];
	strcpy(buf, g_Config.log_path);
	uv_fs_t req;
	uv_fs_mkdir(g_Loop, &req, buf, 0755, NULL);
	strcat(buf, "/engine");
	uv_fs_mkdir(g_Loop, &req, buf, 0755, NULL);
	strcat(buf, "/xgame.log");
	g_LogFile = fopen(buf, "w");
	if (g_LogFile == NULL)
	{
		printf("[InitLog] open file error! %s", buf);
		return 0;
	}
	return 1;
}

void Log(const char* fmt, ...)
{
	static char logbuff[1024];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(logbuff, fmt, ap);
	va_end(ap);

	static char timestr[32];
	time_t t = time(0);
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&t));
	if (g_LogFile)
	{
		fprintf(g_LogFile, "%s (%d)%s\n", timestr, g_GameID, logbuff);
		fflush(g_LogFile);
	}
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


int LoadConfig()
{
	lua_State* lua = luaL_newstate();
	if (lua == NULL) {
		Log("luaL_newstate has failed");
		return 0;
	}
	// 加载基础库
	luaL_openlibs(lua);
	// 加载配置文件
	int ret = luaL_dofile(lua, "xgame.cfg");
	if (ret)
	{
		Log("xgame.cfg error! %s", lua_tostring(lua, -1));
		return 0;
	}
	lua_getglobal(lua, "host_id");
	lua_getglobal(lua, "log_path");
	lua_getglobal(lua, "work_path");
	lua_getglobal(lua, "local_ip");
	lua_getglobal(lua, "local_port");
	g_Config.host_id = lua_tointeger(lua, -5);
	strcpy(g_Config.log_path, lua_tostring(lua, -4));
	strcpy(g_Config.work_path, lua_tostring(lua, -3));
	strcpy(g_Config.local_ip, lua_tostring(lua, -2));
	g_Config.local_port = lua_tointeger(lua, -1);

	lua_close(lua);
	return 1;
}

// 启动参数： xgame.exe 0
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("useage: xgame.exe gameid(0-31)");
		return 0;
	}
	int idx = atoi(argv[1]);
	if (idx < 0 || idx >= 32)
	{
		printf("useage: xgame.exe gameid(0-31)");
		return 0;
	}
	g_GameID = (byte)idx;
	argv = uv_setup_args(argc, argv);

	// 加载配置文件
	int ret = LoadConfig();
	if (ret != 1) return;

	// 初始化loop
	g_Loop = uv_default_loop();
	// Lua虚拟机初始化
	g_Lua = luaL_newstate();
	if (g_Lua == NULL) {
		Log("luaL_newstate has failed\n");
		return 1;
	}
	// 加载基础库
	luaL_openlibs(g_Lua);

	// 加载内建库
	const luaL_Reg libs[] = {
		{ "engine", luaopen_engine },
		{ NULL, NULL },
	};
	for (const luaL_Reg* lib = libs; lib->func; lib++) {
		luaL_requiref(g_Lua, lib->name, lib->func, 1);
		lua_pop(g_Lua, 1);
	}

	// 设置工作目录
	uv_chdir(g_Config.work_path);

	// 启动日志
	ret = InitLog();
	if (ret != 1) return 0;

	Log("game server start! host=%d, gsid=%d", g_Config.host_id, g_GameID);

	// 和网关通信
	ret = Connect2Net(g_Config.local_ip, g_Config.local_port);
	if (ret != 1) return 0;

	// 加载框架层脚本
	ret = ExecFile("base/preload.lua");
	if (ret != 1) return 0;

	// 启动循环
	uv_run(g_Loop, UV_RUN_DEFAULT);

	luaL_dostring(g_Lua, "engine.stop()");
	lua_close(g_Lua);
	g_Lua = NULL;
	uv_loop_close(g_Loop);
	g_Loop = NULL;
	return 1;
}