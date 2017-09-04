
#ifndef XNET_H
#define XNET_H

#include "define.h"

int InitLog(const char* file);
void Log(const char* fmt, ...);



int InitGame(const char* ip, uint port);
int InitClient(const char* ip, uint port);

int Send2Game(byte gsid, GameCmd* cmd, const char* data, uint size);

int SetClient2GS(VFD vfd, byte gsid);
int Send2Vfd(VFD vfd, char* buf, int size);
//int Send2Client(ClientConn* client, char* buf, int size);

#endif