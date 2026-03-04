#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include "ui_get.h"
#include <stdlib.h>
#include "Thread.h"

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

    SetupSignal();

    ui_fd = uart_init();
    if(ui_fd == -1){
        printf("Failed to initialize UART.\n");
    }

    Create_Thread(get_ui_Thread,(void*)ui_fd);
	Create_Thread(write_ui_Thread,(void*)ui_fd);




    return 0;
}