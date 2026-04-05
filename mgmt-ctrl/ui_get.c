#include "ui_get.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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


extern int ui_fd;

extern Global_Radio_Param g_radio_param;
Info_0x06_Statistics stat_info;
extern uint8_t SELFID;
extern GPS_INFO gps_info_uart;

#define RX_BUF_SIZE 1024

#define XWG_FREQ_MIN_KHZ 225
#define XWG_FREQ_MAX_KHZ 2500

/* 联调测试开关：1=拦截成员详情UDP请求并本地模拟3个节点应答；0=走真实UDP链路 */
#define MEMBER_INFO_TEST_MODE 1

static int clamp_int_range(int v, int min_v, int max_v, int def_v)
{
    if (v < min_v || v > max_v) {
        return def_v;
    }
    return v;
}

static uint32_t xwg_freq_khz_to_hz(int raw_khz, uint32_t fallback_khz)
{
    int khz = raw_khz;
    if (khz < XWG_FREQ_MIN_KHZ || khz > XWG_FREQ_MAX_KHZ) {
        khz = (int)fallback_khz;
    }
    if (khz < XWG_FREQ_MIN_KHZ || khz > XWG_FREQ_MAX_KHZ) {
        khz = XWG_FREQ_MIN_KHZ;
    }
    return (uint32_t)khz * 1000u;
}

static uint8_t xwg_route_to_proto(int router_raw)
{
    /* /etc/node_xwg: 1=OLSR,2=AODV,3=BATMAN; 屏端协议: 0/1/2 */
    if (router_raw < 1 || router_raw > 3) {
        return 0u;
    }
    return (uint8_t)(router_raw - 1);
}

#if MEMBER_INFO_TEST_MODE
static int build_member_test_info(uint8_t id, NodeBasicInfo *node)
{
    if (node == NULL) {
        return 0;
    }

    memset(node, 0, sizeof(*node));

    switch (id)
    {
        case 2:
            node->member_id = 2;
            node->spatial_filter_status = 1;
            node->ip_address[0] = 192;
            node->ip_address[1] = 168;
            node->ip_address[2] = 2;
            node->ip_address[3] = 2;
            node->channel1.ch_frequency_hopping = 0;
            node->channel1.ch_working_freq = 1450000u;
            node->channel1.ch_signal_bandwidth = 0;
            node->channel1.ch_waveform = 3;
            node->channel1.ch_power_level = 2;
            node->channel1.ch_power_attenuation = 12;
            node->channel1.ch_routing_protocol = 0;
            node->channel1.ch_access_protocol = 0;
            return 1;

        case 3:
            node->member_id = 3;
            node->spatial_filter_status = 0;
            node->ip_address[0] = 192;
            node->ip_address[1] = 168;
            node->ip_address[2] = 2;
            node->ip_address[3] = 3;
            node->channel1.ch_frequency_hopping = 1;
            node->channel1.ch_working_freq = 430000u;
            node->channel1.ch_signal_bandwidth = 1;
            node->channel1.ch_waveform = 4;
            node->channel1.ch_power_level = 1;
            node->channel1.ch_power_attenuation = 8;
            node->channel1.ch_routing_protocol = 1;
            node->channel1.ch_access_protocol = 0;
            return 1;

        case 4:
            node->member_id = 4;
            node->spatial_filter_status = 1;
            node->ip_address[0] = 192;
            node->ip_address[1] = 168;
            node->ip_address[2] = 2;
            node->ip_address[3] = 4;
            node->channel1.ch_frequency_hopping = 1;
            node->channel1.ch_working_freq = 910000u;
            node->channel1.ch_signal_bandwidth = 2;
            node->channel1.ch_waveform = 5;
            node->channel1.ch_power_level = 3;
            node->channel1.ch_power_attenuation = 5;
            node->channel1.ch_routing_protocol = 2;
            node->channel1.ch_access_protocol = 0;
            return 1;

        default:
            return 0;
    }
}
#endif


int get_interface_stats(Info_0x06_Statistics* info_stat){
    FILE* fp;
    char buffer[1024];
    char command[128];
    unsigned long rx_packets = 0;
    unsigned long tx_packets = 0;
	uint8_t find_tx,find_rx;

	find_tx=find_rx=0;
    // 这里修改想要读取的网络配置
    snprintf(command, sizeof(command), "ifconfig eth0");

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        return -1;
    }

    while(fgets(buffer,sizeof(buffer),fp) !=NULL){
        // 修改想要读取的参数
        if (strstr(buffer, "RX packets") != NULL) {
            // 使用 sscanf 从行中提取 RX packets 的数值
            // ifconfig 输出中 "RX packets" 后的数字就是包数量
            if (sscanf(buffer, " RX packets : %lu", &rx_packets) == 1) {
				find_rx=1;
                //printf("RX packets: %lu\n", rx_packets);
            }
        }
        if (strstr(buffer, "TX packets") != NULL) {
            // 使用 sscanf 从行中提取 RX packets 的数值
            // ifconfig 输出中 "RX packets" 后的数字就是包数量
            if (sscanf(buffer, " TX packets : %lu", &tx_packets) == 1) {
				find_tx=1;
                //printf("TX packets: %lu\n", tx_packets);
            } 
        }

        if(find_tx==1&&find_rx==1)
		{
			info_stat->eth_rx_packets=(uint16_t)rx_packets;
			info_stat->eth_tx_packets=(uint16_t)tx_packets;
			break;

		}	


    }
    // 关闭管道
    pclose(fp);
    return 0;

}

/*0x04命令*/
int8_t Send_0x04(int fd,void* info,int size)
{
    Frame_04_byte frame_send;
    memset(&frame_send,0,sizeof(Frame_04_byte));
    int BYTE04SIZE=sizeof(Frame_04_byte);

    uint32_t center_freq_hz;
    int workmode;
    int router_raw;
    int sync_mode_raw;
    int bw_raw;
    int mcs_raw;

    uint8_t send_byte_stream[BYTE04SIZE] ;
	uint8_t byte_stream_recv[9] ;

	if (info == NULL || size <= 0) {
		return -1;
	}

	Node_Xwg_Pairs *param_pairs=(Node_Xwg_Pairs *)malloc(size);
	if (param_pairs == NULL) {
		return -1;
	}
	memcpy(param_pairs,(Node_Xwg_Pairs*)info,size);

    //将04命令需要的参数值构造成一帧
    frame_send.head = htons(0xD55D);
    frame_send.dirrection = 0X04;
    frame_send.message_type = 0xFF;
    frame_send.current_work_mode = (uint8_t)clamp_int_range(get_int_value((void*)param_pairs,"macmode"), 0, 255, 0);//当前工作模式
    frame_send.spatial_filter = 0x01;
    if(get_int_value((void*)param_pairs,"kylb")==KYLB_MODE_OPEN)
	{
		frame_send.spatial_filter = 0;      
	}

    sync_mode_raw = clamp_int_range(get_int_value((void*)param_pairs,"sync_mode"), 0, 1, 0);
    frame_send.sync_mode = (uint8_t)sync_mode_raw;

    router_raw = get_int_value((void*)param_pairs,"router");
    frame_send.Channel1.route_protocol = xwg_route_to_proto(router_raw);
    frame_send.Channel1.access_protocol = 0x00;

    workmode = get_int_value((void*)param_pairs,"workmode");
    if(workmode==WORK_MODE_TYPE_DP)//跳频方式：定频  自适应选频
	{
		frame_send.Channel1.hopping_mode = 0x00; // 跳频方式 定频
	}
	else if(workmode==WORK_MODE_TYPE_ZSYXP)
	{
		frame_send.Channel1.hopping_mode = 1;      // 跳频方式 自适应选频
	}

    center_freq_hz = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"channel"), XWG_FREQ_MIN_KHZ);
    frame_send.Channel1.center_freq = center_freq_hz;
    frame_send.Channel1.select_freq_1 = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq1"), center_freq_hz / 1000u);
    frame_send.Channel1.select_freq_2 = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq2"), center_freq_hz / 1000u);
    frame_send.Channel1.select_freq_3 = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq3"), center_freq_hz / 1000u);
    frame_send.Channel1.select_freq_4 = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq4"), center_freq_hz / 1000u);

    bw_raw = clamp_int_range(get_int_value((void*)param_pairs,"bw"), 0, 3, 0);
    frame_send.Channel1.signal_bw = (uint8_t)bw_raw;

    mcs_raw = clamp_int_range(get_int_value((void*)param_pairs,"mcs"), 0, 7, 0);
    frame_send.Channel1.mod_wide = (uint8_t)mcs_raw;
    //frame_send.Channel1.tx_power_spread = (uint8_t)get_int_value((void*)param_pairs,"txpower");
    //frame_send.Channel1.tx_attenuation = (uint8_t)get_int_value((void*)param_pairs,"txattenuation");
    frame_send.Channel1.tx_power_spread = 0;// 发射功率
    frame_send.Channel1.tx_attenuation = 0;// 发射功率衰减

    frame_send.check = htons(CRC_Check(&frame_send.dirrection,BYTE04SIZE-6));
    frame_send.tail = htons(0x5DD5);
    memcpy(send_byte_stream,&frame_send,BYTE04SIZE);

    write(fd,send_byte_stream,BYTE04SIZE);
	free(param_pairs);
    sleep(1);
    return 0;
}

