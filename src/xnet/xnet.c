#include "xnet.h"


struct host_conn
{
	unsigned short id;
	uv_tcp_t* handle;
};

// 节点之间的通信格式
struct host_cmd
{
	unsigned char cmd;
	unsigned short id;
};

static unsigned short g_hostid = 0;
static uv_tcp_t g_hostserver;
static struct host_conn* g_hostclient[MAX_HOST_CONN];

unsigned short alloc_hostid()
{
	for (unsigned short i = 0; i < g_hostid; i++)
	{
		if (!g_hostclient[i])
		{
			return i;
		}
	}
	return g_hostid++;
}


void alloc_host(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void onclose(uv_handle_t* handle)
{
	free(handle);
}

void shutdown_host(uv_shutdown_t* req, int status)
{
	uv_close((uv_handle_t*)req->handle, onclose);
	free(req);
}

void read_host(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		free(buf->base);
		uv_shutdown_t* req = malloc(sizeof(uv_shutdown_t));
		uv_shutdown(req, handle, shutdown_host);
		return;
	}
	if (nread == 0)
	{
		free(buf->base);
		return;
	}
	// 读取到的数据转到对应的host处理

}

void send_host(struct host_conn host, uv_buf_t* buf)
{
}


void accept_host(uv_stream_t* stream, int status)
{
	if (status != 0 || stream != (uv_stream_t*)&g_hostserver) return;
	uv_tcp_t* handle = malloc(sizeof(uv_tcp_t));
	int ret = uv_tcp_init(stream->loop, &handle);
	if (ret != 0)
	{
		free(handle);
		return;
	}
	ret = uv_accept(stream, (uv_stream_t*)&handle);
	if (ret != 0)
	{
		free(handle);
		return;
	}

	ret = uv_read_start((uv_stream_t*)&handle, alloc_host, read_host);
	if (ret != 0)
	{
		free(handle);
		return;
	}
	struct host_conn* client = malloc(sizeof(struct host_conn));
	client->id = alloc_hostid();
	client->handle = handle;
	handle->data = client;
	g_hostclient[client->id] = client;

	struct host_cmd cmd;
	cmd.id = client->id;
	cmd.cmd = 1;
}


int init_host(char* ip, int port)
{
	int ret = uv_tcp_init(uv_default_loop(), &g_hostserver);
	if (ret != 0)
	{
		printf("[%s:%d] uv_tcp_init error!", ip, port);
		return 0;
	}

	unsigned int flags = 0;
	struct sockaddr_storage addr;
	if (uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
		uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
	{
		printf("[%s:%d] invalid ip address or port", ip, port);
		return 0;
	}
	ret = uv_tcp_bind(&g_hostserver, (struct sockaddr*)&addr, flags);
	if (ret < 0)
	{
		printf("[%s:%d] uv_tcp_bind error!", ip, port);
		return 0;
	}

	ret = uv_listen((uv_stream_t*)&g_hostserver, 128, accept_host);
	if (ret != 0)
	{
		printf("[%s:%d] uv_listen error!", ip, port);
		return 0;
	}

	printf("[%s:%d] start listen", ip, port);
	return 1;
}














int init_client()
{
	return 1;
}


int init_xnet()
{
	memset(g_hostclient, 0, sizeof(g_hostclient));
	return 1;
}