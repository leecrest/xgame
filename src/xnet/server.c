#include <uv.h>

struct Host
{
	int idx;
	uv_tcp_t* handle;
};

static int g_host_idx = 0;
static uv_tcp_t g_server;
static struct Host g_hosts[32];

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread == 0)
	{
		free(buf->base);
		return;
	}

}

void connect_cb(uv_stream_t* stream, int status)
{
	if (status != 0 || stream != (uv_stream_t*)&g_server) return;
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

	ret = uv_read_start((uv_stream_t*)&handle, alloc_cb, read_cb);
	if (ret != 0)
	{
		free(handle);
		return;
	}
	g_host_idx++;
	struct Host host = g_hosts[g_host_idx - 1];
	host.idx = g_host_idx;
	host.handle = handle;
}


int init_server(char* host, int port)
{
	int ret = uv_tcp_init(uv_default_loop(), &g_server);
	if (ret != 0)
	{
		printf("[%s:%d] uv_tcp_init error!", host, port);
		return 0;
	}

	unsigned int flags = 0;
	struct sockaddr_storage addr;
	if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
		uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr))
	{
		printf("[%s:%d] invalid ip address or port", host, port);
		return 0;
	}
	ret = uv_tcp_bind(&g_server, (struct sockaddr*)&addr, flags);
	if (ret < 0)
	{
		printf("[%s:%d] uv_tcp_bind error!", host, port);
		return 0;
	}

	ret = uv_listen((uv_stream_t*)&g_server, 128, connect_cb);
	if (ret != 0)
	{
		printf("[%s:%d] uv_listen error!", host, port);
		return 0;
	}

	printf("[%s:%d] start listen", host, port);
	return 1;
}