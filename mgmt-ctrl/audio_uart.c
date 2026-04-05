#include "audio_uart.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include "wg_config.h"
#include "socketUDP.h"
#include "mgmt_types.h"
#include "mgmt_transmit.h"
#include "mgmt_netlink.h"
#include "sqlite_unit.h"
#include "gpsget.h"
#include "ui_get.h"

#ifndef CRTSCTS
#ifdef CNEW_RTSCTS
#define CRTSCTS CNEW_RTSCTS
#else
#define CRTSCTS 0
#endif
#endif

struct sockaddr_in S_GROUND_AUDIO;

int SOCKET_AUDIO;
int serial_port;
int audio_read;
Info_0x06_Statistics stat_info;

int audio_uart_init(void)
{
	uint8_t cnt=0;

	while(cnt<MAX_RETRY_COUNT)
	{
		serial_port = open(FD_AUDIO_UART, O_RDWR | O_NOCTTY);  // 打开音频串口
		if(serial_port!=-1)
			break;
		printf("[AUDIO ERROR] open audio uart failed, dev=%s, retrying... (%d/%d), errno=%d(%s)\n",
		       FD_AUDIO_UART,
		       cnt + 1,
		       MAX_RETRY_COUNT,
		       errno,
		       strerror(errno));
		cnt++;
		if(cnt<MAX_RETRY_COUNT)
			sleep(1);		
	}
	if(serial_port==-1)
	{
		printf("[AUDIO ERROR] open audio uart fail, dev=%s\r\n", FD_AUDIO_UART);
		return -1;
	}
    printf("[AUDIO DEBUG] audio fd :%d \r\n",serial_port);

	struct termios tty;
	if(tcgetattr(serial_port,&tty) !=0)
	{
		perror("获取串口配置失败");
		close(serial_port);
		return 1;
	}


	cfsetispeed(&tty,B460800);
	cfsetospeed(&tty,B460800);

	tty.c_cflag &= ~PARENB;//无校验
	tty.c_cflag &= ~CSTOPB;//1位停止位
	tty.c_cflag &= ~CSIZE;//屏蔽数据位
	tty.c_cflag |= CS8;//8位数据位
//	tty.c_cflag |= CREAD | CLOCAL;

	tty.c_lflag &= ~(ICANON | ECHO | ECHOE |ISIG);
	tty.c_oflag &= ~OPOST;

	tty.c_cflag &= ~CRTSCTS;//禁止硬件流控制
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);//禁止软件流控制

	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);



//音频数据
	tty.c_cc[VMIN] = 15;
	tty.c_cc[VTIME] = 0;

	printf("tty.c_cc[VMIN]=%d\n",tty.c_cc[VMIN]);
	printf("tty.c_cc[VTIME]=%d\n",tty.c_cc[VTIME]);

	if(tcsetattr(serial_port,TCSANOW,&tty)!=0)
	{
		perror("设置串口属性失败");
		close(serial_port);
		serial_port = -1;
		return -1;
	}

	printf("[audio uart thread]:audio uart init complete, ready to send\n");
	return 0;

}

void write_audio_info(char* buf,int len){
	int write_len;
	int i;
	uint8_t play_cnt=0;
	if(buf==NULL||len<=0)
	{
		printf("[AUDIO DEBUG] write buf is null \r\n");
		return;
	}
	Audio_Aggregated_Packet *packet=(Audio_Aggregated_Packet*)buf;

	while(play_cnt<PACKET_AGGREGATED_NUM)
	{
		write_len=write(serial_port,packet->packet[play_cnt].data,AUDIO_PACKET_SIZE);
		if(packet->packet[play_cnt].data[0]!=0x61)
		{
			play_cnt++;
			continue;		
		}
		// for(i=0;i<15;i++)
		// {
		// 	printf(" %#x",packet->packet[play_cnt].data[i]);
		// }
		// printf("\r\n");
		usleep(20000);
	}


}




void send_audio_broadcaset_packet(int socket_fd,void *buf,int size)
{
	if(buf==NULL||size<=0)
    {
        printf("[AUDIO DEBUG] audio buf is null\r\n");
        return;
    }

	if(SendUDPClient(socket_fd,buf,size,&S_GROUND_AUDIO)<0)
	{
		printf("[AUDIO DEBUG] send audio  packet fail\r\n");
		return ;
	}
}

