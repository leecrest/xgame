#include "xnet.h"

#define READ_BUFF_SIZE	1024		// 读取来自客户端的请求的缓冲区大小
#define WRITE_BUFF_SIZE (10*1024)	// 发送到客户端的数据的缓冲区大小

#define CLIENT_PACK_HEAD 2			// 包头，2字节表现包体的长度
#define CLIENT_PACK_MIN 3			// 包体最小长度

struct _ClientConn
{
	VFD vfd;
	byte gsid;
	int seed;
	byte seedsend;
	uv_tcp_t* handle;
	
	uv_write_t writereq;
	uv_buf_t writebuff;

	uv_buf_t readbuff;		// 读数据的缓冲区
	uint readsize;			// 缓冲区的有效长度
};
typedef struct _ClientConn ClientConn;

static uv_tcp_t g_Server;
static ClientConn* g_Clients[MAX_VFD];
static uint g_ClientSize = 0;
static uint g_CurVfdIdx = 0;
extern unsigned char g_NetID;


void SendClientSeed(ClientConn* client)
{
	char buf[5];
	buf[0] = 4;
	*(int*)(buf + 1) = client->seed;
	Send2Client(client, buf, 5);
}

void Encode(int seed, byte* buf, uint len)
{
	byte tmp = 0;
	for (uint i = 0; i < len; i++)
	{
		tmp = buf[i];
		buf[i] ^= seed;
		seed = (seed + (tmp & 0x03));
	}
}

void Decode(int seed, byte* buf, uint len)
{
	byte tmp = 0;
	for (uint i = 0; i < len; i++)
	{
		buf[i] ^= seed;
		tmp = buf[i];
		seed = (seed + (tmp & 0x03));
	}
}

VFD AllocVfd()
{
	if (g_ClientSize >= MAX_VFD)
	{
		return 0;
	}

	do
	{
		uint idx = (g_CurVfdIdx++ % MAX_VFD) + 1;
		if (!g_Clients[idx])
		{
			return idx | (g_NetID << (VFD_BITS - NID_BIT));
		}
	} while (1);
	return 0;
}

void AllocClientBuff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	ClientConn* client = (ClientConn*)handle->data;
	if (suggested_size > client->readbuff.len - client->readsize)
	{
		char* data = malloc(suggested_size * 2);
		memcpy(data, client->readbuff.base, client->readbuff.len);
		safe_delete(client->readbuff.base);
		client->readbuff.base = data;
		client->readbuff.len = suggested_size * 2;
	}
	buf->base = client->readbuff.base + client->readsize;
	buf->len = suggested_size;
}

void OnCloseClient(uv_handle_t* handle)
{
	safe_delete(handle);
}

void ShutdownClient(uv_shutdown_t* req, int status)
{
	ClientConn* client = (ClientConn*)req->handle->data;
	VFD vfd = client->vfd;
	byte gsid = client->gsid;
	int idx = VFD2IDX(vfd);
	g_Clients[idx] = NULL;
	req->handle->data = NULL;
	client->handle = NULL;
	safe_delete(client->readbuff.base);
	safe_delete(client->writebuff.base);
	safe_delete(client);

	uv_close((uv_handle_t*)req->handle, OnCloseClient);
	safe_delete(req);

	// 通知game，链接断开了
	GameCmd pack;
	pack.cmd = CMD_N2G_DELVFD;
	pack.vfd = vfd;
	pack.value = 0;
	pack.size = 0;
	Send2Game(gsid, &pack, NULL, 0);
}

