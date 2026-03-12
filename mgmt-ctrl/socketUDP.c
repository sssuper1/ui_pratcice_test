#define _GNU_SOURCE
#include "socketUDP.h"
#include "errno.h"
#include <string.h>
#include <netinet/in.h>
#include "mgmt_types.h"
#include "sqlite3.h"
#include <linux/if.h>
#include <unistd.h>
#include "ui_get.h"

int CreateUDPServer(int port){

	int server_sockfd;// 创建一个UDP套接字
	struct sockaddr_in my_addr;   //服务器网络地址结构体
	memset(&my_addr, 0, sizeof(my_addr)); //数据初始化--清零
	my_addr.sin_family = AF_INET; //设置为IPv4通信
	my_addr.sin_addr.s_addr = INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
	my_addr.sin_port = htons(port); //服务器端口号

	if((server_sockfd = socket(PF_INET,SOCK_DGRAM,0)) < 0){
		perror("socket create failed");
		return -1;
	}
	
	int reuse =1;
	// 设置 Socket 选项：允许地址重用 (SO_REUSEADDR)
	if(setsockopt(server_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0){
		perror("setsockopt SO_REUSEADDR failed");
		return -1;
	}
	// 将套接字与刚才配置的网络地址强行绑定在一起
	if(bind(server_sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))<0){
		perror("bind failed");
		return -1;
	}
	return server_sockfd;
}

int CreateUDPServerToDevice(char* eth,int len,int port)
{
	int workSockfd;
	struct sockaddr_in svrAddr;

	svrAddr.sin_family=AF_INET;
	svrAddr.sin_port=htons(port);
	svrAddr.sin_addr.s_addr= INADDR_ANY;

	workSockfd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (workSockfd <= 0)
	{
		return RETURN_FAILED;
	}
	if (-1 == bind(workSockfd, (struct sockaddr*)&svrAddr, sizeof(svrAddr)))
	{
		CloseUDPSocket(workSockfd);
		return RETURN_FAILED;
	}

	if (setsockopt(workSockfd, SOL_SOCKET, SO_BINDTODEVICE, eth, len) == -1)
	{
		CloseUDPSocket(workSockfd);
		return RETURN_FAILED;
	}

	return workSockfd;
}

// 传入参数：一个空的地址结构体指针(用来装结果)、目标IP字符串、目标端口
int createUdpClient(struct sockaddr_in *addr,const char* ip,const int port){

	int s=socket(AF_INET,SOCK_DGRAM,0);

	addr->sin_family=AF_INET;
	addr->sin_port = htons(port);

	if(inet_pton(AF_INET,ip,&addr->sin_addr)<=0)
	{
		perror("invalid address\r\n");
		return -1;
	}

	return s;
}




int SendUDPClient(int scoket,char *msg,int len,struct sockaddr_in* to)
{
	int s_len;
	// 参数含义：用 socket 邮筒，把 msg 发出去，发 len 这么长，0是默认标志，发给 to 这个地址
	if((s_len = sendto(scoket,msg,len,0,(struct sockaddr*)to,sizeof(struct sockaddr)))!= len)
	{
		fprintf(stderr, "send failed: %s,ErrNo:%d\n", strerror(errno),errno);
		return -1;
	}
	return s_len;
}

// 参数和普通的 SendUDPClient 完全一样
int SendUDPBrocast(int socket, char* msg, int len, struct sockaddr_in* to)
{
    int ret;
    int broadcast = 1;
	//setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));  //ssq
    // Send the message
    ret = sendto(socket, msg, len, 0, (struct sockaddr*)to, sizeof(struct sockaddr_in));
    if (ret < 0) {
        perror("sendto() failed");
        return -1;
    }

    return 0;
}
int RecvUDPClient(int socket,char* buf,int bufsize,struct sockaddr_in* from,int* form_len)
{
	int len;
	if((len = recvfrom(socket,buf,bufsize,0,(struct sockaddr*)from,(socklen_t*)form_len))<0)
	{
		return -1;
	}

	buf[len] = '\0';
	return len;
}

void CloseUDPSocket(int workSockfd)
{
    if (workSockfd > 0)
    {
        shutdown(workSockfd, SHUT_RDWR); // 切断收发通道
        close(workSockfd);               // 彻底销毁邮筒，释放资源
    }
}

// 参数：s (已经创建好的 UDP 邮筒), group_ip (你要加入的群号，比如 "224.0.0.1")
void add_multiaddr_group(int s, char* group_ip)
{
    // 定义一个组播请求结构体 (IP Multicast Request)
    struct ip_mreq ldyw_group;
    
    // 1. 填写你想加入的“群号” (将字符串转为网络IP格式)
    ldyw_group.imr_multiaddr.s_addr = inet_addr(group_ip);
    
    // 2. 告诉系统：用我板子上的任意网卡去加这个群
    ldyw_group.imr_interface.s_addr = htonl(INADDR_ANY);

    // 3. 核心绝招：IP_ADD_MEMBERSHIP
    // 告诉底层网卡驱动：“我要加群！以后看到发给这个群号的包，请帮我收下来！”
    if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ldyw_group, sizeof(ldyw_group)) < 0) {
        printf("[CJ DEBUG] :fail to Join multicast group \n");
    } 
}