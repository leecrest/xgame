
#include <uv.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#define MAX_HOST_CONN			(32)
#define GAME_RECV_BUFFER		(160*1024*1024)
#define GAME_SEND_BUFFER		(160*1024*1024)
#define CLIENT_RECV_BUFFER		(8*1024)
#define CLIENT_SEND_BUFFER		(8*1024*1024)

#define MASTER_GSID				(0)
#define MAX_CLIENT_PACKAGE_LEN	(16*1024)
#define IP_ADDRESS_LEN			(20)
#define MAX_PACKAGE_SEC			(500)
#define MAX_TOOMANY_COUNT		(5)
#define MAX_TOOMANY_RESET		(60*1000000)




int init_xnet();
int init_host(char* ip, int port);
int init_client();