void process_audio_info(int fd)
{
    FILE* file = NULL;

    char audio_buf[MAX_AUDIO_SIZE];
    if(fd<=0)
    {
		printf("[AUDIO ERROR] invalid audio fd: %d\r\n", fd);
        return;
    }

    Audio_Aggregated_Packet send_packet;
	memset(&send_packet,0,sizeof(Audio_Aggregated_Packet));

    SOCKET_AUDIO=createUdpClient((void*)&S_GROUND_AUDIO,"192.168.2.255",AUDIO_BROADCAST_PORT);

   	int opt=1;

    setsockopt(SOCKET_AUDIO,SOL_SOCKET,SO_BROADCAST,&opt,sizeof(opt));

#ifdef TEST_AUDIO    
/* test audio    */   
    char line[64]={0};
	file = fopen("/home/root/AMBE.txt","r");
	if(file == NULL)
	{
		printf("error opening file\n");
	}
	printf("[AUDIO DEBUG]test AMBE \r\n");

	
	while(1)
	{
	//需要把文件重定位到开头
    rewind(file);
	while(fgets(line,sizeof(line)-1,file) != NULL){
		uint8_t WT3081B_message[15]={0};

		WT3081B_message[0] = 0x61;
		WT3081B_message[1] = 0x00;
		WT3081B_message[2] = 0x09;
		WT3081B_message[3] = 0x01;
		WT3081B_message[4] = 0x01;
		WT3081B_message[5] = 0x48;
		uint8_t WT3081B_num = 6;

		char* split	= strtok(line,",");
		while(split != NULL && *split != '\r'){
			char* endptr = NULL;
			long hex_num = strtol(split,&endptr,0);
			WT3081B_message[WT3081B_num++] = hex_num;
			split = strtok(NULL,",");
		}

		send_audio_broadcaset_packet(SOCKET_AUDIO,WT3081B_message,sizeof(WT3081B_message));
		//ssize_t bytes_written = write(serial_port,WT3081B_message,sizeof(WT3081B_message));
		// for(i=0;i<bytes_written;i++)
		// {
		// 	printf(" %#x",WT3081B_message[i]);
		// }
		// printf("\r\n");
		//printf("[AUDIO DEBUG] write len %d \r\n",bytes_written);
		usleep(20000);
	}

}

#endif

	while(1)
    {
        memset(audio_buf,0,MAX_AUDIO_SIZE);
        audio_read=read(fd,audio_buf,MAX_AUDIO_SIZE-1);
        if(audio_read>0)
        {
			if(send_packet.current_cnt<PACKET_AGGREGATED_NUM)
			{
				memcpy(send_packet.packet[send_packet.current_cnt].data,audio_buf,audio_read);
				send_packet.current_cnt++;
			}
			else
			{
				//聚包完成
				send_packet.current_cnt=0;
				send_packet.seq++;
				send_audio_broadcaset_packet(SOCKET_AUDIO,(void*)&send_packet,sizeof(Audio_Aggregated_Packet));
				stat_info.audio_tx_packets++;
			}

			//audio_buf[2]=0x09; 
			//usleep(20000);
            //send_audio_broadcaset_packet(SOCKET_AUDIO,audio_buf,audio_read);
			
			//write_audio_info(audio_buf,audio_read);
			
        }
    }

}

void play_audio(void){
	int buflen=0;
	char buffer[1024];
	char ifname[] = "br0";
	struct sockaddr_in from;
	uint32_t selfip;
	char selfAddr[4] = {0xc0,0xa8,0x02,0x01};

	selfAddr[3]=SELFID;

	memcpy(&selfip,selfAddr,sizeof(uint32_t));

	int audio_s = CreateUDPServer(AUDIO_BROADCAST_PORT);
	if (audio_s <= 0)
	{
		printf("ERROR: create socket_audio \n");
		exit(1);
	}

	if (setsockopt(audio_s, SOL_SOCKET, SO_BROADCAST, ifname, 4) < 0) {
		printf("socket_audio opt error\n");
		exit(1);
	}
	
	int socklen=0;
	while(1)
	{
		buflen=recvfrom(audio_s, buffer, 1000, 0, &from, &socklen);
		if(selfip!=inet_addr(inet_ntoa(from.sin_addr))&&(buflen>0))
		{
			//printf("[AUDIO DEBUG] recv audio socket from %s  \r\n",inet_ntoa(from.sin_addr));
				
			//
			write_audio_info((char*)buffer,buflen);
			stat_info.audio_rx_packets++;
		}
	}
}


void audio_thread(void)
{
    printf("creat thread:get audio\r\n");
	if (audio_uart_init() != 0) {
		printf("[AUDIO ERROR] audio uart init failed, stop audio capture thread\r\n");
		return;
	}
    process_audio_info(serial_port);
}

void play_audio_thread(void)
{
	printf("create thread : send audio \r\n");
	play_audio();
}
