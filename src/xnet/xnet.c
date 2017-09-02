
#include "xnet.h"


/*
启动附加参数：
1、网关编号：传入 g_NetID

配置文件的参数：
1、workdir：网关的工作目录
2、log：日志文件路径
3、lanhost/lanport：监听服务器节点的ip和端口
4、wanhost/wanport：监听客户端链接的ip和端口
*/


unsigned char g_NetID = 0;
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
	
	fprintf(g_LogFile, "%s (%d)%s\n", timestr, g_NetID, logbuff);
	fflush(g_LogFile);
	printf("%s (%d)%s\n", timestr, g_NetID, logbuff);
}




int main(int argc, char* argv[] ) {
	argv = uv_setup_args(argc, argv);

	// 初始化loop
	uv_loop_t* loop = uv_default_loop();

	// 启动日志
	int ret = InitLog("xnet.log");
	if (ret != 1) return 0;

	ret = InitGame("127.0.0.1", 12345);
	if (ret != 1) return 0;

	ret = InitClient("127.0.0.1", 12346);
	if (ret != 1) return 0;

	// 启动循环
	uv_run(loop, UV_RUN_DEFAULT);
	return 1;
}