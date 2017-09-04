#include "xgame.h"

struct _GameConn
{
	uv_connect_t req;
	uv_tcp_t* handle;		// ���Ӿ��

	uv_write_t writereq;	// д����
	uv_buf_t writebuff;

	uv_buf_t readbuff;		// �����ݵĻ�����
	uint readsize;			// ����������Ч����
};
typedef struct _GameConn GameConn;

extern byte g_GameID;
extern uv_loop_t* g_Loop;
GameConn g_Conn;



void AllocGateBuff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	if (suggested_size > g_Conn.readbuff.len - g_Conn.readsize)
	{
		char* data = malloc(suggested_size * 2);
		memcpy(data, g_Conn.readbuff.base, g_Conn.readbuff.len);
		safe_delete(g_Conn.readbuff.base);
		g_Conn.readbuff.base = data;
		g_Conn.readbuff.len = suggested_size * 2;
	}
	buf->base = g_Conn.readbuff.base + g_Conn.readsize;
	buf->len = suggested_size;
}

void OnClose(uv_handle_t* handle)
{
	safe_delete(handle);
}

void ShutdownGame(uv_shutdown_t* req, int status)
{
	Log("[Gate] game %d shutdown", g_GameID);
	req->handle->data = NULL;
	g_Conn.handle = NULL;
	safe_delete(g_Conn.readbuff.base);
	safe_delete(g_Conn.writebuff.base);

	uv_close((uv_handle_t*)req->handle, OnClose);
	safe_delete(req);

	// ���й�����gsid�Ŀͻ�����Ҫ����

}

void CloseConn2Gate()
{
	if (g_Conn.handle == NULL) return;
	uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
	uv_shutdown(req, g_Conn.handle, ShutdownGame);
	g_Conn.handle = NULL;
}

void ReadGate(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		CloseConn2Gate();
		return;
	}
	g_Conn.readsize += nread;
	if (nread == 0 || g_Conn.readsize < GAME_CMD_SIZE) return;

	// ���ճ������
	GameCmd* pack = (GameCmd*)g_Conn.readbuff.base;
	if (pack->size + GAME_CMD_SIZE < g_Conn.readsize) return;

	char* data = g_Conn.readbuff.base + GAME_CMD_SIZE;
	int ret = 0;
	switch (pack->cmd)
	{
	case CMD_N2G_SYNCGSID:
		if (pack->value != g_GameID)
		{
			Log("[Gate] gsid error! Local is %d, Gate send is %d", g_GameID, pack->value);
			CloseConn2Gate();
			return;
		}
		break;
	case CMD_N2G_ADDVFD:
		{
			
		}
		break;
	case CMD_N2G_DELVFD:

		break;
	case CMD_N2G_DATA:


	default:
		break;
	}
	// ��������ǰ�ƶ����Ѵ����İ�����
	g_Conn.readsize -= pack->size - GAME_CMD_SIZE;
	memcpy(g_Conn.readbuff.base, data + pack->size, g_Conn.readsize);
}



void OnConnectGate(uv_connect_t* req, int status)
{
	if (status != 0)
	{
		Log("[Gate]Connect failed! Error:%s", GetUVError(status));
		return;
	}
	Log("[Gate] connect to gate server");
	g_Conn.handle = req->handle;
	g_Conn.readbuff.base = (char*)malloc(GAME_BUFF_INIT);
	g_Conn.readbuff.len = GAME_BUFF_INIT;
	memset(g_Conn.readbuff.base, 0, GAME_BUFF_INIT);
	g_Conn.readsize = 0;
	g_Conn.writebuff.base = (char*)malloc(GAME_BUFF_INIT);
	g_Conn.writebuff.len = GAME_BUFF_INIT;
	memset(g_Conn.writebuff.base, 0, GAME_BUFF_INIT);
	int ret = uv_read_start((uv_stream_t*)g_Conn.handle, AllocGateBuff, ReadGate);
	if (ret != 0)
	{
		Log("[Gate] uv_read_start Error: %s", GetUVError(ret));
		safe_delete(g_Conn.handle);
		return;
	}
}





int Connect2Gate(const char* ip, uint port)
{
	struct sockaddr_in addr;
	int ret = uv_ip4_addr(ip, port, &addr);
	if (ret != 0)
	{
		Log("[Gate] invalid ip or port %s:%d Error:%s", ip, port, GetUVError(ret));
		return 0;
	}
	uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	ret = uv_tcp_init(uv_default_loop(), handle);
	if (ret != 0)
	{
		Log("[Gate] uv_tcp_init Error:%s", GetUVError(ret));
		return 0;
	}
	ret = uv_tcp_connect(&g_Conn.req, handle, &addr, OnConnectGate);
	if (ret != 0)
	{
		Log("[Gate] uv_tcp_connect Error:%s", GetUVError(ret));
		return 0;
	}
	return 1;
}