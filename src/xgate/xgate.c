
#include "xgate.h"

struct _NetConfig
{
	uint host_id;			// 服务器编号
	char work_path[256];	// 工作目录
	char log_path[256];		// 日志文件路径

	char remote_ip[32];		// 对外ip
	uint remote_port;		// 对外端口

	char local_ip[32];
	uint local_port;		// 对内端口
};
typedef struct _NetConfig NetConfig;

unsigned char g_NetID = 0;
FILE* g_LogFile = 0;
uv_loop_t* g_Loop;
NetConfig g_Config;


// 日志功能
int InitLog(const char* path)
{
	if (g_LogFile) return 0;
	// 检查日志目录是否存在
	char buf[256];
	strcpy(buf, path);
	uv_fs_t req;
	uv_fs_mkdir(g_Loop, &req, buf, 0755, NULL);
	strcat(buf, "/engine");
	uv_fs_mkdir(g_Loop, &req, buf, 0755, NULL);
	strcat(buf, "/xgate.log");
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
	static char logbuff[512];
	va_list ap;
	va_start(ap, fmt);
	vsprintf(logbuff, fmt, ap);
	va_end(ap);

	static char timestr[32];
	time_t t = time(0);
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&t));
	if (g_LogFile)
	{
		fprintf(g_LogFile, "%s (%d)%s\n", timestr, g_NetID, logbuff);
		fflush(g_LogFile);
	}
	printf("%s (%d)%s\n", timestr, g_NetID, logbuff);
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
	lua_getglobal(lua, "work_path");
	lua_getglobal(lua, "log_path");
	lua_getglobal(lua, "local_ip");
	lua_getglobal(lua, "local_port");
	lua_getglobal(lua, "remote_ip");
	lua_getglobal(lua, "remote_port");
	
	g_Config.host_id = lua_tointeger(lua, -7);
	strcpy(g_Config.work_path, lua_tostring(lua, -6));
	strcpy(g_Config.log_path, lua_tostring(lua, -5));
	strcpy(g_Config.local_ip, lua_tostring(lua, -4));
	g_Config.local_port = lua_tointeger(lua, -3);
	strcpy(g_Config.remote_ip, lua_tostring(lua, -2));
	g_Config.remote_port = lua_tointeger(lua, -1);

	lua_close(lua);
	return 1;
}

// 启动参数： xnet.exe 0
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("useage: xnet.exe netid(0-31)");
		return 0;
	}
	int idx = atoi(argv[1]);
	if (idx < 0 || idx >= 32)
	{
		printf("useage: xnet.exe netid(0-31)");
		return 0;
	}
	g_NetID = (byte)idx;
	argv = uv_setup_args(argc, argv);

	// 加载配置文件，配置文件是lua格式
	int ret = LoadConfig();
	if (ret != 1) return 0;

	// 初始化loop
	g_Loop = uv_default_loop();

	// 设置工作目录
	uv_chdir(g_Config.work_path);

	// 启动日志
	ret = InitLog(g_Config.log_path);
	if (ret != 1) return 0;

	Log("net server start! netid=%d", g_NetID);
	char buf[256] = { 0 };
	int size = 255;
	uv_cwd(buf, &size);
	Log("work path: %s", buf);

	srand(time(NULL));

	ret = InitGame(g_Config.local_ip, g_Config.local_port);
	if (ret != 1) return 0;

	ret = InitClient(g_Config.remote_ip, g_Config.remote_port);
	if (ret != 1) return 0;

	// 启动循环
	uv_run(g_Loop, UV_RUN_DEFAULT);

	if (g_LogFile)
	{
		fflush(g_LogFile);
		fclose(g_LogFile);
	}
	return 1;
}