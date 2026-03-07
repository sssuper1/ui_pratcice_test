#include "gpsget.h"
#include "ui_get.h"


GPS_INFO gps_info_uart;

//度分格式 (ddmm.mmmm)
double dmconverttodeg(char* dm){
	double s_dm;
	int deg;
	double min;

	if(dm==NULL||strlen(dm)==0)
	{
		return 0.0;
	}

	s_dm = atof(dm);
	deg = (int)(s_dm/100);
	min = s_dm - deg*100;

	return deg + min/60.0;
}

// UTC时间转北京时间 (UTC时间格式为hhmmss.sss，北京时间为小时、分钟、秒)
void convertUTCToBeijingTime(const char* utc, uint8_t* time) {

	if(strlen(utc)>=6){
		uint8_t hour = (utc[0]-'0')*10 + (utc[1]-'0');
        uint8_t minute = (utc[2]-'0')*10 + (utc[3]-'0');
        uint8_t second = (utc[4]-'0')*10 + (utc[5]-'0');

		hour = (hour + 8) % 24; // 北京时间比UTC时间快8小时

		time[0] = hour;
		time[1] = minute;
		time[2] = second;
	}
	else{
		time[0]=time[1]=time[2]=0;
	}
}

void gps_getfrom_uart(int fd)
{
	int nemafd=0;
    int i=0;
    int count=0;
    char nema[MAX_GPS_SIZE];

	memset(&gps_info_uart,0,sizeof(GPS_INFO));
    int nread;
    set_opt(fd, GPS_UART_BAUD, 8, 'N', 1);	//设置nmea串口属性
	//write(fd, "$cfgsys,h01", 10);//唤醒gps模块 ssq

	uint8_t cmd[200];

	while(1){

		memset(nema,0,sizeof(nema));

		nread=read(fd,nema,MAX_GPS_SIZE); //从串口获取gps信息

		if(nread>0)
		{
			if(nread <=8)
			{
				/* 读取字节小于8字节，数据不完整 ，连续超出5次读取失败，重启串口*/
				count++;
			}
			else{
				count =0;
			}
			if(count >5)
			{
				 printf("error:fail to read gps from uart,reopen gps uart\r\n ");
                close(fd);
				sleep(2);
				while (1)
				{
					fd = open(FD_GPS_UART, O_RDWR);  //打开nmea串口
					if (fd == -1)
					{
						perror("nmeaFd:");
						sleep(1);
					}
					else
					{
						printf("gps uart reopen success\r\n");
						set_opt(fd, GPS_UART_BAUD, 8, 'N', 1);	//设置nmea串口属性
						//write(fd, "$cfgsys,h01", 10);//唤醒gps模块 ssq
						count = 0;
						break;
					}
				}
				
			}

			int gps_size = sizeof(nema);
			char print_gps[gps_size];
			for (i = 0; i < sizeof(nema)-5; i++){
				if(strstr(nema+i,"$GNGGA")!=NULL){
					// 字段1: UTC时间
					char *str1 = strchr(nema+i +7,',');
					if(str1==NULL)continue;
					int str1_len = strlen(nema + i+7) - strlen(str1);
					memcpy(gps_info_uart.utc,nema + i+7, str1_len);
					convertUTCToBeijingTime(gps_info_uart.utc,gps_info_uart.bj_time);  //UTC时间转北京时间
					// 字段2: 纬度
					char* str2 = strchr(str1 + 1, ',');
					if(str2==NULL)continue;
					int str2_len = strlen(str1 + 1) - strlen(str2);
					memcpy(gps_info_uart.latitude, str1 + 1, str2_len);
					// 字段3: 纬度方向
					char* str3 = strchr(str2 + 1, ',');
					if(str3==NULL)continue;
					int str3_len = strlen(str2 + 1) - strlen(str3);
					memcpy(&gps_info_uart.lat_mode, str2 + 1, str3_len);
					// 计算纬度
					if (gps_info_uart.lat_mode == 'N') {
					gps_info_uart.lat = dmconverttodeg(gps_info_uart.latitude);
					}
					else if (gps_info_uart.lat_mode == 'S') {
						gps_info_uart.lat = -dmconverttodeg(gps_info_uart.latitude);
					}
					else;
					// 字段4: 经度
					char* str4 = strchr(str3 + 1, ',');
					if(str4==NULL)continue;
					int str4_len = strlen(str3 + 1) - strlen(str4);
					memcpy(gps_info_uart.longitude, str3 + 1, str4_len);
					// 字段5: 经度方向
					char* str5 = strchr(str4 + 1, ',');
					if(str5==NULL)continue;
					int str5_len = strlen(str4 + 1) - strlen(str5);
					memcpy(&gps_info_uart.lon_mode, str4 + 1, str5_len);
					// 计算经度
					if (gps_info_uart.lon_mode == 'E') {
						gps_info_uart.lon = dmconverttodeg(gps_info_uart.longitude);
					}
					else if (gps_info_uart.lon_mode == 'W') {
						gps_info_uart.lon = -dmconverttodeg(gps_info_uart.longitude);
					}
					else;
					// 字段6: 定位状态
					char* str6 = strchr(str5 + 1, ',');
					if(str6==NULL)continue;
					int str6_len = strlen(str5 + 1) - strlen(str6);
					memcpy(&gps_info_uart.position_status, str5 + 1, str6_len);
					if(atoi(gps_info_uart.position_status)==0)
					{
						continue;
					}
					// 字段7: 使用卫星数量
					char* str7 = strchr(str6 + 1, ',');
					if(str7==NULL)continue;
					int str7_len = strlen(str6 + 1) - strlen(str7);
					memcpy(gps_info_uart.satellites_used, str6 + 1, str7_len);
					// 字段8: 水平精度因子
					char* str8 = strchr(str7 + 1, ',');
					if(str8==NULL)continue;
					int str8_len = strlen(str7 + 1) - strlen(str8);
					memcpy(gps_info_uart.hdop, str7 + 1, str8_len);
					// 字段9: 海拔高度
					char* str9 = strchr(str8 + 1, ',');
					if(str9==NULL)continue;
					int str9_len = strlen(str8 + 1) - strlen(str9);
					memcpy(gps_info_uart.altitude, str8 + 1, str9_len);
					// 字段10: 海拔高度单位
					char* str10 = strchr(str9 + 1, ',');
					if(str10==NULL)continue;
					int str10_len = strlen(str9 + 1) - strlen(str10);
					memcpy(&gps_info_uart.altitude_unit, str9 + 1, str10_len);
					// 打印GPS信息
					printf("UTC Time: %s, Beijing Time: %02d:%02d:%02d, Latitude: %s %c, Longitude: %s %c, Position Status: %s, Satellites Used: %s, HDOP: %s, Altitude: %s %c\n",
						gps_info_uart.utc, gps_info_uart.bj_time[0], gps_info_uart.bj_time[1], gps_info_uart.bj_time[2],
						gps_info_uart.latitude, gps_info_uart.lat_mode, gps_info_uart.longitude, gps_info_uart.lon_mode, 
						gps_info_uart.position_status, gps_info_uart.satellites_used, gps_info_uart.hdop, gps_info_uart.altitude, 
						gps_info_uart.altitude_unit);
					break;

				}
			}

		}

		else
			printf("[GPS DEBUG]read gps info error \r\n");

		sleep(10);

	}


}



void getGPS(void)
{
	int nmeaFd;
    nmeaFd = open(FD_GPS_UART, O_RDWR);  //打开nmea串口
	if (nmeaFd == -1)
    {
		printf("[GPS DEBUG]open gps \r\n");
		perror("nmeaFd:");
		sleep(1);
	}
	gps_getfrom_uart(nmeaFd);//传入串口文件描述符，获取gps信息

	close(nmeaFd);
	return;
}



void gps_Thread(void* arg) 
{
	printf("thread:get gps info\r\n");
	while (1) 
	{
		getGPS();
//		printf("gps recyle\n");
		sleep(1);
	}
}