int8_t Send_0x05(int fd,void* info)
{
    int i;

	double t_lon,t_lat;
	uint16_t t_alit;
    Frame_05_byte frame_send ;
	int BYTE05SIZE=sizeof(Frame_05_byte);
	memset(&frame_send,0,BYTE05SIZE);
	
	uint8_t send_byte_stream[BYTE05SIZE] ;

	Global_Radio_Param param;
	memcpy(&param,(Global_Radio_Param*)info,sizeof(Global_Radio_Param));

    frame_send.head= htons(0xD55D);
    frame_send.dirrection=0x05;
    frame_send.message_type=0xFF;

    frame_send.value.device_id = SELFID;
    char device_name[10];
    memset(device_name,0,sizeof(device_name));
    sprintf(frame_send.value.device_name,"kddt-%d",SELFID);

    // 业务网口IP地址
	frame_send.value.service_ip[0]=192;
	frame_send.value.service_ip[1]=168;
	frame_send.value.service_ip[2]=2;
	frame_send.value.service_ip[3]=SELFID;
    frame_send.value.service_port = 6000; 

    frame_send.value.serial_baudrate = 4;        // 串口波特率
	frame_send.value.serial_databits = 3;        // 串口数据位
	frame_send.value.serial_stopbits = 0;        // 串口停止位
	frame_send.value.serial_parity = 0;          // 串口校验位
	frame_send.value.serial_flowctrl = 0;        // 串口流控

    frame_send.value.gps_auto_sync=0;			// 位置设置-自动获取
	sprintf(frame_send.value.longitude, "%.6f", gps_info_uart.lon);  // 经度
	sprintf(frame_send.value.latitude, "%.6f", gps_info_uart.lat);   // 纬度
	frame_send.value.altitude = gps_info_uart.gaodu*10;  

    frame_send.value.time_auto_sync = 0;         // 时间自动获取
	frame_send.value.manual_hour = gps_info_uart.bj_time[0];            // 手动设置时
	frame_send.value.manual_minute = gps_info_uart.bj_time[1];          // 手动设置分
	frame_send.value.manual_second = gps_info_uart.bj_time[2];          // 手动设置秒
    
    frame_send.check = htons(CRC_Check(&frame_send.dirrection,BYTE05SIZE-6));
    frame_send.tail = htons(0x5DD5);
    memcpy(send_byte_stream,&frame_send,BYTE05SIZE);

    write(fd,send_byte_stream,BYTE05SIZE);
    sleep(1);

    return 0;
}

int8_t Send_0x06(int fd,void* buf)
{
    int i=0;
	uint16_t BYTE06SIZE=sizeof(Frame_06_byte);

	Frame_06_byte frame_send ;
	memset(&frame_send,0,BYTE06SIZE);

	uint8_t send_byte_stream[BYTE06SIZE] ;

	static uint16_t s_rx_packet=0;
	static uint16_t s_tx_packet=0; 
	Info_0x06_Statistics *info=(Info_0x06_Statistics*)buf;

    if(info->stat_flag==1)
	{
		// 停止计数

		return 0;
	}
    else if(info->stat_flag==2)   //清零
	{
		get_interface_stats(info);
		s_rx_packet=info->eth_rx_packets;
		s_tx_packet=info->eth_tx_packets;
		// printf("[UI DEBUG] static rx %d tx %d \r\n",s_rx_packet,s_tx_packet);
		stat_info.stat_flag=0;
		
		stat_info.audio_rx_packets=stat_info.audio_tx_packets=0;
	}

    get_interface_stats(info);
    frame_send.head = htons(0xD55D);
	frame_send.dirrection = 0x06;
	frame_send.message_type = 0xFF;
    frame_send.value.eth_tx_cnt = info->eth_tx_packets-s_tx_packet;      // 以太网消息发送个数
	frame_send.value.eth_rx_cnt = info->eth_rx_packets-s_rx_packet;      // 以太网消息接收个数
	frame_send.value.voice_tx_cnt = info->audio_tx_packets;      		 // 模拟话音发送个数
	frame_send.value.voice_rx_cnt = info->audio_rx_packets;     		 // 模拟话音接收个数
	frame_send.value.total_tx_cnt = frame_send.value.eth_tx_cnt+frame_send.value.voice_tx_cnt;      // 总发送消息个数
	frame_send.value.total_rx_cnt = frame_send.value.eth_rx_cnt+frame_send.value.voice_rx_cnt;      // 总接收消息个数
    frame_send.check = htons(CRC_Check(&frame_send.dirrection,BYTE06SIZE-6));
    frame_send.tail = htons(0x5DD5);
    memcpy(send_byte_stream,&frame_send,BYTE06SIZE);

    write(fd,send_byte_stream,BYTE06SIZE);
    sleep(1);

    return 0;
}

