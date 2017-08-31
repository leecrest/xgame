#include "client.h"

struct _ClientConn
{
	unsigned short id;
	uv_tcp_t* handle;
};
typedef struct _ClientConn ClientConn;

static unsigned short g_ClientID = 0;
static uv_tcp_t g_ClientServer;
static ClientConn* g_ClientClient[65535];

unsigned short AllocClientID()
{
	for (unsigned short i = 0; i < g_ClientID; i++)
	{
		if (!g_ClientClient[i])
		{
			return i;
		}
	}
	return g_ClientID++;
}


void AllocClientBuff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void OnCloseClient(uv_handle_t* handle)
{
	safe_delete(handle);
}

void ShutdownClient(uv_shutdown_t* req, int status)
{
	uv_close((uv_handle_t*)req->handle, OnCloseClient);
	safe_delete(req);
}

void ReadClient(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		if (buf->base) free(buf->base);
		uv_shutdown_t* req = malloc(sizeof(uv_shutdown_t));
		uv_shutdown(req, handle, ShutdownClient);
		return;
	}
	if (nread == 0)
	{
		if (buf->base) free(buf->base);
		return;
	}

	
	
	{
		if (buf->base) free(buf->base);
		return;
	}
}

void Send2Client(ClientConn client, uv_buf_t* buf)
{
}


void AcceptClient(uv_stream_t* stream, int status)
{
	if (status != 0 || stream != (uv_stream_t*)&g_ClientServer) return;
	uv_tcp_t* handle = malloc(sizeof(uv_tcp_t));
	int ret = uv_tcp_init(stream->loop, handle);
	if (ret != 0)
	{
		safe_delete(handle);
		return;
	}
	ret = uv_accept(stream, (uv_stream_t*)handle);
	if (ret != 0)
	{
		safe_delete(handle);
		return;
	}

	ret = uv_read_start((uv_stream_t*)handle, AllocClientBuff, ReadClient);
	if (ret != 0)
	{
		safe_delete(handle);
		return;
	}
	ClientConn* client = malloc(sizeof(ClientConn));
	client->id = AllocClientID();
	client->handle = handle;
	handle->data = client;
	g_ClientClient[client->id] = client;

	printf("[Client]new client = %d\n", client->id);
}


int InitClient(char* ip, int port)
{
	memset(g_ClientClient, 0, sizeof(g_ClientClient));

	int ret = uv_tcp_init(uv_default_loop(), &g_ClientServer);
	if (ret != 0)
	{
		printf("[%s:%d] uv_tcp_init error!\n", ip, port);
		return 0;
	}

	unsigned int flags = 0;
	struct sockaddr_storage addr;
	if (uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
		uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
	{
		printf("[%s:%d] invalid ip address or port\n", ip, port);
		return 0;
	}
	ret = uv_tcp_bind(&g_ClientServer, (struct sockaddr*)&addr, flags);
	if (ret < 0)
	{
		printf("[%s:%d] uv_tcp_bind error!\n", ip, port);
		return 0;
	}

	ret = uv_listen((uv_stream_t*)&g_ClientServer, 128, AcceptClient);
	if (ret != 0)
	{
		printf("[%s:%d] uv_listen error!\n", ip, port);
		return 0;
	}

	printf("[%s:%d] start listen\n", ip, port);
	return 1;
}