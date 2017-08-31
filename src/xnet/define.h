#include <uv.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>


#define MAX_GAME_CONN			(32)				// ���������ڵ�����
#define GAME_RECV_BUFFER		(160*1024*1024)		// �������ڵ�֮�䴫�����ݵ�����
#define GAME_SEND_BUFFER		(160*1024*1024)
#define CLIENT_RECV_BUFFER		(8*1024)			// �ͻ��˺ͷ������������ݵ�����
#define CLIENT_SEND_BUFFER		(8*1024*1024)

#define MASTER_GAME_ID			(0)
#define MAX_CLIENT_PACKAGE_LEN	(16*1024)
#define IP_ADDRESS_LEN			(20)
#define MAX_PACKAGE_SEC			(500)
#define MAX_TOOMANY_COUNT		(5)
#define MAX_TOOMANY_RESET		(60*1000000)


#define safe_delete(x) if((x) != 0) { free(x); (x) = 0; }

typedef unsigned char byte;
typedef unsigned int uint;

