#ifndef DEFINE_H
#define DEFINE_H

#define _CRT_SECURE_NO_WARNINGS


#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <io.h>
#include <process.h>

#include <uv.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

typedef unsigned char byte;
typedef unsigned int uint;
// 客户端链接编号，其中高2位表示所属网关编号
typedef unsigned int VFD;


// 节点之间的通信格式
// 操作类型
#define CMD_N2G_SYNCGSID	0x01	// 一般由net发送给game，同步gameid
#define CMD_G2N_VFD2GSID	0x02	// 改变vfd到gsid的映射，新收到的协议将直接发送到gsid中
#define CMD_G2N_SEND2VFD	0x03	// 发送数据给vfd
#define CMD_G2N_SEND2VFDS	0x04	// 发送广播数据
#define CMD_N2G_ADDVFD		0x05	// 通知game，有新的链接需要管理
#define CMD_N2G_DELVFD		0x06	// 通知game，链接断开
#define CMD_N2G_DATA		0x07	// 通知game，链接收到数据

struct _GameCmd
{
	byte cmd;		// 指令类型
	VFD vfd;		// 操作vfd
	byte value;		// 附加数值
	uint size;		// 附加数据长度
};
typedef struct _GameCmd GameCmd;
#define GAME_CMD_SIZE	sizeof(GameCmd)


#define NULL_VFD		0
#define NID_BIT			2
#define VFD_BITS		(sizeof(VFD)*8)
#define MAX_VFD			65535
#define MAX_NETD		(1 << NID_BIT)

// 从vfd中获取netid
inline uint VFD2NID(VFD vfd)
{
	return (uint)(vfd >> (VFD_BITS - NID_BIT));
}

// 查询vfd在net中的序号
inline uint VFD2IDX(VFD vfd)
{
	return (((VFD)(-1) >> NID_BIT) & vfd);
}


#define safe_delete(x) if((x) != NULL) { free(x); (x) = 0; }




static char* GetUVError(int err)
{
	static char buf[256];
	sprintf(&buf, "%s:%s", uv_err_name(err), uv_strerror(err));
	return buf;
}


#endif