#include "xgame.h"

struct _GameConn
{
	uv_tcp_t* handle;		// 链接句柄

	uv_write_t writereq;	// 写请求
	uv_buf_t writebuff;

	uv_buf_t readbuff;		// 读数据的缓冲区
	uint readsize;			// 缓冲区的有效长度
};
typedef struct _GameConn GameConn;

extern byte g_GameID;
GameConn g_Conn;

int Connect2Net(const char* ip, uint port)
{


	return 1;
}