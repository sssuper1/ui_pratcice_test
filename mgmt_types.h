#ifndef MGMT_TYPES_H_
#define MGMT_TYPES_H_
#include <stdint.h>
#include <asm/byteorder.h>
#include <linux/types.h>
#include <linux/if_ether.h>

#include "wg_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define LINUX

#ifdef LINUX
#include <netinet/in.h>
#define PACKED __attribute__((packed))
#endif

#define Radio_SWARM_S2
#define Radio_QK 


#define MGMT_NL_NAME "lumgmt"


typedef int INT32;
typedef short INT16;
typedef char INT8;

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;

#define HEAD 0x4C4A

#define RETURN_OK 0
#define RETURN_FAILED -1
#define RETURN_NULL -2
#define RETURN_TIMEOUT -3
#define TCPCONNECTNUM 6
#define TIME_WAIT_FOR_EVER 0
#define NUMBERNULL -1

#define HOP_FREQ_NUM 32

#define POWER_CHANNEL_NUM 4
#define NET_SIZE 56   


typedef enum
{
	FALSE = 0,
	TRUE = 1
}BOOL;

typedef enum EMGMT_HEADER_PHY_TYPE {
	MGMT_SET_SLOTLEN = 0x0001,           // 设置时隙长度
	MGMT_SET_POWER_LEVEL = 0x0002,       // 设置功率等级
	MGMT_SET_POWER_ATTENUATION = 0x0004, // 设置功率衰减
	MGMT_SET_RX_CHANNEL_MODE = 0x0008    // 设置接收通道模式
} Emgmt_header_phy_type;

typedef enum EMGMT_HEADER_TYPE {
	MGMT_LOGIN = 0x0000,
	MGMT_SET_ID = 0x01,
	MGMT_SET_INTERVAL = 0x02,
	MGMT_SET_TTL = 0x04,
	MGMT_SET_QUEUE_NUM = 0x08,
	MGMT_SET_QUEUE_LENGTH = 0x10,
	MGMT_SET_QOS_STATEGY = 0x20,
	MGMT_SET_UNICAST_MCS = 0x40,
	MGMT_SET_MULTICAST_MCS = 0x80,
	MGMT_SET_FREQUENCY = 0x100,
	MGMT_SET_POWER = 0x200,
	MGMT_SET_BANDWIDTH = 0x400,
	MGMT_SET_TEST_MODE = 0x800,
	MGMT_SET_TEST_MODE_MCS = 0x1000,
	MGMT_SET_PHY = 0x2000,
	MGMT_SET_WORKMODE = 0x4000,
	MGMT_SET_IQ_CATCH = 0x8000,

} Emgmt_header_type;

enum mgmt_nl_commands {
	MGMT_CMD_UNSPEC,
	MGMT_CMD_GET_ROUTE_INFO,
	MGMT_CMD_GET_VETH_INFO,
	MGMT_CMD_SET_PARAM,
	MGMT_CMD_NODEID,


	//route
	MGMT_CMD_VERSION_ROUTE,


	//veth
	MGMT_CMD_VERSION_VETH,
	MGMT_CMD_VETH_ADDRESS,
	MGMT_CMD_VETH_TX,
	MGMT_CMD_VETH_RX,
	//mac phy
	MGMT_CMD_VERSION_MAC_PHY,
	MGMT_CMD_FREQ,
	MGMT_CMD_BW,
	MGMT_CMD_TXPOWER,
	/* add new commands above here */
	__MGMT_CMD_AFTER_LAST,
	MGMT_CMD_MAX = __MGMT_CMD_AFTER_LAST - 1
};

enum mgmt_nl_attrs {
	MGMT_ATTR_UNSPEC,                // 未指定属性
	MGMT_ATTR_GET_INFO,              // 获取信息请求属性
	MGMT_ATTR_SET_PARAM,             // 设置参数属性
	MGMT_ATTR_NODEID,                // 节点ID属性


