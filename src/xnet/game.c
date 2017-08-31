#include "game.h"


struct _GameConn
{
	unsigned short id;
	uv_tcp_t* handle;
};
typedef struct _GameConn GameConn;


// 节点之间的通信格式
// 操作类型
#define CMD_N2G_SETGSID		0x01	// 一般由net发送给game，同步gameid
#define CMD_G2N_VFD2GSID	0x02	// 改变vfd到gsid的映射，新收到的协议将直接发送到gsid中
#define CMD_G2N_SEND2VFD	0x03	// 发送数据给vfd
#define CMD_G2N_SEND2VFDS	0x04	// 发送广播数据
struct _GameCmd
{
	unsigned char cmd;		// 指令类型
	unsigned short gsid;	// 消息来源
	char* data;
};
typedef struct _GameCmd GameCmd;

static unsigned short g_GameID = 0;
static uv_tcp_t g_GameServer;
static GameConn* g_GameClient[MAX_GAME_CONN];

unsigned short AllocGameID()
{
	for (unsigned short i = 0; i < g_GameID; i++)
	{
		if (!g_GameClient[i])
		{
			return i;
		}
	}
	return g_GameID++;
}


void AllocGameBuff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	buf->base = (char*)malloc(suggested_size);
	buf->len = suggested_size;
}

void OnClose(uv_handle_t* handle)
{
	safe_delete(handle);
}

void ShutdownGame(uv_shutdown_t* req, int status)
{
	uv_close((uv_handle_t*)req->handle, OnClose);
	safe_delete(req);
}

void ReadGame(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		if (buf->base) free(buf->base);
		uv_shutdown_t* req = malloc(sizeof(uv_shutdown_t));
		uv_shutdown(req, handle, ShutdownGame);
		return;
	}
	if (nread == 0)
	{
		if (buf->base) free(buf->base);
		return;
	}
	
	GameCmd* cmd = (GameCmd*)buf->base;
	if (cmd->cmd == CMD_G2N_VFD2GSID)
	{
		
	}
	else if (cmd->cmd == CMD_G2N_SEND2VFD)
	{

	}
	else if (cmd->cmd == CMD_G2N_SEND2VFDS)
	{

	}
	else
	{
		if (buf->base) free(buf->base);
		return;
	}
}

void Send2Game(GameConn game, uv_buf_t* buf)
{
}


void AcceptGame(uv_stream_t* stream, int status)
{
	if (status != 0 || stream != (uv_stream_t*)&g_GameServer) return;
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

	ret = uv_read_start((uv_stream_t*)handle, AllocGameBuff, ReadGame);
	if (ret != 0)
	{
		safe_delete(handle);
		return;
	}
	GameConn* client = malloc(sizeof(GameConn));
	client->id = AllocGameID();
	client->handle = handle;
	handle->data = client;
	g_GameClient[client->id] = client;

	GameCmd cmd;
	cmd.gsid = client->id;
	cmd.cmd = CMD_N2G_SETGSID;
	printf("[Game]new gsid = %d\n", client->id);
}


int InitGame(char* ip, int port)
{
	memset(g_GameClient, 0, sizeof(g_GameClient));

	int ret = uv_tcp_init(uv_default_loop(), &g_GameServer);
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
	ret = uv_tcp_bind(&g_GameServer, (struct sockaddr*)&addr, flags);
	if (ret < 0)
	{
		printf("[%s:%d] uv_tcp_bind error!\n", ip, port);
		return 0;
	}

	ret = uv_listen((uv_stream_t*)&g_GameServer, 128, AcceptGame);
	if (ret != 0)
	{
		printf("[%s:%d] uv_listen error!\n", ip, port);
		return 0;
	}

	printf("[%s:%d] start listen\n", ip, port);
	return 1;
}

