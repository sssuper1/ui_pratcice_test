#include "gpsget.h"
#include "ui_get.h"
#include <time.h>


GPS_INFO gps_info_uart;

static void set_fake_gps_info(void)
{
	time_t now = time(NULL) + 8 * 3600;
	struct tm *tmv = gmtime(&now);
	if (!tmv) {
		memset(&gps_info_uart, 0, sizeof(GPS_INFO));
		return;
	}

	memset(&gps_info_uart, 0, sizeof(GPS_INFO));
	snprintf(gps_info_uart.utc, sizeof(gps_info_uart.utc), "%02d%02d%02d.000", tmv->tm_hour, tmv->tm_min, tmv->tm_sec);
	gps_info_uart.bj_time[0] = (uint8_t)tmv->tm_hour;
	gps_info_uart.bj_time[1] = (uint8_t)tmv->tm_min;
	gps_info_uart.bj_time[2] = (uint8_t)tmv->tm_sec;

	snprintf(gps_info_uart.latitude, sizeof(gps_info_uart.latitude), "3202.4000");
	gps_info_uart.lat_mode = 'N';
	snprintf(gps_info_uart.longitude, sizeof(gps_info_uart.longitude), "11845.6000");
	gps_info_uart.lon_mode = 'E';
	gps_info_uart.lat = 32.04;
	gps_info_uart.lon = 118.76;

	gps_info_uart.position_status[0] = '1';
	gps_info_uart.position_status[1] = '\0';
	snprintf(gps_info_uart.satellites_used, sizeof(gps_info_uart.satellites_used), "08");
	snprintf(gps_info_uart.hdop, sizeof(gps_info_uart.hdop), "0.9");
	snprintf(gps_info_uart.altitude, sizeof(gps_info_uart.altitude), "15.0");
	gps_info_uart.altitude_unit = 'M';
}

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
			char *gga_start = strstr(nema, "$GNGGA");

			if(gga_start != NULL){
				// 使用 do...while(0) 包裹，方便在解析失败时用 break 安全跳出当前解析，去执行下方的 sleep(10)
				do {
					// 字段1: UTC时间
					// 注意：这里必须是 gga_start + 7，绝对不能用 nema + 7
					char *str1 = strchr(gga_start + 7, ',');
					if(str1 == NULL) break; 
					
					// 优化：直接用指针相减计算长度
					int str1_len = str1 - (gga_start + 7);
					memcpy(gps_info_uart.utc, gga_start + 7, str1_len);
					convertUTCToBeijingTime(gps_info_uart.utc, gps_info_uart.bj_time); 
					
					// 字段2: 纬度
					char* str2 = strchr(str1 + 1, ',');
					if(str2 == NULL) break;
					int str2_len = str2 - (str1 + 1);
					memcpy(gps_info_uart.latitude, str1 + 1, str2_len);
					
					// 字段3: 纬度方向
					char* str3 = strchr(str2 + 1, ',');
					if(str3 == NULL) break;
					int str3_len = str3 - (str2 + 1);
					memcpy(&gps_info_uart.lat_mode, str2 + 1, str3_len);
					
					// 计算纬度
					if (gps_info_uart.lat_mode == 'N') {
						gps_info_uart.lat = dmconverttodeg(gps_info_uart.latitude);
					} else if (gps_info_uart.lat_mode == 'S') {
						gps_info_uart.lat = -dmconverttodeg(gps_info_uart.latitude);
					}
					
					// 字段4: 经度
					char* str4 = strchr(str3 + 1, ',');
					if(str4 == NULL) break;
					int str4_len = str4 - (str3 + 1);
					memcpy(gps_info_uart.longitude, str3 + 1, str4_len);
					
					// 字段5: 经度方向
					char* str5 = strchr(str4 + 1, ',');
					if(str5 == NULL) break;
					int str5_len = str5 - (str4 + 1);
					memcpy(&gps_info_uart.lon_mode, str4 + 1, str5_len);
					
					// 计算经度
					if (gps_info_uart.lon_mode == 'E') {
						gps_info_uart.lon = dmconverttodeg(gps_info_uart.longitude);
					} else if (gps_info_uart.lon_mode == 'W') {
						gps_info_uart.lon = -dmconverttodeg(gps_info_uart.longitude);
					}
					
					// 字段6: 定位状态
					char* str6 = strchr(str5 + 1, ',');
					if(str6 == NULL) break;
					int str6_len = str6 - (str5 + 1);
					memcpy(&gps_info_uart.position_status, str5 + 1, str6_len);
					
					// 如果未定位，退出当前解析
					if(atoi((char*)&gps_info_uart.position_status) == 0) {
						break;
					}
					
					// 字段7: 使用卫星数量
					char* str7 = strchr(str6 + 1, ',');
					if(str7 == NULL) break;
					int str7_len = str7 - (str6 + 1);
					memcpy(gps_info_uart.satellites_used, str6 + 1, str7_len);
					
					// 字段8: 水平精度因子
					char* str8 = strchr(str7 + 1, ',');
					if(str8 == NULL) break;
					int str8_len = str8 - (str7 + 1);
					memcpy(gps_info_uart.hdop, str7 + 1, str8_len);
					
					// 字段9: 海拔高度
					char* str9 = strchr(str8 + 1, ',');
					if(str9 == NULL) break;
					int str9_len = str9 - (str8 + 1);
					memcpy(gps_info_uart.altitude, str8 + 1, str9_len);
					
					// 字段10: 海拔高度单位
					char* str10 = strchr(str9 + 1, ',');
					if(str10 == NULL) break;
					int str10_len = str10 - (str9 + 1);
					memcpy(&gps_info_uart.altitude_unit, str9 + 1, str10_len);
					
					// 打印GPS信息
					printf("UTC Time: %s, Beijing Time: %02d:%02d:%02d, Latitude: %s %c, Longitude: %s %c, Position Status: %s, Satellites Used: %s, HDOP: %s, Altitude: %s %c\n",
						gps_info_uart.utc, gps_info_uart.bj_time[0], gps_info_uart.bj_time[1], gps_info_uart.bj_time[2],
						gps_info_uart.latitude, gps_info_uart.lat_mode, gps_info_uart.longitude, gps_info_uart.lon_mode, 
						gps_info_uart.position_status, gps_info_uart.satellites_used, gps_info_uart.hdop, gps_info_uart.altitude, 
						gps_info_uart.altitude_unit);
						
				} while(0); // 利用 do...while(0) 实现单次安全跳转
			}

		}
		else {
			static time_t last_notice = 0;
			time_t now = time(NULL);
			set_fake_gps_info();
			if (now - last_notice >= 30) {
				last_notice = now;
				printf("[GPS SIM] gps uart read failed, using fake gps data\r\n");
			}
		}

		sleep(10);

	}


}



void getGPS(void)
{
	int nmeaFd;
    nmeaFd = open(FD_GPS_UART, O_RDWR);  //打开nmea串口
	if (nmeaFd == -1)
    {
		static time_t last_notice = 0;
		time_t now = time(NULL);
		set_fake_gps_info();
		if (now - last_notice >= 30) {
			last_notice = now;
			printf("[GPS SIM] gps uart open failed, using fake gps data\r\n");
		}
		return;
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
