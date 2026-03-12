#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include "ui_get.h"
#include <stdlib.h>
#include "Thread.h"
#include "socketUDP.h"
#include "mgmt_types.h"
#include "mgmt_transmit.h"
#include "sqlite_unit.h"
#include "gpsget.h"
#include "sim_heartbeat.h"
//#pragma pack(1)


int ui_fd;

void SetupSignal()
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigemptyset(&sa.sa_mask) == -1 ||
            sigaction(SIGPIPE,&sa,0) == -1)
    {
        exit(-1); 
    }
}


int main() {
    printf("Hello, World!\n");

    sim_init();//初始化测试数据
    SetupSignal();//忽略SIGPIPE信号，防止写socket时对方关闭导致程序崩溃
    mgmt_mysql_init();
    ui_fd = uart_init();
    if(ui_fd == -1){
        printf("Failed to initialize UART.\n");
        return -1;
    }
    sqliteinit();
    Create_Thread(mgmt_get_msg,NULL);//状态上报
	Create_Thread(mgmt_recv_web,NULL);//将网页配置的参数通过netlink传递给mgmt模块
	Create_Thread(mgmt_recv_msg,NULL);
	Create_Thread(sqlite_set_param,NULL);//参数设置
	Create_Thread(gps_Thread,NULL);//gps数据获取
    
    Create_Thread(get_ui_Thread,(void*)ui_fd);
	Create_Thread(write_ui_Thread,(void*)ui_fd);


    while(1){
		sleep(10);
	
		}

	return 0;

}