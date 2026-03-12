
#ifndef SOCKETUDP_H_
#define SOCKETUDP_H_

#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero
#include <math.h>
#include <arpa/inet.h>

void udp_receive_thread(void* arg);



int CreateUDPServer(int port);
int RecvUDPClient(int socket, char *buf, int bufsize, struct sockaddr_in *from,
		int *from_len);
int SendUDPClient(int socket, char *msg, int len, struct sockaddr_in * to);
int SendUDPBrocast(int socket, char* msg, int len, struct sockaddr_in* to);
int CreateUDPServerToDevice(char* eth, int len, int port);
void CloseUDPSocket(int workSockfd);

int createUdpClient(struct sockaddr_in *addr,const char* ip,const int port);
void add_multiaddr_group(int s,char* group_ip);

#endif /* SOCKETUDP_H_ */