int8_t Send_0x07(int fd,void* info)
{
    int i;
    Frame_07_byte frame_send ;
    int BYTE07SIZE=sizeof(Frame_07_byte);
    memset(&frame_send,0,BYTE07SIZE);

    uint8_t send_byte_stream[BYTE07SIZE] ;

    DEVICE_SC_STATUS_REPORT amp_param;
	memcpy(&amp_param,(DEVICE_SC_STATUS_REPORT*)info,sizeof(DEVICE_SC_STATUS_REPORT));
	
	frame_send.head = htons(0xD55D);
    frame_send.dirrection=0x07;
    frame_send.message_type=0xFF;
    frame_send.value.self_test_status = 0;                     // 自检状态
    frame_send.value.battery_remaining_capacity = amp_param.battery_level;           // 电池剩余电量
    frame_send.value.battery_cycle_count = 1000;                  // 电池剩余循环次数
    frame_send.value.battery_self_test_status = amp_param.battery_self_test;             // 电池自检状态
    frame_send.value.info_processor_temp = amp_param.temperature;                  // 综合信息处理温度
    frame_send.value.fan_speed_status = amp_param.fan_status;                     // 风机转速状态
    frame_send.value.nav_lock_status = amp_param.nav_lock_status;                      // 卫导锁定状态
    frame_send.value.clock_selection = amp_param.sync_status;                      // 时钟选择状态
    frame_send.value.adc_status = amp_param.sense_adc_status;                           // ADC状态
    frame_send.value.clock_source_temp = amp_param.freq_temperature;                    // 时钟频率源温度
    frame_send.value.freq_word_send_count = amp_param.freq_word_count;                 // 频率字下发次数计数
    frame_send.value.comm_sensing_status = amp_param.rf_sense_status;                  // 通信感知状态
    frame_send.value.ref_clock1_status = 0x01;                    // 输出参考时钟1状态
    frame_send.value.ref_clock2_status = 0x01;                    // 输出参考时钟2状态
    frame_send.value.lo1_output_status = (amp_param.freq_lo_ready >> 0) & 0x01;                    // 本振1输出状态
    frame_send.value.lo2_output_status = (amp_param.freq_lo_ready >> 1) & 0x01;                    // 本振2输出状态
    frame_send.value.lo3_output_status = (amp_param.freq_lo_ready >> 2) & 0x01;                    // 本振3输出状态
    frame_send.value.lo4_output_status = (amp_param.freq_lo_ready >> 3) & 0x01;                    // 本振4输出状态
    frame_send.value.power_conversion_temp = amp_param.power_temperature;                // 电源变换温度
    frame_send.value.power_on_fault_indicator = amp_param.power_power_fault;             // 上电/故障指示
    frame_send.value.power_supply_control = amp_param.power_ch_power_status;                 // 各路加电控制
    frame_send.value.ac220_power_consumption = amp_param.power_ac220_power;              // AC220功耗
    frame_send.value.dc24v_power_consumption = amp_param.power_dc24v_power;              // DC24V功耗

    frame_send.value.rf_channel_temp = amp_param.rf_ch1_temp1;                          // 通道1温度
    frame_send.value.rf_channel_tx_power_status = amp_param.rf_tx_power_status;               // 发射功率检波状态
    frame_send.value.rf_channel_antenna_l_vswr = amp_param.rf_antenna_l_vswr;                // 天线L口驻波状态
    frame_send.value.rf_channel_antenna_h1_vswr = amp_param.rf_antenna_h1_vswr;               // 天线H-1口驻波状态
    frame_send.value.rf_channel_antenna_h2_vswr =  amp_param.rf_antenna_h2_vswr;               // 天线H-2口驻波状态
    frame_send.value.rf_channel_tx_rx_status = amp_param.rf_tx_rx_status;                  // 收发状态指示
    frame_send.value.rf_Channel_antenna_select = amp_param.rf_antenna_select;                // 天线选择控制状态
    frame_send.value.rf_channel_comm_sensing = amp_param.rf_sense_status;                  // 通信感知状态
    frame_send.value.rf_channel_power_control = amp_param.power_ch_power_status;                 // 加电控制状态
    frame_send.value.rf_channel_power_level = amp_param.rf_ch_power_level;                   // 通道功率等级

    frame_send.Channel1.rf_channel_rf_power_detect = amp_param.rf_ch1_rf_power;               // 射频功率检波
    frame_send.Channel1.rf_channel_if_power_detect = amp_param.rf_ch1_if_power;               // 中频功率检波
    frame_send.Channel1.rf_channel_current_freq = amp_param.rf_ch1_freq;                  // 当前频点
    frame_send.Channel1.rf_channel_agc_attenuation = amp_param.rf_ch1_agc_atten;               // AGc衰减值
    frame_send.Channel1.rf_channel_signal_bandwidth = amp_param.rf_ch1_bandwidth;              // 通道1信号带宽
    frame_send.Channel1.rf_channel_attenuation = amp_param.rf_ch1_agc_atten;                   // 通道1衰减量

	frame_send.Channel2.rf_channel_rf_power_detect = amp_param.rf_ch2_rf_power;               // 射频功率检波
	frame_send.Channel2.rf_channel_if_power_detect = amp_param.rf_ch2_if_power;               // 中频功率检波
	frame_send.Channel2.rf_channel_current_freq = amp_param.rf_ch2_freq;                  // 当前频点
	frame_send.Channel2.rf_channel_agc_attenuation = amp_param.rf_ch2_agc_atten;               // AGc衰减值
	frame_send.Channel2.rf_channel_signal_bandwidth = amp_param.rf_ch2_bandwidth;              // 通道1信号带宽
	frame_send.Channel2.rf_channel_attenuation = amp_param.rf_ch2_agc_atten;                   // 通道1衰减量

	frame_send.Channel3.rf_channel_rf_power_detect = amp_param.rf_ch3_rf_power;               // 射频功率检波
	frame_send.Channel3.rf_channel_if_power_detect = amp_param.rf_ch3_if_power;               // 中频功率检波
	frame_send.Channel3.rf_channel_current_freq = amp_param.rf_ch3_freq;                  // 当前频点
	frame_send.Channel3.rf_channel_agc_attenuation = amp_param.rf_ch3_agc_atten;               // AGc衰减值
	frame_send.Channel3.rf_channel_signal_bandwidth = amp_param.rf_ch3_bandwidth;              // 通道1信号带宽
	frame_send.Channel3.rf_channel_attenuation = amp_param.rf_ch3_agc_atten;                   // 通道1衰减量

	frame_send.Channel4.rf_channel_rf_power_detect = amp_param.rf_ch4_rf_power;               // 射频功率检波
	frame_send.Channel4.rf_channel_if_power_detect = amp_param.rf_ch4_if_power;               // 中频功率检波
	frame_send.Channel4.rf_channel_current_freq = amp_param.rf_ch4_freq;                  // 当前频点
	frame_send.Channel4.rf_channel_agc_attenuation = amp_param.rf_ch4_agc_atten;               // AGc衰减值
	frame_send.Channel4.rf_channel_signal_bandwidth = amp_param.rf_ch4_bandwidth;              // 通道1信号带宽
	frame_send.Channel4.rf_channel_attenuation = amp_param.rf_ch4_agc_atten;                   // 通道1衰减量

	frame_send.check = htons(CRC_Check(&frame_send.dirrection, BYTE07SIZE-6));					//校验
	frame_send.tail = htons(0x5DD5);
	memcpy(send_byte_stream,&frame_send,BYTE07SIZE);
	
	
	write(fd,(void*)send_byte_stream,BYTE07SIZE);
    sleep(1);
    return 0;
}

int8_t Send_0x08(int fd,void* info,int size)
{
    int i;
	Frame_08_byte frame_send ;
	memset(&frame_send,0,sizeof(Frame_08_byte));
	int BYTE08SIZE=sizeof(Frame_08_byte);

    uint32_t center_freq_hz;
    int workmode;
    int bw_raw;
    int mcs_raw;
    int sync_mode_raw;

	uint8_t send_byte_stream[BYTE08SIZE] ;
	uint8_t byte_stream_recv[9] ;

    if (info == NULL || size <= 0) {
        return -1;
    }

	Node_Xwg_Pairs *param_pairs=(Node_Xwg_Pairs *)malloc(size);
    if (param_pairs == NULL) {
        return -1;
    }
	memcpy(param_pairs,(Node_Xwg_Pairs*)info,size);

	frame_send.head = htons(0xD55D);
	frame_send.dirrection = 0x08;
	frame_send.message_type = 0xFF;
	
    //当前时间
    memcpy(frame_send.current_time,gps_info_uart.bj_time,3) ;
    //入网、未入网
    frame_send.net_join_state=0;
    //mcs
    mcs_raw = clamp_int_range(get_int_value((void*)param_pairs,"mcs"), 0, 7, 0);
    frame_send.mcs= (uint8_t)mcs_raw;		
    //信号带宽
    bw_raw = clamp_int_range(get_int_value((void*)param_pairs,"bw"), 0, 3, 0);
    frame_send.bw = (uint8_t)bw_raw;
    //跳频方式
	workmode = get_int_value((void*)param_pairs,"workmode");
    if(workmode==WORK_MODE_TYPE_DP)
	{
		
		frame_send.workmode = 0x00; // 跳频方式 定频
	}
	else if(workmode==WORK_MODE_TYPE_ZSYXP)
	{
		frame_send.workmode = 1;      // 跳频方式 自适应选频
	}
    else
    {
        frame_send.workmode = 0x00;
    }
    //点频-工作频点 自适应选频-备选频点1 2 3 4
    center_freq_hz = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"channel"), XWG_FREQ_MIN_KHZ);
    frame_send.center_freq    = center_freq_hz;// 中心频率（点频） *1000
    frame_send.select_freq1  = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq1"), center_freq_hz / 1000u);// 自适应-中心频率1 *1000
    frame_send.select_freq2  = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq2"), center_freq_hz / 1000u);// 自适应-中心频率2 *1000
    frame_send.select_freq3  = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq3"), center_freq_hz / 1000u);// 自适应-中心频率3 *1000
    frame_send.select_freq4  = xwg_freq_khz_to_hz(get_int_value((void*)param_pairs,"select_freq4"), center_freq_hz / 1000u);// 自适应-中心频率4 *1000

    //空域滤波
    frame_send.kylb = 0x01;
    if(get_int_value((void*)param_pairs,"kylb")==KYLB_MODE_OPEN)
	{
		frame_send.kylb = 0;     
	}
    //内外同步模式
    sync_mode_raw = clamp_int_range(get_int_value((void*)param_pairs,"sync_mode"), 0, 1, 0);
    frame_send.sync_mode =  (uint8_t)sync_mode_raw;
    //发射功率
    frame_send.tx_power_spread = 0;
    //发射功率衰减
    frame_send.tx_attenuation = 0;

    frame_send.check = htons(CRC_Check(&frame_send.dirrection, BYTE08SIZE-6));	//校验
	frame_send.tail = htons(0x5DD5);
	memcpy(send_byte_stream,&frame_send,BYTE08SIZE);

	write(fd,(void*)send_byte_stream,BYTE08SIZE);
    free(param_pairs);
	sleep(1);
    return 0;  
}

