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
// �ͻ������ӱ�ţ����и�2λ��ʾ�������ر��
typedef unsigned int VFD;


// �ڵ�֮���ͨ�Ÿ�ʽ
// ��������
#define CMD_N2G_SYNCGSID	0x01	// һ����net���͸�game��ͬ��gameid
#define CMD_G2N_VFD2GSID	0x02	// �ı�vfd��gsid��ӳ�䣬���յ���Э�齫ֱ�ӷ��͵�gsid��
#define CMD_G2N_SEND2VFD	0x03	// �������ݸ�vfd
#define CMD_G2N_SEND2VFDS	0x04	// ���͹㲥����
#define CMD_N2G_ADDVFD		0x05	// ֪ͨgame�����µ�������Ҫ����
#define CMD_N2G_DELVFD		0x06	// ֪ͨgame�����ӶϿ�
#define CMD_N2G_DATA		0x07	// ֪ͨgame�������յ�����

struct _GameCmd
{
	byte cmd;		// ָ������
	VFD vfd;		// ����vfd
	byte value;		// ������ֵ
	uint size;		// �������ݳ���
};
typedef struct _GameCmd GameCmd;
#define GAME_CMD_SIZE	sizeof(GameCmd)


#define NULL_VFD		0
#define NID_BIT			2
#define VFD_BITS		(sizeof(VFD)*8)
#define MAX_VFD			65535
#define MAX_NETD		(1 << NID_BIT)

// ��vfd�л�ȡnetid
inline uint VFD2NID(VFD vfd)
{
	return (uint)(vfd >> (VFD_BITS - NID_BIT));
}

// ��ѯvfd��net�е����
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