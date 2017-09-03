#ifndef XGAME_H
#define XGAME_H


#include "define.h"

struct _GameConfig
{
	uint host_id;			// ���������
	char work_path[256];	// ����Ŀ¼
	char log_path[256];		// ��־�ļ�·��

	char local_ip[256];		//
	uint local_port;		// ���ڶ˿�
};
typedef struct _GameConfig GameConfig;

int InitLog(const char* file);
void Log(const char* fmt, ...);

int Connect2Net(const char* ip, uint port);


#endif