#include "xnet.h"

#define MAX_GAME_CONN			(32)				// 服务器最大节点数量
#define GAME_BUFF_SIZE			(160*1024*1024)		// 服务器节点之间传递数据的限制
#define GAME_BUFF_INIT			(10240)				// 初始大小

#define MASTER_GAME_ID			(0)
#define MAX_CLIENT_PACKAGE_LEN	(16*1024)
#define IP_ADDRESS_LEN			(20)
#define MAX_PACKAGE_SEC			(500)
#define MAX_TOOMANY_COUNT		(5)
#define MAX_TOOMANY_RESET		(60*1000000)

struct _GameConn
{
	byte id;				// gsid
	uv_tcp_t* handle;		// 链接句柄

	uv_write_t writereq;	// 写请求
	uv_buf_t writebuff;
	
	uv_buf_t readbuff;		// 读数据的缓冲区
	uint readsize;			// 缓冲区的有效长度
};
typedef struct _GameConn GameConn;


static byte g_GameID = 0;
static uv_tcp_t g_GameServer;
static GameConn* g_Games[MAX_GAME_CONN];

byte AllocGameID()
{
	for (byte i = 0; i < g_GameID; i++)
	{
		if (!g_Games[i])
		{
			return i;
		}
	}
	return g_GameID++;
}


void AllocGameBuff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	GameConn* game = (GameConn*)handle->data;
	if (suggested_size > game->readbuff.len - game->readsize)
	{
		char* data = malloc(suggested_size * 2);
		memcpy(data, game->readbuff.base, game->readbuff.len);
		safe_delete(game->readbuff.base);
		game->readbuff.base = data;
		game->readbuff.len = suggested_size * 2;
	}
	buf->base = game->readbuff.base + game->readsize;
	buf->len = suggested_size;
}

void OnClose(uv_handle_t* handle)
{
	safe_delete(handle);
}

void ShutdownGame(uv_shutdown_t* req, int status)
{
	GameConn* game = (GameConn*)req->handle->data;
	byte gsid = game->id;
	Log("[Game] game %d shutdown", gsid);
	g_Games[gsid] = NULL;
	req->handle->data = NULL;
	game->handle = NULL;
	safe_delete(game->readbuff.base);
	safe_delete(game->writebuff.base);
	safe_delete(game);

	uv_close((uv_handle_t*)req->handle, OnClose);
	safe_delete(req);

	// 所有关联到gsid的客户端需要断线

}

void ReadGame(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
		uv_shutdown(req, handle, ShutdownGame);
		return;
	}
	GameConn* game = (GameConn*)handle->data;
	game->readsize += nread;
	if (nread == 0 || game->readsize < GAME_CMD_SIZE) return;

	// 拆包粘包处理
	GameCmd* pack = (GameCmd*)game->readbuff.base;
	if (pack->size + GAME_CMD_SIZE < game->readsize) return;

	char* data = game->readbuff.base + GAME_CMD_SIZE;
	int ret = 0;
	if (pack->cmd == CMD_G2N_VFD2GSID)
	{
		ret = SetClient2GS(pack->vfd, pack->value);
		if (ret == 1)
		{
			GameCmd cmd;
			cmd.cmd = CMD_N2G_ADDVFD;
			cmd.size = 0;
			cmd.value = 0;
			cmd.vfd = pack->vfd;
			Send2Game(pack->value, &cmd, NULL, 0);
		}
	}
	else if (pack->cmd == CMD_G2N_SEND2VFD)
	{
		Send2Vfd(pack->vfd, data, pack->size);
	}
	else if (pack->cmd == CMD_G2N_SEND2VFDS)
	{
		// 数据区的格式：
		// 前2字节表示vfd长度
		// 中间每4个字节表示一个vfd
		// 最后是数据区
		unsigned short count = *(unsigned short*)data;
		VFD* vfd = data + sizeof(unsigned short);
		char* buff = vfd + count * sizeof(VFD);
		uint buffsize = pack->size - sizeof(unsigned short) - count * sizeof(VFD);
		for (unsigned short i = 0; i < count; i++)
		{
			Send2Vfd(*vfd, buff, buffsize);
			vfd++;
		}
	}
	// 将数据往前移动，已处理的包丢掉
	game->readsize -= pack->size - GAME_CMD_SIZE;
	memcpy(game->readbuff.base, data + pack->size, game->readsize);
}

