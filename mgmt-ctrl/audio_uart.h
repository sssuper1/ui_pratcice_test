#ifndef __AUDIO_UART_H
#define __AUDIO_UART_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define FD_AUDIO_UART         "/dev/ttyS0"
#define MAX_AUDIO_SIZE        1024
#define AUDIO_BROADCAST_PORT   9001
#define MAX_RETRY_CNT_AUDIO    5

#define PACKET_AGGREGATED_NUM   30
#define AUDIO_PACKET_SIZE       15

typedef struct 
{
    uint8_t data[AUDIO_PACKET_SIZE];
}voice_packet_t;

typedef struct{

    uint32_t seq;
    uint8_t  current_cnt;
    voice_packet_t packet[PACKET_AGGREGATED_NUM];

}Audio_Aggregated_Packet;


int audio_uart_init(void);
void write_audio_info(char* buf,int len);
void audio_thread(void);
void play_audio_thread(void);

#endif