int8_t Send_0x09(int fd,void* info)
{
    // 0x09命令：上报成员信息变化（单播）
    int neigh_num=0;
	int size=0;
	int i;

    int NODESTATUSSIZE = sizeof(NetworkNodeStatus);// 每个成员信息的大小
	int BYTE0ASIZE=sizeof(Frame_0A_byte);

    struct mgmt_send self_msg;
    memcpy((void*)&self_msg,(struct mgmt_send*)info,sizeof(self_msg));

    neigh_num=self_msg.neigh_num;

	if(neigh_num==0||neigh_num>4)
	{
		return 0;
    }
    size=NODESTATUSSIZE*neigh_num;// 成员信息总大小

    NetworkNodeStatus *net_info=(NetworkNodeStatus*)malloc(size);
	memset(net_info,0,size);
	uint8_t send_byte_stream[size+9] ;//9:帧头2字节 帧尾2字节 命令字1字节 占位符1字节 数量1字节 校验2字节

    for(int i=0;i<neigh_num;i++)
	{
		net_info[i].member_id=self_msg.msg[i].node_id;
		net_info[i].ip_address[0]=192;
		net_info[i].ip_address[1]=168;
		net_info[i].ip_address[2]=2;
		net_info[i].ip_address[3]=self_msg.msg[i].node_id;
	
		//sprintf(net_info[i].ip_address,"192.168.2.%d",self_msg.msg[i].node_id);
		net_info[i].hop_count=1;
		net_info[i].signal_strength=0-self_msg.msg[i].rssi;
		net_info[i].transmission_delay=self_msg.msg[i].time_jitter;

	}
    send_byte_stream[0]=0xD5;		
	send_byte_stream[1]=0x5D;		
	send_byte_stream[2]=0x09;		
	send_byte_stream[3]=0xFF;		
	send_byte_stream[4]=neigh_num;		

	memcpy(send_byte_stream+5,net_info,size);
	uint16_t check = CRC_Check(send_byte_stream+2, size+3);//+3 是命令字1字节 + 占位符1字节 + 数量1字节
	send_byte_stream[size+5]=check >> 8;
	send_byte_stream[size+6]=check & 0xff;
	send_byte_stream[size+7]=0x5d;
	send_byte_stream[size+8]=0xd5;

	write(fd,(void*)send_byte_stream,size+9);

    free(net_info);
    net_info=NULL;
    sleep(3); 
    return 0;
}

int8_t Send_0x0A(int fd,void* param,int size)
{
    uint8_t i=0;
	Frame_0A_byte frame_0A;
	int BYTE0ASIZE=sizeof(Frame_0A_byte);
	memset(&frame_0A,0,BYTE0ASIZE);
	uint8_t send_byte_stream[BYTE0ASIZE] ;
    
    if (param == NULL) return -1;
	MEM_REPLY_FRAME *member_info=(MEM_REPLY_FRAME*)param;

    frame_0A.head=htons(0xD55D);
	frame_0A.dirrection=0x0A;
	frame_0A.message_type=0x0;

    memcpy((void*)&frame_0A.member,(void*)&member_info->member,sizeof(NodeBasicInfo));

    frame_0A.check = htons(CRC_Check(&frame_0A.dirrection, BYTE0ASIZE-6));	
	frame_0A.tail =  htons(0x5DD5);

    memcpy(send_byte_stream,&frame_0A,BYTE0ASIZE);
	printf("send cmd 0x0a,len:%d\r\n",write(fd,(void*)send_byte_stream,BYTE0ASIZE));

    sleep(1);

    for(i=0;i<sizeof(send_byte_stream);i++)
	{
		printf("%02X",send_byte_stream[i]);
	}
	printf("\r\n");	
	
	return 0;

}
static void read_xwg_info(char *info,int size)
{
/* temp param */
	uint8_t  t_workmode=0;
	uint32_t t_freq;
	uint8_t  t_bw;
	uint8_t  t_mcs;
	uint8_t  t_routing;
	uint8_t  t_power_level;
	uint8_t  t_power_atten;
	uint32_t t_select_freq1,t_select_freq2,t_select_freq3,t_select_freq4;
	uint8_t  t_kylb;
	uint8_t  t_sync_mode;
// read /etc/node_xwg
    Node_Xwg_Pairs param_pairs[MAX_XWG_PAIRS] = {
        {"channel", 0, 0},{"power", 0, 0},{"bw", 0, 0},{"mcs", 0, 0},
        {"macmode", 0, 0},{"slotlen", 0, 0},{"router", 0, 0},{"workmode", 0, 0},
        {"select_freq1", 0, 0},{"select_freq2", 0, 0},{"select_freq3", 0, 0},{"select_freq4", 0, 0},
		{"sync_mode", 0, 0},{"kylb", 0, 0}
		// {"longitude ",0,0},{"latitude  ",0,0},{"altitude ",0,0}
    };

	//sleep(30);
	read_node_xwg_file("/etc/node_xwg",param_pairs,MAX_XWG_PAIRS);

	NodeBasicInfo node_info;
	memset(&node_info,0,sizeof(NodeBasicInfo));

	node_info.member_id=SELFID;
	// sprintf(node_info.ip_address,"192.168.2.%d",SELFID);
	node_info.ip_address[0]=192;
	node_info.ip_address[1]=168;
	node_info.ip_address[2]=2;
	node_info.ip_address[3]=SELFID;



	t_workmode=(uint8_t)get_int_value((void*)param_pairs,"workmode");

	t_kylb=(uint8_t )get_int_value((void*)param_pairs,"kylb");
	t_freq=get_int_value((void*)param_pairs,"channel")*1000;
	t_select_freq1=get_int_value((void*)param_pairs,"select_freq1");
	t_select_freq2=get_int_value((void*)param_pairs,"select_freq2");
	t_select_freq3=get_int_value((void*)param_pairs,"select_freq3");
	t_select_freq4=get_int_value((void*)param_pairs,"select_freq4");
	t_sync_mode=(uint8_t)get_int_value((void*)param_pairs,"sync_mode");
	t_bw=(uint8_t)get_int_value((void*)param_pairs,"bw");
	t_mcs=(uint8_t)get_int_value((void*)param_pairs,"mcs");
	t_routing=(uint8_t)get_int_value((void*)param_pairs,"router");
	// t_power_level
	// t_power_atten

	node_info.spatial_filter_status=1;  //默认空余滤波关
	node_info.channel1.ch_frequency_hopping=0; //默认定频

	if(t_kylb==KYLB_MODE_OPEN)
	{
		node_info.spatial_filter_status=0;
	}
	if(t_workmode==5)
	{
		node_info.channel1.ch_frequency_hopping=1;
	}


    /* 屏端参数字典按 kHz 存储并以 scale=3 显示，0x0A 这里保持 kHz 口径 */
    node_info.channel1.ch_working_freq=t_freq;
	node_info.channel1.ch_waveform=t_mcs;
	node_info.channel1.ch_signal_bandwidth=t_bw;
	node_info.channel1.ch_routing_protocol=t_routing-1;
	

	memcpy(info,(void*)&node_info,size);
    
}
void send_member_request(uint8_t id)
{
#if MEMBER_INFO_TEST_MODE
    MEM_REPLY_FRAME reply_info;
    memset(&reply_info, 0, sizeof(reply_info));

    if (!build_member_test_info(id, &reply_info.member)) {
        printf("[UI TEST] member detail id=%u not configured\r\n", id);
        return;
    }

    reply_info.head = htons(0x4c4a);
    reply_info.type = 1;
    reply_info.src_id = id;
    reply_info.dst_id = SELFID;
    reply_info.tail = htons(0x6467);

    printf("[UI TEST] send fake member detail id=%u by request id\r\n", id);
    Send_0x0A(ui_fd, (void *)&reply_info, sizeof(reply_info));
    return;
#else
    char dest_ip[20];
    int s_request;
	int ret=0;
    memset(dest_ip,0,sizeof(dest_ip));
    snprintf(dest_ip,sizeof(dest_ip),"192.168.2.%d",id);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));	
    s_request=createUdpClient(&addr,dest_ip,MEM_REQUEST_PORT);

    MEM_REQUEST_FRAME info;
	memset(&info,0,sizeof(MEM_REQUEST_FRAME));
	info.head=htons(0x4c4a);  // 局域网通信的“暗号”头
	info.src_id=SELFID;       // 告诉对方：我是 1 号
	info.dst_id=id;           // 告诉对方：我要找 5 号
	info.type=0;              // 【核心】type=0 代表“这是一个提问（Request）”
	info.tail=htons(0x6467);  // 局域网通信的“暗号”尾

    ret=SendUDPClient(s_request,(char*)&info,sizeof(MEM_REQUEST_FRAME),&addr);
	if(ret<=0)
	{
		printf("[UI DEBUG]send member request fail\r\n");
	}
	printf("send member request \r\n");
	sleep(1);
	close(s_request);