	//route
	MGMT_ATTR_VERSION_ROUTE,         // 路由模块版本
	MGMT_ATTR_PKTNUMB_ROUTE,         // 路由相关包计数
	MGMT_ATTR_TABLE_ROUTE,           // 路由表内容
	MGMT_ATTR_UCDS_ROUTE,            // 路由UCDS信息
	//	MGMT_ATTR_MESH_IFINDEX,
	//veth
	MGMT_ATTR_VERSION_VETH,          // 虚拟网卡模块版本
	MGMT_ATTR_INFO_VETH,             // 虚拟网卡统计信息
	//	MGMT_ATTR_INFO_VETH2,

	MGMT_ATTR_VETH_ADDRESS,          // 虚拟网卡地址信息
	MGMT_ATTR_VETH_TX,               // 虚拟网卡发送统计
	MGMT_ATTR_VETH_RX,               // 虚拟网卡接收统计

	//mac phy
	MGMT_ATTR_VERSION_MAC_PHY,       // MAC/PHY模块版本
	MGMT_ATTR_MSG_MAC,               // MAC层消息信息


	MGMT_ATTR_FREQ,                  // 当前频率
	MGMT_ATTR_BW,                    // 当前带宽
	MGMT_ATTR_TXPOWER,               // 当前发射功率

	//set
	//mgmt
	MGMT_ATTR_SET_NODEID,            // 设置节点ID
	//route
	MGMT_ATTR_SET_INTERVAL,          // 设置路由广播间隔
	MGMT_ATTR_SET_TTL,               // 设置路由TTL
	//veth
	MGMT_ATTR_SET_QUEUE_NUM,         // 设置队列数量
	MGMT_ATTR_SET_QUEUE_LENGTH,      // 设置队列长度
	MGMT_ATTR_SET_QOS_STATEGY,       // 设置QoS策略
	MGMT_ATTR_SET_UNICAST_MCS,       // 设置单播MCS
	MGMT_ATTR_SET_MULTICAST_MCS,     // 设置组播MCS
	//mac phy
	MGMT_ATTR_SET_FREQUENCY,         // 设置频率
	MGMT_ATTR_SET_POWER,             // 设置功率
	MGMT_ATTR_SET_BANDWIDTH,         // 设置带宽
	MGMT_ATTR_SET_TEST_MODE,         // 设置测试模式
	MGMT_ATTR_SET_TEST_MODE_MCS,     // 设置测试模式MCS
	MGMT_ATTR_SET_PHY,               // 设置PHY参数
	MGMT_ATTR_SET_WORKMODE,          // 设置工作模式
	MGMT_ATTR_SET_IQ_CATCH,          // 设置IQ抓取参数
	MGMT_ATTR_SET_SLOTLEN,           // 设置时隙长度
	MGMT_ATTR_SET_POWER_LEVEL,       // 设置功率等级
	MGMT_ATTR_SET_POWER_ATTENUATION, // 设置功率衰减
	MGMT_ATTR_SET_RX_CHANNEL_MODE,   // 设置接收通道模式
#ifdef Radio_CEC
    MGMT_ATTR_SET_NCU,
    
#endif

	/* add attributes above here, update the policy in netlink.c */
	__MGMT_ATTR_AFTER_LAST,
	NUM_MGMT_ATTR = __MGMT_ATTR_AFTER_LAST,
	MGMT_ATTR_MAX = __MGMT_ATTR_AFTER_LAST - 1
};

enum{
	FIX_FREQ_MODE = 1,        // 定频模式
	HOP_FREQ_MODE,            // 跳频模式
	COGNITVE_HOP_FREQ_MODE    // 认知跳频模式
};

typedef struct 
{
	uint8_t device_id;
	uint16_t g_txpower;
	uint32_t g_rf_freq;
	uint8_t g_chanbw;
	uint8_t g_rate;
	uint8_t g_bcastmode;
	uint8_t g_workmode;
	uint8_t g_route;
	uint8_t g_slot_len;
	uint16_t g_trans_mode;
	uint32_t g_select_freq_1;	
	uint32_t g_select_freq_2;	
	uint32_t g_select_freq_3;	
	uint32_t g_select_freq_4;	

}Global_Radio_Param;


