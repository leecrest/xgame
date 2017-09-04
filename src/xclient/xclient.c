#include "define.h"


struct _GameConn
{
	uv_connect_t req;
	uv_tcp_t* handle;		// 链接句柄

	uv_write_t writereq;	// 写请求
	uv_buf_t writebuff;

	uv_buf_t readbuff;		// 读数据的缓冲区
	uint readsize;			// 缓冲区的有效长度

	int seed;
};
typedef struct _GameConn GameConn;

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
	req->handle->data = NULL;
	g_Conn.handle = NULL;
	safe_delete(g_Conn.readbuff.base);
	safe_delete(g_Conn.writebuff.base);

	uv_close((uv_handle_t*)req->handle, OnClose);
	safe_delete(req);

	// 所有关联到gsid的客户端需要断线

}

void CloseConn2Gate()
{
	if (g_Conn.handle == NULL) return;
	uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
	uv_shutdown(req, g_Conn.handle, ShutdownGame);
	g_Conn.handle = NULL;
}

void ParsePack()
{
	// 拆包粘包处理，客户端的数据包定义：2字节长度 + 数据
	/*unsigned short len = *(unsigned short*)g_Conn.readbuff.base;
	if (g_Conn.readsize - CLIENT_PACK_HEAD < len) return;

	char* data = client->readbuff.base + CLIENT_PACK_HEAD;
	// 上行数据需要解密
	Decode(client->seed, (byte*)data, len);

	// 将数据往前移动，已处理的包丢掉
	g_Conn.readsize -= pack->size - GAME_CMD_SIZE;
	memcpy(g_Conn.readbuff.base, data + pack->size, g_Conn.readsize);*/
}

void ReadGate(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		CloseConn2Gate();
		return;
	}
	g_Conn.readsize += nread;
	if (nread == 0 || g_Conn.readsize < 3) return;

	if (g_Conn.seed <= 0)
	{
		int len = g_Conn.readbuff.base[0];
		memcpy(&g_Conn.seed, g_Conn.readbuff.base + 1, 4);
		g_Conn.readsize -= 5;
		memcpy(g_Conn.readbuff.base, g_Conn.readbuff.base + 5, g_Conn.readsize);
	}
	ParsePack();
}





void OnConnectGate(uv_connect_t* req, int status)
{
	if (status != 0)
	{
		printf("[Gate]Connect failed! Error:%s", GetUVError(status));
		return;
	}
	printf("[Gate] connect to gate server");
	g_Conn.handle = req->handle;
	g_Conn.readbuff.base = (char*)malloc(10240);
	g_Conn.readbuff.len = 10240;
	memset(g_Conn.readbuff.base, 0, 10240);
	g_Conn.readsize = 0;
	g_Conn.writebuff.base = (char*)malloc(10240);
	g_Conn.writebuff.len = 10240;
	memset(g_Conn.writebuff.base, 0, 10240);
	int ret = uv_read_start((uv_stream_t*)g_Conn.handle, AllocGateBuff, ReadGate);
	if (ret != 0)
	{
		printf("[Gate] uv_read_start Error: %s", GetUVError(ret));
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
		printf("[Gate] invalid ip or port %s:%d Error:%s", ip, port, GetUVError(ret));
		return 0;
	}
	uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	ret = uv_tcp_init(uv_default_loop(), handle);
	if (ret != 0)
	{
		printf("[Gate] uv_tcp_init Error:%s", GetUVError(ret));
		return 0;
	}
	g_Conn.seed = 0;
	ret = uv_tcp_connect(&g_Conn.req, handle, &addr, OnConnectGate);
	if (ret != 0)
	{
		printf("[Gate] uv_tcp_connect Error:%s", GetUVError(ret));
		return 0;
	}
	return 1;
}


int main(int argc, char* argv[])
{
	uv_loop_t* loop = uv_default_loop();

	Connect2Gate("127.0.0.1", 12345);

	uv_run(loop, UV_RUN_DEFAULT);
	uv_loop_close(loop);
}