#include "xgame.h"

struct _GameConn
{
	uv_tcp_t* handle;		// ���Ӿ��

	uv_write_t writereq;	// д����
	uv_buf_t writebuff;

	uv_buf_t readbuff;		// �����ݵĻ�����
	uint readsize;			// ����������Ч����
};
typedef struct _GameConn GameConn;

extern byte g_GameID;
GameConn g_Conn;

int Connect2Net(const char* ip, uint port)
{


	return 1;
}