#endif
    
}
/* 处理网内成员信息单播包 */
void process_member_info(char* info,int size)
{
    if(info==NULL || size <=0)
        return;
    int reply_s;
    int ret=0;
    char dest_ip[20];
    memset(dest_ip,0,sizeof(dest_ip));

    MEM_REQUEST_FRAME *request_info=(MEM_REQUEST_FRAME*)info;

	if(ntohs(request_info->head)!=0x4c4a)
	{
		printf("[ERROR] member request head error\r\n");
		return;
	}

    MEM_REPLY_FRAME reply_info;
	memset(&reply_info,0,sizeof(MEM_REPLY_FRAME));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));	

    switch(request_info->type)
	{
		case 0:
		/* 回复成员信息单播请求 */

			printf("recv member request info\r\n");
			reply_info.head=htons(0x4c4a);
			reply_info.type=1;
			reply_info.src_id=SELFID;
			reply_info.dst_id=request_info->src_id;
			reply_info.tail=htons(0x6467);

			read_xwg_info((void*)&reply_info.member,sizeof(NodeBasicInfo));

			snprintf(dest_ip, sizeof(dest_ip), "192.168.2.%d", request_info->src_id);

			reply_s=createUdpClient(&addr,dest_ip,MEM_REQUEST_PORT);
			ret=SendUDPClient(reply_s,(char*)&reply_info,sizeof(MEM_REPLY_FRAME),&addr);
			if(ret<=0)
			{
				printf("[ERROR]send member reply info fail\r\n");
			}
			sleep(1);
			close(reply_s);
		break;
		case 1:
		/* 收到回复，直接上报0x0A */
			printf("recv member reply info\r\n");
			memcpy(&reply_info,info,size);
			//增加帧头帧尾判断
			Send_0x0A(ui_fd,(void*)info,size);

		break;
		
		default:
		break;
	}



}
/* 处理工控屏指令 */
uint16_t  reply_uart_info(int fd,void* ack_info, uint16_t len)
{
	return write(fd,ack_info,len);
}

uint16_t CRC_Check(uint8_t *CRC_Ptr,uint16_t LEN)
{
    uint16_t CRC_Value = 0xffff;
    for(int  i=0;i<LEN;i++)
    {
        CRC_Value ^= CRC_Ptr[i];
        for(int j=0;j<8;j++)
        {
            if(CRC_Value & 0x0001)
                CRC_Value = (CRC_Value>>1)^0xA001;
            else
                CRC_Value = (CRC_Value >> 1);
        }
    }
    CRC_Value = ((CRC_Value >>8)| (CRC_Value <<8));
    return CRC_Value;
}

