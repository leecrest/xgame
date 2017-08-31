#include <uv.h>

struct client
{
	int vfd;
	uv_tcp_t* handle;
};

void alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void read_cb(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
void connect_cb(uv_stream_t* stream, int status);

// 启动来自服务节点的监听
int init_server(char* host, int port);