void ReadClient(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
		uv_shutdown(req, handle, ShutdownClient);
		return;
	}
	ClientConn* client = (ClientConn*)handle->data;
	client->readsize += nread;
	if (nread == 0 || client->readsize < CLIENT_PACK_MIN) return;

	// 拆包粘包处理，客户端的数据包定义：2字节长度 + 数据
	unsigned short len = *(unsigned short*)client->readbuff.base;
	if (client->readsize - CLIENT_PACK_HEAD < len) return;

	char* data = client->readbuff.base + CLIENT_PACK_HEAD;
	// 上行数据需要解密
	Decode(client->seed, (byte*)data, len);

	GameCmd pack;
	pack.cmd = CMD_N2G_DATA;
	pack.vfd = client->vfd;
	pack.value = 0;
	pack.size = len;
	Send2Game(client->gsid, &pack, data, len);

	// 将数据往前移动，已处理的包丢掉
	client->readsize -= CLIENT_PACK_HEAD + len;
	memcpy(client->readbuff.base, data + len + 2, client->readsize);
}





void OnSendClient(uv_write_t* req, int status)
{
	if (status == 0) return;
	Log("[Client] uv_write_error: %s", GetUVError(status));
}

int Send2Vfd(VFD vfd, char* buf, int size)
{
	ClientConn* client = g_Clients[VFD2IDX(vfd)];
	if (client == NULL) return 0;
	return Send2Client(client, buf, size);
}

int Send2Client(ClientConn* client, char* buf, int size)
{
	if (client == NULL) return 0;
	memcpy(client->writebuff.base, buf, size);
	int ret = uv_write(&client->writereq, client->handle, &client->writebuff, 1, OnSendClient);
	if (ret != 0)
	{
		Log("[Client] uv_write to %d error: %s", client->vfd, GetUVError(ret));
		return 0;
	}
	return 1;
}


int SetClient2GS(VFD vfd, byte gsid)
{
	uint idx = VFD2IDX(vfd);
	ClientConn* client = g_Clients[idx];
	if (client == NULL) return 0;
	if (client->gsid == gsid) return 0;
	client->gsid = gsid;
	return 1;
}


void AcceptClient(uv_stream_t* stream, int status)
{
	if (status != 0 || stream != (uv_stream_t*)&g_Server) return;
	uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
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
	ClientConn* client = (ClientConn*)malloc(sizeof(ClientConn));
	client->vfd = AllocVfd();
	client->gsid = 0;
	client->seed = rand();
	client->seedsend = 0;
	client->handle = handle;
	handle->data = client;

	client->writereq.data = client;
	client->writebuff.base = malloc(WRITE_BUFF_SIZE);
	client->writebuff.len = WRITE_BUFF_SIZE;
	memset(client->writebuff.base, 0, WRITE_BUFF_SIZE);

	client->readbuff.base = malloc(READ_BUFF_SIZE);
	client->readbuff.len = READ_BUFF_SIZE;
	memset(client->readbuff.base, 0, READ_BUFF_SIZE);
	client->readsize = 0;

	int idx = VFD2IDX(client->vfd);
	g_Clients[idx] = client;
	Log("[Client]new client = %d", client->vfd);
	
	// 发送随机种子
	SendClientSeed(client);
}


int InitClient(const char* ip, int port)
{
	memset(g_Clients, 0, sizeof(g_Clients));

	int ret = uv_tcp_init(uv_default_loop(), &g_Server);
	if (ret != 0)
	{
		Log("[Client] uv_tcp_init error: %s", ip, port, GetUVError(ret));
		return 0;
	}

	unsigned int flags = 0;
	struct sockaddr_storage addr;
	if (uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
		uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
	{
		Log("[Client] invalid ip address or port: %s:%d", ip, port);
		return 0;
	}
	ret = uv_tcp_bind(&g_Server, (struct sockaddr*)&addr, flags);
	if (ret < 0)
	{
		Log("[Client] uv_tcp_bind error: %s", GetUVError(ret));
		return 0;
	}

	ret = uv_listen((uv_stream_t*)&g_Server, 128, AcceptClient);
	if (ret != 0)
	{
		Log("[Client] uv_listen error: %s", GetUVError(ret));
		return 0;
	}

	Log("[Client] start listen at %s:%d", ip, port);
	return 1;
}