void set_opt(int fd,int bspeed,int dbits,int parity,int stopbit)
{
    struct termios newtio,oldtio;
    if(tcgetattr(fd,&oldtio) != 0)
    {
        perror("tcgetattr");
        exit(1);
    }

    bzero(&newtio,sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD; //本地连接，接收使能
    newtio.c_cflag &= ~CSIZE; //屏蔽数据位
    newtio.c_lflag = 0;
    newtio.c_oflag = 0;

    //设置波特率
    switch(bspeed)
    {
        case 460800:
            cfsetispeed(&newtio, B460800);
            cfsetospeed(&newtio, B460800);
            break;
        case 115200:
            cfsetispeed(&newtio,B115200);
            cfsetospeed(&newtio,B115200);
            break;
        case 9600:
            cfsetispeed(&newtio,B9600);
            cfsetospeed(&newtio,B9600);
            break;
        case 4800:
            cfsetispeed(&newtio,B4800);
            cfsetospeed(&newtio,B4800);
            break;
        case 2400:
            cfsetispeed(&newtio,B2400);
            cfsetospeed(&newtio,B2400);
            break;
        default:
            cfsetispeed(&newtio,B9600);
            cfsetospeed(&newtio,B9600);
            break;
    }
    //设置数据位
    switch(dbits){
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }
    //设置校验位
    switch(parity){
        case 'n':
        case 'N':
            newtio.c_cflag &= ~PARENB; //无校验
            newtio.c_iflag &= ~(IXON|IXOFF|IXANY|BRKINT|ICRNL|ISTRIP);
            break;
        case 'o':
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD; //奇校验
            newtio.c_iflag |= (INPCK|ISTRIP); //启用输入校验
            break;
        case 'e':
        case 'E':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD; //偶校验
            newtio.c_iflag |= (INPCK|ISTRIP);
            break;
    }
    //设置停止位
    if(stopbit == 1)
        newtio.c_cflag &= ~CSTOPB; //1位停止位
    else if(stopbit == 2)
        newtio.c_cflag |= CSTOPB; //2位停止位


    newtio.c_cc[VTIME] = 1; //设置等待时间
    newtio.c_cc[VMIN] = 0; //允许短帧返回，避免阻塞等待200字节
    //设置新属性，旧属性不再使用
    tcflush(fd,TCIFLUSH); //清空输入缓冲区
    if((tcsetattr(fd,TCSANOW,&newtio)) != 0)
    {        
        perror("com set error");
        exit(1);
    }

}
//配置参数和向内核下发参数
void process_cmd_info(uint32_t cmd_addr,uint32_t cmd_value)
{
    int ret=0;
	uint32_t t_freq=0;


	bool isset =FALSE;//标志位，修改为ture的参数会下发到内核
    uint8_t cmd[200];

    INT8 buffer[sizeof(Smgmt_header) + sizeof(Smgmt_set_param)];
    INT32 buflen = sizeof(Smgmt_header)+sizeof(Smgmt_set_param);
    Smgmt_header* mhead = (Smgmt_header*)buffer;
    Smgmt_set_param* mparam = (Smgmt_set_param*)mhead->mgmt_data;

    bzero(buffer,buflen);
    memset(cmd,0,sizeof(cmd));
    mhead->mgmt_head = htons(HEAD);
    mhead->mgmt_len = sizeof(Smgmt_set_param);
    mhead->mgmt_type = 0;

    /*static wg param init*/
    static uint8_t s_sync_mode=0;
	static uint8_t s_workmode=WORK_MODE_TYPE_DP;
	static uint8_t s_kylb=1;   //默认关闭
	static uint8_t s_bw=0;
	static uint8_t s_mcs=0;   //调制方式
	static uint8_t s_transmode=0;
	static uint32_t s_freq,s_select_freq1,s_select_freq2,s_select_freq3,s_select_freq4;
	s_freq=FREQ_INIT;

    switch(cmd_addr)
    {
        case PARAM_0A_REQUEST_ADDR://0a命令 节点详细信息
            send_member_request((uint8_t)cmd_value);
            break;
        case PARAM_OP_MODE_CURRENT_WORK_MODE:// 当前工作模式
            printf("[UI DEBUG] transmode :%d",cmd_value);
            isset=TRUE;
            mhead->mgmt_type |=MGMT_SET_TEST_MODE;
            mparam->mgmt_mac_work_mode=htons((uint16_t)cmd_value);
            s_transmode=cmd_value;
            break;
        case PARAM_OP_MODE_SPATIAL_FILTERING:// 空域滤波
            printf("[UI DEBUG] kylb :%d \r\n", (uint8_t)cmd_value);
            s_kylb=cmd_value;
            if(cmd_value == 0)
            {
                if(s_workmode==WORK_MODE_TYPE_ZSYXP)
                {
                    printf("DT workmode is zsyxp,fail to set kylb\r\n");
                    break;
                }   
                isset=TRUE;
                mhead->mgmt_type |=MGMT_SET_WORKMODE;
                mparam->mgmt_net_work_mode.NET_work_mode=WORK_MODE_TYPE_KYLB;
                sprintf(cmd,
                        "sed -i \"s/kylb .*/kylb %d/g\" /etc/node_xwg",
                    KYLB_MODE_OPEN);
                system(cmd);
            }
            else
            {
                sprintf(cmd,
                        "sed -i \"s/kylb .*/kylb %d/g\" /etc/node_xwg",
                    KYLB_MODE_CLOSE);
                system(cmd);
            }
            break;
        case PARAM_OP_MODE_SYNC_MODE:// 内外同步模式
            s_sync_mode=cmd_value;
            sprintf(cmd,
                    "sed -i \"s/sync_mode .*/sync_mode %d/g\" /etc/node_xwg",
                    cmd_value);
            system(cmd);
            break;
        case PARAM_CH1_FREQ_HOPPING_MODE:// 跳频方式/工作模式
            printf("[UI DEBUG] work mode: %d \r\n",cmd_value);
            if(cmd_value == 0)
            {
                printf("[UI DEBUG]set work mode dp \r\n ");
                isset=TRUE;
                mhead->mgmt_type |=MGMT_SET_WORKMODE;
                mparam->mgmt_net_work_mode.NET_work_mode=WORK_MODE_TYPE_DP;
                mhead->mgmt_type |= MGMT_SET_FREQUENCY;
                mparam->mgmt_mac_freq = htonl(s_freq);

                s_workmode=WORK_MODE_TYPE_DP;

                sprintf(cmd,
				"sed -i \"s/workmode .*/workmode %d/g\" /etc/node_xwg",
				WORK_MODE_TYPE_DP);		
			    system(cmd);

                updateData_meshinfo_qk("workmode", WORK_MODE_TYPE_DP);
                updateData_meshinfo_qk("rf_freq", (int)s_freq);
            }
            else if(cmd_value == 1)
            {
                /*自适应选频*/
                if(s_select_freq1==0||s_select_freq2==0||s_select_freq3==0||s_select_freq4==0)
                {
                    printf("[UI DEBUG] set zsyxp error , freq info :%d %d %d %d \r\n",s_select_freq1,
                    s_select_freq2,s_select_freq3,s_select_freq4);
                    break;
                }
                printf("[UI DEBUG]set work mode zsyxp \r\n ");

                isset=TRUE;
                mhead->mgmt_type |= MGMT_SET_WORKMODE;
                mparam->mgmt_net_work_mode.NET_work_mode=WORK_MODE_TYPE_ZSYXP;

                mparam->mgmt_net_work_mode.fh_len=4;
                mparam->mgmt_net_work_mode.hop_freq_tb[0]=s_select_freq1;
                mparam->mgmt_net_work_mode.hop_freq_tb[1]=s_select_freq2;
                mparam->mgmt_net_work_mode.hop_freq_tb[2]=s_select_freq3;
                mparam->mgmt_net_work_mode.hop_freq_tb[3]=s_select_freq4;

                s_workmode=WORK_MODE_TYPE_ZSYXP;
                sprintf(cmd, "sed -i \"s/workmode .*/workmode %d/; \
						s/select_freq1 .*/select_freq1 %d/; \
						s/select_freq2 .*/select_freq2 %d/; \
						s/select_freq3 .*/select_freq3 %d/; \
						s/select_freq4 .*/select_freq4 %d/\" /etc/node_xwg", 
					WORK_MODE_TYPE_ZSYXP, s_select_freq1, s_select_freq2, s_select_freq3, s_select_freq4);
			system(cmd);

                updateData_meshinfo_qk("workmode", WORK_MODE_TYPE_ZSYXP);
                updateData_meshinfo_qk("m_select_freq1", (int)s_select_freq1);
                updateData_meshinfo_qk("m_select_freq2", (int)s_select_freq2);
                updateData_meshinfo_qk("m_select_freq3", (int)s_select_freq3);
                updateData_meshinfo_qk("m_select_freq4", (int)s_select_freq4);

            }
            break;
        case PARAM_CH1_ROUTING_PROTOCOL:// 路由协议
            printf("[UI DEBUG] router :%d \r\n", (uint8_t)cmd_value);	
            switch(cmd_value){
                case 0: //olsr
                    printf("[UI DEBUG] set route olsr \r\n");
                    ret = system("/home/root/cs_olsr.sh");
                    if(ret == -1)printf("change olsr failed\r\n");
                    sprintf(cmd,
					"grep -q '^router ' /etc/node_xwg && sed -i \"s/router .*/router %d/g\" /etc/node_xwg || echo \"router %d\" >> /etc/node_xwg",
					KD_ROUTING_OLSR,
                    KD_ROUTING_OLSR);		
                    system(cmd);
                    break;
                case 1:  //aodv
                    printf("[UI DEBUG] set route aodv \r\n");
                    ret = system("/home/root/cs_aodv.sh");
                    if(ret == -1) printf("change aodv failed\r\n");
                    sprintf(cmd,
						"grep -q '^router ' /etc/node_xwg && sed -i \"s/router .*/router %d/g\" /etc/node_xwg || echo \"router %d\" >> /etc/node_xwg",
						KD_ROUTING_AODV,
                    KD_ROUTING_AODV);		
                    system(cmd);
                    break;
                case 2:  //batman 
                    printf("[UI DEBUG] set route batman \r\n");
                    ret = system("/home/root/cs_batman.sh");
                    if(ret == -1) printf("change batman failed\r\n");
                    sprintf(cmd,
						"grep -q '^router ' /etc/node_xwg && sed -i \"s/router .*/router %d/g\" /etc/node_xwg || echo \"router %d\" >> /etc/node_xwg",
						KD_ROUTING_CROSS_LAYER,
                    KD_ROUTING_CROSS_LAYER);		
                    system(cmd);
                    break;
                default:
                    break;
            }
            break;
        case PARAM_CH1_FIXED_FREQ_CENTER:// 点频-中心频率
            printf("[UI DEBUG]fix freq :%d workmode :%d \r\n", cmd_value/1000,s_workmode);
            s_freq=t_freq=cmd_value/1000;
            if(t_freq<=255) s_freq=255;
            if(t_freq>=2500) s_freq=2500;
            if(s_workmode==WORK_MODE_TYPE_ZSYXP)break;
            isset=TRUE;
            mhead->mgmt_type |= MGMT_SET_WORKMODE;
            mparam->mgmt_net_work_mode.NET_work_mode=WORK_MODE_TYPE_DP;
            mhead->mgmt_type |=MGMT_SET_FREQUENCY;
            mparam->mgmt_mac_freq = htonl(s_freq);

            updateData_systeminfo_qk("rf_freq",s_freq);
            updateData_meshinfo_qk("rf_freq", (int)s_freq);
            break;
        case PARAM_CH1_SELECTED_FREQ_1://通道1设置-工作频率-自适应-中心频率1
            printf("[UI DEBUG]select freq-1 :%d \r\n", cmd_value/1000);
            t_freq=cmd_value/1000;
            s_select_freq1=t_freq;
            if(t_freq<=225) s_select_freq1=225;
            if(t_freq>=2500) s_select_freq1=2500;
            updateData_meshinfo_qk("m_select_freq1", (int)s_select_freq1);
            break;
        case PARAM_CH1_SELECTED_FREQ_2://通道1设置-工作频率-自适应-中心频率2
            printf("[UI DEBUG]select freq-2 :%d \r\n", cmd_value/1000);
            t_freq=cmd_value/1000;
            s_select_freq2=t_freq;
            if(t_freq<=225) s_select_freq2=225;
            if(t_freq>=2500) s_select_freq2=2500;
            updateData_meshinfo_qk("m_select_freq2", (int)s_select_freq2);
            break;
        case PARAM_CH1_SELECTED_FREQ_3://通道1设置-工作频率-自适应-中心频率3
            printf("[UI DEBUG]select freq-3 :%d \r\n", cmd_value/1000);
            t_freq=cmd_value/1000;
            s_select_freq3=t_freq;
            if(t_freq<=225) s_select_freq3=225;
            if(t_freq>=2500) s_select_freq3=2500;
            updateData_meshinfo_qk("m_select_freq3", (int)s_select_freq3);
            break;
        case PARAM_CH1_SELECTED_FREQ_4://通道1设置-工作频率-自适应-中心频率4
            printf("[UI DEBUG]select freq-4 :%d \r\n", cmd_value/1000);
            t_freq=cmd_value/1000;
            s_select_freq4=t_freq;
            if(t_freq<=225) s_select_freq4=225;
            if(t_freq>=2500) s_select_freq4=2500;
            updateData_meshinfo_qk("m_select_freq4", (int)s_select_freq4);
            break;
        case PARAM_CH1_SIGNAL_BANDWIDTH: // 信号带宽
            printf("[UI DEBUG]bw: %d \r\n",cmd_value);
            isset=TRUE;
            mhead->mgmt_type |= MGMT_SET_BANDWIDTH;
            mparam->mgmt_mac_bw=cmd_value;
            break;
        case PARAM_CH1_MODULATION_WIDEBAND:// 宽带mcs
            printf("[UI DEBUG] mcs :%d \r\n",cmd_value);
            isset=TRUE;
            mhead->mgmt_type |=MGMT_SET_UNICAST_MCS;

            s_mcs = cmd_value;
            mparam->mgmt_virt_unicast_mcs = s_mcs;
            updateData_systeminfo_qk("m_rate",mparam->mgmt_virt_unicast_mcs);
            break;
        case PARAM_CH1_TX_POWER_ATTENUATION:// 发射功率衰减
            printf("[UI DEBUG] power_atten: %d \r\n",cmd_value);

            break;
        case PARAM_TXRX_INFO_OPERATION:// 功能-收发消息统计-操作
            printf("[UI DEBUG] 0x06 cmd info operation: %d\r\n",cmd_value);
            stat_info.stat_flag=cmd_value;

            break;
        default:
            break;
    }


    if(isset)
	{
		isset=FALSE;
		mhead->mgmt_type = htons(mhead->mgmt_type);
		mhead->mgmt_keep = htons(mhead->mgmt_keep);
		mgmt_netlink_set_param(buffer, buflen,NULL);	
		sleep(1);
		if (!persist_test_db()) {
			printf("[ui_get] persist test.db failed after UI command\n");
		}
	}

}

//将串口数据解析，分发。
void process_uart_info(int fd,char *info,int len){
    uint8_t uart_buf[MAX_UI_SIZE];
    uint8_t cmd_type = 0;
	uint16_t cmd_len = 0;
	uint8_t  param_len = 0;

    UART_FRAME_1_BYTE recv_frame_1;
	UART_FRAME_2_BYTE recv_frame_2;
	UART_FRAME_4_BYTE recv_frame_4;


	memset(&recv_frame_1,0,sizeof(UART_FRAME_1_BYTE));
	memset(&recv_frame_2,0,sizeof(UART_FRAME_2_BYTE));
	memset(&recv_frame_4,0,sizeof(UART_FRAME_4_BYTE));

    if(info == NULL || len <=0)
    {
        printf("ERROR:Uart info is null\r\n");
        return;
    }

    memset(uart_buf,0,MAX_UI_SIZE);
    memcpy(uart_buf,info,len);
    
    cmd_type = uart_buf[2];

    if(cmd_type == 0x0a)
    {
        process_cmd_info(PARAM_0A_REQUEST_ADDR,uart_buf[4]);
        return;
    }

    if (cmd_type != 0x01) {
        return;
    }
    
    cmd_len = uart_buf[4];

    uint32_t addr = (uart_buf[5]<<24) | (uart_buf[6]<<16) | (uart_buf[7]<<8) | uart_buf[8];
    //addr = htonl(addr);  ssq rm 
    param_len = cmd_len -1-4; 

    switch(param_len)
    {
        case 1:
            memcpy(&recv_frame_1,info,len);
            printf("[UART DEBUG] Recv 1-byte CMD. Addr=0x%08X Val=0x%02X\n", addr, recv_frame_1.value);
            process_cmd_info(addr,recv_frame_1.value);
            //ack
            recv_frame_1.cmd_no = 0x02;
            recv_frame_1.ack_flag = MESSAGE_TYPE_REPLY;
            recv_frame_1.crc = htons(CRC_Check(&recv_frame_1.cmd_no,sizeof(recv_frame_1)-6));
            reply_uart_info(fd,(void*)&recv_frame_1,sizeof(recv_frame_1));
            break;

        case 2:
            memcpy(&recv_frame_2,info,len);
            {
                uint16_t val_host = ntohs(recv_frame_2.value);
                printf("[UART DEBUG] Recv 2-byte CMD. Addr=0x%08X Val=0x%04X\n", addr, val_host);
                process_cmd_info(addr, val_host);
            }
            //ack
            recv_frame_2.cmd_no = 0x02;
            recv_frame_2.ack_flag = MESSAGE_TYPE_REPLY;
            recv_frame_2.crc = htons(CRC_Check(&recv_frame_2.cmd_no,sizeof(recv_frame_2)-6));
            reply_uart_info(fd,(void*)&recv_frame_2,sizeof(recv_frame_2));
            break;

        case 4:
            memcpy(&recv_frame_4,info,len);
            {
                uint32_t val_host = ntohl(recv_frame_4.value);
                printf("[UART DEBUG] Recv 4-byte CMD. Addr=0x%08X Val=0x%08X\n", addr, val_host);
                process_cmd_info(addr, val_host);
            }
            //ack
            recv_frame_4.cmd_no = 0x02;
            recv_frame_4.ack_flag = MESSAGE_TYPE_REPLY;
            recv_frame_4.crc = htons(CRC_Check(&recv_frame_4.cmd_no,sizeof(recv_frame_4)-6));
            reply_uart_info(fd,(void*)&recv_frame_4,sizeof(recv_frame_4));
            break;

        default:
            break;
    }


}


//从串口读到消息
void get_ui_info(int fd)
{
    int ui_Fd;
    uint8_t rx_buf[1024];
    int rx_len = 0;// 当前缓存区里积攒的有效数据长度

    uint8_t temp_buf[1024];

    int len;// 每次read的返回长度
    ui_Fd = fd;

    while(1)
    {
        len = read(ui_Fd,temp_buf,sizeof(temp_buf));
        if(len > 0)
        {
            printf("[UART DEBUG] Read %d bytes from UART\n", len);

            if(rx_len + len > (int)sizeof(rx_buf)){
                printf("[UART ERROR] RX Buffer Overflow!r\r\n");
                rx_len=0;
            }
            memcpy(rx_buf + rx_len, temp_buf, (size_t)len);
            rx_len+=len;
            while(rx_len >= 9)
            {
                //当发现这个数据的前两个字节不是帧头 D5 5D，就丢掉这个字节，继续往后找，直到找到为止
                if(rx_buf[0]!=0xD5 || rx_buf[1] !=0x5D){
                    rx_len--;
                    memmove(rx_buf, rx_buf + 1, rx_len);
                    continue;
                }

                uint8_t cmd_no = rx_buf[2];
                uint16_t expected_total_len = 0;
                uint16_t crc_calc_len = 0;

                if (cmd_no == 0x0A) {
                    /* 0x0A请求帧固定9字节：D5 5D 0A FF <node_id> CRC_H CRC_L 5D D5 */
                    expected_total_len = 9;
                    crc_calc_len = 3;
                } else {
                    uint16_t cmd_len = rx_buf[4];
                    expected_total_len = 8 + cmd_len;
                    crc_calc_len = expected_total_len - 6;
                }

                //处理半包问题
                if(rx_len < expected_total_len) {
                    break; // 不够，跳出 while，等下一次 read 追加
                }

                uint16_t recv_crc = ((uint16_t)rx_buf[expected_total_len - 4] << 8) |
                                    (uint16_t)rx_buf[expected_total_len - 3];
                uint16_t calc_crc = CRC_Check(&rx_buf[2], crc_calc_len);

                if(recv_crc == calc_crc && rx_buf[expected_total_len-2] == 0x5D && rx_buf[expected_total_len-1] == 0xD5) 
                {
                    // 校验完美！剥离出一包绝对纯净的数据，交给业务层！
                    process_uart_info(ui_Fd, (char*)rx_buf, expected_total_len);
                }
                else 
                {
                    printf("[UART ERROR] CRC Failed or Bad Tail. Dropping fake header.\n");
                    expected_total_len = 2; // 是假帧头，只丢掉 D5 5D
                }

                //处理粘包问题
                rx_len -= expected_total_len;
                if(rx_len > 0) {
                    memmove(rx_buf, rx_buf + expected_total_len, rx_len);
                }


            }
        }
    }


}

int uart_init(void)
{
    int ui_Fd = -1;
    uint8_t cnt =0;

    while(cnt < MAX_RETRY_COUNT)
    {
        ui_Fd = open(FD_UI_UART, O_RDWR);
        if(ui_Fd < 0)
        {
            printf("Failed to open UART %s, retrying... (%d/%d), errno=%d(%s)\n",
                   FD_UI_UART,
                   cnt + 1,
                   MAX_RETRY_COUNT,
                   errno,
                   strerror(errno));
            cnt++;
            sleep(1); // 等待一段时间后重试
        }
        else
        {
            break;
        }
    }
    if(ui_Fd < 0)
    {
        printf("ERROR:open ui uart fail, dev=%s\r\n", FD_UI_UART);
		return -1;
    }

    printf("[UI DEBUG] uart fd: %d\r\n", ui_Fd);

    set_opt(ui_Fd, UI_UART_BAUD, 8, 'N', 1);

    return ui_Fd;
}
//读串口线程，监控工控屏的动作
void get_ui_Thread(void *arg)
{
    int fd = (int)arg;
    printf("create thread: read uart info uart fd %d \r\n",fd);
    //死循环读
    while (1)
    {
        get_ui_info(fd);

        usleep(50000);
    }
    
}
//向串口写消息
void write_ui_Thread(void *arg)
{
    //发给工屏的，用xwg里面的读
    int ui_Fd=(int )arg;
	printf("create thread: report uart  info uart fd :%d \r\n",ui_Fd);

	struct mgmt_send self_msg;
	memset(&self_msg,0,sizeof(self_msg));
    int last_router = KD_ROUTING_OLSR;
    int current_router = KD_ROUTING_OLSR;

    Node_Xwg_Pairs param_pairs[MAX_XWG_PAIRS] = {
        {"channel", 0, 0},{"power", 0, 0},{"bw", 0, 0},{"mcs", 0, 0},
        {"macmode", 0, 0},{"slotlen", 0, 0},{"router", 0, 0},{"workmode", 0, 0},
        {"select_freq1", 0, 0},{"select_freq2", 0, 0},{"select_freq3", 0, 0},{"select_freq4", 0, 0},
		{"sync_mode",0,0},{"kylb",0,0}
		// {"longitude ",0,0},{"latitude  ",0,0},{"altitude ",0,0},
    };

    read_node_xwg_file("/etc/node_xwg",param_pairs,MAX_XWG_PAIRS);
	last_router = get_int_value((void*)param_pairs,"router");
	if (last_router < KD_ROUTING_OLSR || last_router > KD_ROUTING_CROSS_LAYER) {
		last_router = KD_ROUTING_OLSR;
	}

    Send_0x04(ui_Fd,(void*)&param_pairs,sizeof(Node_Xwg_Pairs)*MAX_XWG_PAIRS);
    Send_0x08(ui_Fd,(void*)&param_pairs,sizeof(Node_Xwg_Pairs)*MAX_XWG_PAIRS);
    memset(&stat_info,0,sizeof(Info_0x06_Statistics));

	while(1)
	{

		read_node_xwg_file("/etc/node_xwg",param_pairs,MAX_XWG_PAIRS);
        current_router = get_int_value((void*)param_pairs,"router");
        if (current_router < KD_ROUTING_OLSR || current_router > KD_ROUTING_CROSS_LAYER) {
            current_router = KD_ROUTING_OLSR;
        }
	 	Send_0x08(ui_Fd,(void*)&param_pairs,sizeof(Node_Xwg_Pairs)*MAX_XWG_PAIRS);

        if (current_router != last_router) {
            printf("[UI INFO] router changed %d -> %d, send 0x04 sync\r\n", last_router, current_router);
            Send_0x04(ui_Fd,(void*)&param_pairs,sizeof(Node_Xwg_Pairs)*MAX_XWG_PAIRS);
            last_router = current_router;
        }

        if (mgmt_netlink_get_info(0, MGMT_CMD_GET_VETH_INFO, NULL, (char*)&self_msg) == NULL) {
            printf("[UI WARN] get veth info failed, skip 0x07/0x09 report this cycle\r\n");
            sleep(1);
            continue;
        }
		//printf("send cmd 05 07 09 \r\n");
		Send_0x05(ui_Fd,&g_radio_param);// 发送 0x05 设备基础信息(IP/GPS等)
		//sleep(1);

		Send_0x06(ui_Fd,(void*)&stat_info);// 发送 0x06 流量统计信息
		Send_0x07(ui_Fd,&self_msg.amp_infomation);    //0x07 自检
		//sleep(1);

		Send_0x09(ui_Fd,&self_msg);// 发送 0x09 邻居拓扑信息
		// //sleep(1);

		sleep(1);
	}


}
