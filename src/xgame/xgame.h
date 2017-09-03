#ifndef XGAME_H
#define XGAME_H


#include "define.h"

struct _GameConfig
{
	uint host_id;			// 服务器编号
	char work_path[256];	// 工作目录
	char log_path[256];		// 日志文件路径

	char local_ip[256];		//
	uint local_port;		// 对内端口
};
typedef struct _GameConfig GameConfig;

int InitLog(const char* file);
void Log(const char* fmt, ...);

int Connect2Net(const char* ip, uint port);


#endif