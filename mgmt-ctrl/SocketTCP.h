#ifndef __SOCKETTCP_H
#define __SOCKETTCP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
//#include <error.h>
#include <arpa/inet.h>
#include "mgmt_types.h"
#include <sys/time.h>

typedef struct STcpClient {
	INT32 sockfd;
	BOOL useful;//表示该结构体是否被使用，TRUE表示正在使用，FALSE表示未使用
	INT32 srcip;//存储连接的客户端的IP地址，便于后续管理和通信
	struct timeval time;//记录连接的时间，便于实现连接超时等功能
} TcpClient;


INT32 CreateTCPServer(INT16 port);
INT32 CreateTCPClient(INT16 port);
INT32 ConnectServer(INT32 workSockfd,struct sockaddr_in* serAddr);
INT32 SendTCPClient(INT32 workSockfd,INT8 *pSendBuf,INT32 sendDataLen);
INT32 RecvTCPClient(INT32 workSockfd,INT8 *pRecvBuf,INT32 recvBufLen);
INT32 RecvTCPServer(INT32 workSockfd,INT8 *pRecvBuf,INT32 recvBufLen);
INT32 SendTCPServer(INT32 workSockfd,INT8 *pSendBuf,INT32 sendDataLen);
INT32 OnlineMonitor(INT32 listenSockd,struct sockaddr_in* clientAddr);
void CloseTCPSocket(INT32 workSockfd);
#endif // !__SOCKETTCP_H