void OnSendGame(uv_write_t* req, int status)
{
	if (status == 0) return;
	Log("[Client] uv_write_error: %s", GetUVError(status));
}

int Send2Game(byte gsid, GameCmd* cmd, const char* data, uint size)
{
	GameConn* game = g_Games[gsid];
	if (game == NULL) return 0;

	memcpy(game->writebuff.base, cmd, sizeof(GameCmd));
	if (data != NULL && size > 0)
	{
		memcpy(game->writebuff.base + sizeof(GameCmd), data, size);
	}
	int ret = uv_write(&game->writereq, game->handle, &game->writebuff, 1, OnSendGame);
	if (ret != 0)
	{
		Log("[Game] uv_write to %d error: %s", game->id, GetUVError(ret));
		return 0;
	}
	return 1;
}


// 目前是按照game的链接先后分配gsid，分配后发送到game中进行校验
void AcceptGame(uv_stream_t* stream, int status)
{
	if (status != 0 || stream != (uv_stream_t*)&g_GameServer) return;
	uv_tcp_t* handle = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	int ret = uv_tcp_init(stream->loop, handle);
	if (ret != 0)
	{
		Log("[Game] uv_tcp_init Error: %s", GetUVError(ret));
		safe_delete(handle);
		return;
	}
	ret = uv_accept(stream, (uv_stream_t*)handle);
	if (ret != 0)
	{
		Log("[Game] uv_accept Error: %s", GetUVError(ret));
		safe_delete(handle);
		return;
	}
	
	ret = uv_read_start((uv_stream_t*)handle, AllocGameBuff, ReadGame);
	if (ret != 0)
	{
		Log("[Game] uv_read_start Error: %s", GetUVError(ret));
		safe_delete(handle);
		return;
	}
	GameConn* game = (GameConn*)malloc(sizeof(GameConn));
	game->id = AllocGameID();
	game->handle = handle;
	handle->data = game;
	
	game->readbuff.base = (char*)malloc(GAME_BUFF_INIT);
	game->readbuff.len = GAME_BUFF_INIT;
	memset(game->readbuff.base, 0, GAME_BUFF_INIT);
	game->readsize = 0;
	
	game->writereq.data = game;
	game->writebuff.base = (char*)malloc(GAME_BUFF_INIT);
	game->writebuff.len = GAME_BUFF_INIT;
	memset(game->writebuff.base, 0, GAME_BUFF_INIT);
	
	g_Games[game->id] = game;
	Log("[Game]new gsid = %d", game->id);

	// 发送数据到game进行校验
	GameCmd cmd;
	cmd.cmd = CMD_N2G_SYNCGSID;
	cmd.value = game->id;
	cmd.size = 0;
	//Send2Game(game, &cmd);
}


int InitGame(const char* ip, int port)
{
	memset(g_Games, 0, sizeof(g_Games));

	int ret = uv_tcp_init(uv_default_loop(), &g_GameServer);
	if (ret != 0)
	{
		Log("[Game] uv_tcp_init Error: %s", GetUVError(ret));
		return 0;
	}

	unsigned int flags = 0;
	struct sockaddr_storage addr;
	if (uv_ip4_addr(ip, port, (struct sockaddr_in*)&addr) &&
		uv_ip6_addr(ip, port, (struct sockaddr_in6*)&addr))
	{
		Log("[Game] invalid ip address or port: %s:%d", ip, port);
		return 0;
	}
	ret = uv_tcp_bind(&g_GameServer, (struct sockaddr*)&addr, flags);
	if (ret < 0)
	{
		Log("[Game] uv_tcp_bind error: %s", ip, port, GetUVError(ret));
		return 0;
	}

	ret = uv_listen((uv_stream_t*)&g_GameServer, 128, AcceptGame);
	if (ret != 0)
	{
		Log("[Game] uv_listen error: %s", ip, port, GetUVError(ret));
		return 0;
	}

	Log("[Game] start listen at %s:%d", ip, port);
	return 1;
}

