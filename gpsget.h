#ifndef __GPSGET_H
#define __GPSGET_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<termios.h>
#include<string.h>
#include<stdint.h>


#define MAX_GPS_SIZE 1024
#define GPS_UART_BAUD    9600
#define FD_GPS_UART     "/dev/ttymxc1" 

#pragma pack(push, 1)

typedef struct 
{
    char utc[20];
    uint8_t bj_time[3];
    char locate_mode;    //定位状态
    char latitude[20];   //纬度    
    char lat_mode;       //N，S
    char longitude[20];  //经度
    char lon_mode;       //E W
    // char rate[20];       //地面速率
    // char hangxiang[20];  //地面航向
    double lat;          //坐标格式
    double lon;          //
    uint16_t gaodu;
    // GNGGA特有字段
    char position_status[2];   // 定位质量指示
    char satellites_used[3]; // 使用卫星数量
    char hdop[6];           // 水平精度因子
    char altitude[10];      // 海拔高度
    char altitude_unit;     // 高度单位
}GPS_INFO;

void gps_Thread(void* arg);
#endif // !__GPSGET_H
