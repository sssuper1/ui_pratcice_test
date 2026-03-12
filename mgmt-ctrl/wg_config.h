#ifndef __WG_CONFIG_H
#define __WG_CONFIG_H


#include "stdint.h"

#define SC_REPORT_PORT    10001   //自检状态上报端口
#define STATUS_REPORT_PORT  10001   //参数状态信息上报端口

#define MAX_XWG_PAIRS       20

#pragma pack(push, 1)

typedef enum
{
    KYLB_MODE_OPEN=0,
    KYLB_MODE_CLOSE=1
}KYLB_MODE;


/* 路由协议类型 */
typedef enum {
    KD_ROUTING_OLSR    = 1,  // OLSR协议
    KD_ROUTING_AODV    = 2,  // AODV协议
    KD_ROUTING_CROSS_LAYER = 3  // 跨层抗干扰路由协议
} kd_routing_protocol_t;


//设备自检状态信息数据
typedef struct{
    /* 综合模块状态 */
    int8_t temperature;               /* 综合模块温度 有符号char型。精度为℃，取值范围-127到﹢127    */
    uint8_t voltage;                   /* 综合模块电压  相对12V的偏移量，LSB表示0.1V，有符号数。例如，0x01表示12.1V，0x81表示11.9V*/
    uint8_t fan_status;                /* 综合模块风机转速状态 取值0-100，表示当前为0-100%*/
    uint8_t nav_lock_status;           /* 综合模块卫导锁定状态 0x00：已锁定，0x01：未锁定*/
    uint8_t sync_status;               /* 综合模块内外同步状态 0x00：外同步  0x01：内同步 */
    uint8_t clock_switch_status;       /* 综合模块内外时钟切换状态 0x00：板载时钟 0x01：外供时钟 */
    uint16_t panel_rs232_rx_count;      /* 综合模块与面板RS232接收消息记数 无符号数，取值范围0-65535  */
    uint16_t panel_rs232_tx_count;     /* 综合模块与面板RS232发送消息记数 */
    uint16_t module_power_rs422_rx_count;     /* 综合模块与电源变换RS422接收消息记数 无符号数，取值范围0-65535。*/
    uint16_t module_power_rs422_tx_count;     /* 综合模块与电源变换RS422发送消息记数 */
    uint16_t module_freq_rs422_rx_count;      /* 综合模块与频率源RS422接收消息记数 无符号数，取值范围0-65535*/
    uint16_t module_freq_rs422_tx_count;      /* 综合模块与频率源RS422发送消息记数 */
    uint16_t rf_rs422_rx_count;        /* 综合模块与射频前端RS422接收消息记数 无符号数，取值范围0-65535*/
    uint16_t rf_rs422_tx_count;        /* 综合模块与射频前端RS422发送消息记数 */
    uint8_t soc1_online;              /* 综合模块SOC1在线 0x00：程序未正常运行，0x01：程序正常运*/
    uint8_t soc2_online;               /* 综合模块SOC2在线 */
    uint8_t soc3_online;               /* 综合模块SOC3在线 */
    
    /* ADC/DAC状态 */
    uint8_t adc_ch1_status;            /* 综合模块通道1ADC状态 0x00：正常，0x01：异常*/
    uint8_t adc_ch2_status;            /* 综合模块通道2ADC状态 */
    uint8_t adc_ch3_status;            /* 综合模块通道3ADC状态 */
    uint8_t adc_ch4_status;            /* 综合模块通道4ADC状态 */
    uint8_t dac_ch1_status;            /* 综合模块通道1DAC状态 */
    uint8_t dac_ch2_status;            /* 综合模块通道2DAC状态 */
    uint8_t dac_ch3_status;            /* 综合模块通道3DAC状态 */
    uint8_t dac_ch4_status;            /* 综合模块通道4DAC状态 */
    uint8_t sense_adc_status;          /* 综合模块感知通道ADC状态  */
    
    /* 频率源状态 */
    uint8_t freq_power_fault;          /* 频率源上电/故障指示    无符号char，1表示故障，0表示正常，默认为0。*/
    uint8_t freq_ref1_ready;           /* 频率源输出参考时钟1就绪 无符号char，1表示就绪，0表示失锁，默认为0*/
    uint8_t freq_ref2_ready;           /* 频率源输出参考时钟2就绪 */
    uint8_t freq_sense_status;         /* 频率源通信感知状态指示 */
    uint8_t freq_12v_voltage;          /* 频率源12V供电电压 对12V的偏移量，LSB表示0.1V，有符号数。例如，0x01表示12.1V，0x81表示11.9V。*/
    int8_t freq_temperature;            /* 频率源温度 有符号char型。精度为℃，取值范围-127到﹢127*/
    uint16_t freq_rs422_rx_count;      /* 频率源RS422接收消息记数 */
    uint16_t freq_rs422_tx_count;      /* 频率源RS422发送消息记数 */
    uint16_t freq_word_count;          /* 频率源频率字下发次数记数 */
    uint16_t freq_pulse_count;         /* 频率源频率更新脉冲下发次数记数 */
    uint8_t freq_ch_power_status;      /* 频率源各通道加电状态指示 */
    uint8_t freq_lo_ready;             /* 频率源各通道输出本振就绪 */
    uint16_t freq_lo1_freq;            /* 频率源本振1对应频点 */
    uint16_t freq_lo2_freq;            /* 频率源本振2对应频点 */
    uint16_t freq_lo3_freq;            /* 频率源本振3对应频点 */
    uint16_t freq_lo4_freq;            /* 频率源本振4对应频点 */
    
    /* 电源变换状态 */
    uint8_t power_power_fault;         /* 电源变换上电/故障指示 */
    uint8_t power_temperature;         /* 电源变换温度 */
    uint16_t power_ac220_power;        /* 电源变换AC220输入功耗 */
    uint16_t power_dc24v_power;        /* 电源变换DC24V输入功耗 */
    uint16_t power_rs422_rx_count;     /* 电源变换RS422接收消息记数 */
    uint16_t power_rs422_tx_count;     /* 电源变换RS422发送消息记数 */
    uint8_t power_ch_power_status;     /* 电源变换各路加电控制状态 */
    uint8_t power_fault_status;        /* 电源变换各路输出故障指示 */
    uint8_t power_overload_status;     /* 电源变换各路过载状态指示 */
    
    /* 射频前端状态 */
    uint8_t rf_power_fault;            /* 射频前端上电/故障指示 */
    uint8_t rf_28v_voltage;            /* 射频前端28V供电电压 */
    uint8_t rf_12v_voltage;            /* 射频前端12V供电电压 */
    uint8_t rf_tx_power_status;        /* 射频前端发射功率检波状态 */
    uint8_t rf_antenna_l_vswr;         /* 射频前端天线L口驻波状态 */
    uint8_t rf_antenna_h1_vswr;        /* 射频前端天线H-1口驻波状态 */
    uint8_t rf_antenna_h2_vswr;        /* 射频前端天线H-2口驻波状态 */
    uint8_t rf_tx_rx_status;           /* 射频前端收发状态指示 */
    uint8_t rf_antenna_select;         /* 射频前端天线选择控制状态 */
    uint8_t rf_sense_status;           /* 射频前端通信感知状态 */
    uint8_t rf_power_status;           /* 射频前端加电状态指示 */
    uint8_t rf_ch_power_level;         /* 射频前端各通道功率等级 */
    
    /* 射频前端通道1 */
    uint8_t rf_ch1_rf_power;           /* 射频前端通道1接收射频功率检波 */
    uint8_t rf_ch1_if_power;           /* 射频前端通道1接收中频功率检波 */
    uint8_t rf_ch1_temp1;              /* 射频前端通道1温度1 */
    uint16_t rf_ch1_freq;              /* 射频前端通道1当前工作频点 */
    uint8_t rf_ch1_agc_atten;          /* 射频前端通道1当前AGC衰减值 */
    uint16_t rf_ch1_rs422_rx_count;    /* 射频前端通道1RS422接收消息记数 */
    uint16_t rf_ch1_rs422_tx_count;    /* 射频前端通道1RS422发送消息记数 */
    uint16_t rf_ch1_freq_word_count;   /* 射频前端通道1频率字下发次数记数 */
    uint16_t rf_ch1_agc_count;         /* 射频前端通道1AGC控制下发次数记数 */
    uint16_t rf_ch1_freq_pulse_count;  /* 射频前端通道1频率更新脉冲下发次数记数 */
    uint16_t rf_ch1_power_consumption; /* 射频前端通道1通道功耗 */
    uint8_t rf_ch1_bandwidth;          /* 射频前端通道1当前信号带宽 */
    uint8_t rf_ch1_power_adjust;       /* 射频前端通道1功率调整量 */
    
    /* 射频前端通道2 */
    uint8_t rf_ch2_rf_power;
    uint8_t rf_ch2_if_power;
    uint8_t rf_ch2_temp1;
    uint16_t rf_ch2_freq;
    uint8_t rf_ch2_agc_atten;
    uint16_t rf_ch2_rs422_rx_count;
    uint16_t rf_ch2_rs422_tx_count;
    uint16_t rf_ch2_freq_word_count;
    uint16_t rf_ch2_agc_count;
    uint16_t rf_ch2_freq_pulse_count;
    uint16_t rf_ch2_power_consumption;
    uint8_t rf_ch2_bandwidth;
    uint8_t rf_ch2_power_adjust;
    
    /* 射频前端通道3 */
    uint8_t rf_ch3_rf_power;
    uint8_t rf_ch3_if_power;
    uint8_t rf_ch3_temp1;
    uint16_t rf_ch3_freq;
    uint8_t rf_ch3_agc_atten;
    uint16_t rf_ch3_rs422_rx_count;
    uint16_t rf_ch3_rs422_tx_count;
    uint16_t rf_ch3_freq_word_count;
    uint16_t rf_ch3_agc_count;
    uint16_t rf_ch3_freq_pulse_count;
    uint16_t rf_ch3_power_consumption;
    uint8_t rf_ch3_bandwidth;
    uint8_t rf_ch3_power_adjust;
    
    /* 射频前端通道4 */
    uint8_t rf_ch4_rf_power;
    uint8_t rf_ch4_if_power;
    uint8_t rf_ch4_temp1;
    uint16_t rf_ch4_freq;
    uint8_t rf_ch4_agc_atten;
    uint16_t rf_ch4_rs422_rx_count;
    uint16_t rf_ch4_rs422_tx_count;
    uint16_t rf_ch4_freq_word_count;
    uint16_t rf_ch4_agc_count;
    uint16_t rf_ch4_freq_pulse_count;
    uint16_t rf_ch4_power_consumption;
    uint8_t rf_ch4_bandwidth;
    uint8_t rf_ch4_power_adjust;
    
    /* 电池状态 */
    uint8_t battery_level;             /* 电池当前电量 */
    uint8_t battery_self_test;         /* 电池自检状态 */
    uint8_t battery_voltage;           /* 电池当前电压 */
    uint8_t battery_temperature;       /* 电池温度 */
    uint16_t battery_rs422_rx_count;   /* 电池RS422接收消息记数 */
    uint16_t battery_rs422_tx_count;   /* 电池RS422发送消息记数 */
    
    //uint8_t reserved;              /* 预留 */
}DEVICE_SC_STATUS_REPORT;
/* xwg 内容 */ 
typedef struct {
    char key[25];
    char value[100];
    int found;
} Node_Xwg_Pairs;

#endif // !1