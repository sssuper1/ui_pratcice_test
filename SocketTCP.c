#include "SocketTCP.h"
#include "mgmt_transmit.h"
#include "Thread.h"

INT32 CreateTCPServer(INT16 port){
    INT32 workSockfd;
	struct sockaddr_in svrAddr;

	svrAddr.sin_family      = AF_INET;
	svrAddr.sin_addr.s_addr = INADDR_ANY;
	svrAddr.sin_port        = htons(port);

    workSockfd = socket (AF_INET , SOCK_STREAM , 0 );
    if(workSockfd <= 0)
	{
		return RETURN_FAILED;
	}
    if(-1 == bind(workSockfd,(struct sockaddr *)&svrAddr,sizeof(svrAddr)))
	{
		CloseTCPSocket(workSockfd);
		return RETURN_FAILED;
	}
    if(-1 == listen(workSockfd,10))
	{
		CloseTCPSocket(workSockfd);
		return RETURN_FAILED;
	}
	return workSockfd;

}

INT32 CreateTCPClient(INT16 port)
{
    INT32 sockfd;
    struct sockaddr_in svrAddr;

    svrAddr.sin_family      = AF_INET;
    svrAddr.sin_addr.s_addr = INADDR_ANY;
    svrAddr.sin_port        = htons(port);

    sockfd = socket (AF_INET , SOCK_STREAM , 0 );
    if(sockfd <= 0)
    {
        return RETURN_FAILED;
    }
    return sockfd;

}

INT32 ConnectServer(INT32 workSockfd,struct sockaddr_in* serAddr)
{
	int i = 5;
		//printf("4 %d\n",workSockfd);
	if(workSockfd <= 0)
		return RETURN_FAILED;
	while(-1 == connect (workSockfd, serAddr,sizeof(struct sockaddr_in)))
	{
		i --;
		ThreadSleep(500);
		if(i == 0)
		{
			return RETURN_FAILED;
		}
	}
}

INT32 SendTCPClient(INT32 workSockfd,INT8 *pSendBuf,INT32 sendDataLen)
{
	INT32 allsendlen = 0;
	INT32 thissendlen;

	while(allsendlen<sendDataLen)
	{
		thissendlen = send (workSockfd, pSendBuf+ allsendlen, sendDataLen-allsendlen, 0 );
		if(thissendlen<0)
		{
			return RETURN_FAILED;
		}
		allsendlen += thissendlen;
	}
	//printf("allsendlen = %d\n",allsendlen);
	return RETURN_OK;
}

INT32 SendTCPServer(INT32 workSockfd,INT8 *pSendBuf,INT32 sendDataLen)
{
	int allsendlen = 0;
	int thissendlen;

	while(allsendlen<sendDataLen)
	{
		thissendlen = send (workSockfd, pSendBuf+ allsendlen, sendDataLen-allsendlen, 0 );
		if(thissendlen<0)
		{
			return RETURN_FAILED;
		}
		allsendlen += thissendlen;
	}
	return RETURN_OK;
}

INT32 RecvTCPClient(INT32 workSockfd,INT8 *pRecvBuf,INT32 recvBufLen)
{
	INT32 allrecvlen = 0;
	INT32 thisrecvlen;

	while (allrecvlen<recvBufLen)
	{
		thisrecvlen = recv (workSockfd , pRecvBuf+allrecvlen , recvBufLen-allrecvlen , 0 );
		if(thisrecvlen<=0)//ssq
		{
			return RETURN_FAILED;
		}
		allrecvlen+=thisrecvlen;
	}
	return RETURN_OK;
}

INT32 OnlineMonitor(INT32 listenSockd,struct sockaddr_in* clientAddr)
{
    INT32 newSockfd;
	socklen_t clientAddrSize = sizeof(struct sockaddr_in);
	newSockfd = accept(listenSockd,(struct sockaddr*)clientAddr,&clientAddrSize);
	if(newSockfd > 0)
	{
		return newSockfd;
	}
	else{
		return 0;
	}


}

INT32 RecvTCPServer(INT32 workSockfd,INT8 *pRecvBuf,INT32 recvBufLen)
{
    INT32 allrecvlen = 0;
	INT32 thisrecvlen;

	while (allrecvlen<recvBufLen)
	{
		thisrecvlen = recv (workSockfd , pRecvBuf+allrecvlen , recvBufLen-allrecvlen , 0 );
		if(thisrecvlen<0)
		{
			return RETURN_FAILED;
		}
		allrecvlen+=thisrecvlen;
	}
	return RETURN_OK;
}

void CloseTCPSocket(INT32 workSockfd)
{
    if(workSockfd > 0)
	{
		shutdown(workSockfd,SHUT_RDWR);
		close(workSockfd);
	}
}