struct mgmt_msg {
	uint32_t  node_id : 8;
	uint32_t  mcs : 8;
	uint32_t  ucds : 8;
	uint32_t  rssi : 8;
	uint32_t  snr : 8;
	int  time_jitter : 16;
	int  good : 16;
	int  bad : 16;
	uint32_t  noise : 8;
	uint32_t  reserved : 8;
}__attribute__((__packed__));


typedef struct {
	uint8_t  nbr_list[NET_SIZE];
	uint8_t  slot_list[NET_SIZE * 2];
	uint8_t  n_used_l0;
	uint8_t  n_free_hx;
	uint8_t  n_ol0_hx;
	uint8_t  n_free_h1;
	uint8_t  n_ol0_h1;
	uint8_t  n_free_h2;
	uint8_t  n_ol0_h2;
	uint8_t  n_used_l1;
	uint8_t  ctf_live_num;
	uint8_t  tsn_avgload_demand;
	uint8_t  tsn_traffic_demand;
	uint8_t  reserved;
}ob_state_part1;

struct mgmt_send {
	uint8_t node_id;             // 本节点ID
	uint8_t bw;                  // 带宽
	uint16_t txpower;            // 发射功率
	uint32_t freq;               // 频率
	uint32_t neigh_num;          // 邻居数量
	uint32_t seqno;              // 序号
	uint32_t tx;                 // 发送流量/计数
	uint32_t rx;                 // 接收流量/计数
	uint8_t  veth_version[4];    // veth版本
	uint8_t  agent_version[4];   // agent版本
	uint8_t  ctrl_version[4];    // ctrl版本
	struct mgmt_msg msg[NET_SIZE];// 邻居状态数组
	ob_state_part1 mac_information_part1; //mac层监管控信息part1
	DEVICE_SC_STATUS_REPORT amp_infomation;     //功放数据结构

};

typedef struct __attribute__((__packed__)) {
	uint16_t mgmt_head;
	uint16_t mgmt_len;
	uint16_t mgmt_type;
	uint16_t mgmt_keep;
	uint8_t  mgmt_data[];
}Smgmt_header;

typedef struct{
	uint8_t	 NET_work_mode;
	uint8_t	 fh_len;
	uint16_t res;
	uint32_t hop_freq_tb[HOP_FREQ_NUM];
}Smgmt_net_work_mode;

typedef struct{
	uint32_t trig_mode;
	uint32_t catch_addr;
	uint32_t catch_length;
}Smgmt_IQ_Catch;

typedef struct __attribute__((__packed__)){
	uint8_t rf_agc_framelock_en;
	uint8_t phy_cfo_bypass_en;
	uint16_t phy_pre_STS_thresh;
	uint16_t phy_pre_LTS_thresh;
	uint16_t phy_tx_iq0_scale;
	uint16_t phy_tx_iq1_scale;
	uint8_t  phy_msc_length_mode;
 	uint8_t  phy_sfbc_en;
 	uint8_t  phy_cdd_num;
 	uint8_t  reserved;
}Smgmt_phy;

typedef struct __attribute__((__packed__)) {
	uint16_t mgmt_id;
	uint16_t mgmt_route_interval;
	uint16_t mgmt_route_ttl;
	uint16_t mgmt_virt_queue_num;
	uint16_t mgmt_virt_queue_length;
	uint16_t mgmt_virt_qos_stategy;
	uint8_t mgmt_virt_unicast_mcs;
	uint8_t mgmt_virt_multicast_mcs;
	uint8_t  mgmt_mac_bw;
	uint8_t  reserved;
	uint32_t mgmt_mac_freq;
	uint16_t mgmt_mac_txpower;
	uint16_t mgmt_mac_txpower_ch[POWER_CHANNEL_NUM];
	uint8_t  mgmt_mac_power_level;
	uint8_t  mgmt_mac_power_attenuation;
	uint8_t  mgmt_rx_channel_mode;
	uint16_t mgmt_mac_work_mode;
	Smgmt_net_work_mode  mgmt_net_work_mode;
	Smgmt_IQ_Catch  mgmt_mac_iq_catch;
	Smgmt_phy mgmt_phy;
	uint8_t  u8Slotlen;
	uint8_t resv;
}Smgmt_set_param;

#endif