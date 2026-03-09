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

#define NODEPARAMTYPE 0x0001

#define HEAD 0x4C4A

#define RETURN_OK 0
#define RETURN_FAILED -1
#define RETURN_NULL -2
#define RETURN_TIMEOUT -3
#define TCPCONNECTNUM 6
#define TIME_WAIT_FOR_EVER 0
#define NUMBERNULL -1

#define NODE_MAX 64

#define MSG_TYPE 1
#define ETH_ADDR_SIZE 6

#define MCS_NUM  8
#define MAX_QUEUE_NUM 256

#define NO_MCS 0x0f

#define HOP_FREQ_NUM 32

#define POWER_CHANNEL_NUM 4
#define NET_SIZE 56   

#define POWER_TABLE_SIZE 71


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

typedef enum EMGMT_TYPE {
	MGMT_NODEFIND = 0x3c01,         // 节点发现应答
	MGMT_DEVINFO = 0x4d01,          // 设备信息查询
	MGMT_DEVINFO_REPORT = 0x3c02,   // 设备信息上报
	MGMT_SET_PARAM = 0x4d02,        // 参数设置命令
	MGMT_TOPOLOGY_INFO = 0x3c03,    // 拓扑信息上报
	MGMT_SLOT_INFO = 0x3c04,        // 时隙信息上报
	MGMT_SPECTRUM_QUERY = 0x4d05,   // 频谱查询命令
	MGMT_SPECTRUM_REPORT = 0x3c05,  // 频谱信息上报

	MGMT_POWEROFF = 0x4d06,         // 关机命令
	MGMT_RESTART = 0x4d07,          // 重启命令
	MGMT_FACTORY_RESET = 0x4d08,    // 恢复出厂命令
	MGMT_FIRMWARE_UPDATE = 0x4d09,  // 固件升级命令
	MGMT_FILE_UPDATE = 0x4d0a,      // 文件升级命令
	MGMT_MULTIPOINT_SET = 0x4d0b,   // 多点配置命令
	MGMT_TOPOLOGY_REQUEST = 0x4d0e	//add by yang,拓扑请求
} Emgmt_type;

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

typedef enum {LinkSt_h1,LinkSt_h2,LinkSt_idle} nbr_link; // 邻居链路状态：h1/h2/空闲


struct batadv_jgk_node {
	__be32   nodeid;   // 节点ID
	__be32   s_ogm;    // OGM发送计数
	__be32   r_ogm;    // OGM接收计数
	__be32   f_ogm;    // OGM转发计数
	__be32   s_bcast;  // 广播发送计数
	__be32   r_bcast;  // 广播接收计数
	__be32   f_bcast;  // 广播转发计数
	__be32   num;      // 统计计数/保留
};

struct batadv_jgk_route {
	uint8_t   route;     // 路由下一跳ID
	uint8_t   reserved;  // 预留
	uint16_t  tq;        // 主路径链路质量
	uint8_t   neigh1;    // 候选邻居1
	uint8_t   reserved1; // 预留
	uint16_t  tq1;       // 邻居1链路质量
	uint8_t   neigh2;    // 候选邻居2
	uint8_t   reserved2; // 预留
	uint16_t  tq2;       // 邻居2链路质量
	__be32 s_unicast;    // 单播发送计数
	__be32 r_unicast;    // 单播接收计数
};

struct routetable
{
	struct batadv_jgk_route bat_jgk_route[NODE_MAX];
};


typedef struct __attribute__((__packed__)) {
	uint16_t node_num; // 当前网络节点数量
	uint16_t selfid;   // 本机节点ID
	uint32_t selfip;   // 本机IP（网络序）
}Snodefind;

//拓扑信息
typedef struct neighbor_data {
	uint32_t neighbor_ip;    // 邻居IP
	uint32_t neighbor_rssi;  // 邻居信号强度
	uint32_t neighbor_tx;    // 邻居发送流量
}__attribute__((packed)) neighbor_data;

typedef struct topo_data {
	uint32_t selfip;		//设备IP
	double   longitude;		//经度
	double   latitude;		//纬度
	uint32_t noise;			//本地噪声
	uint32_t tx_traffic;	//节点流量发
	uint32_t rx_traffic;	//节点流量收
	uint32_t neighbors_num;	//邻居个数
	struct neighbor_data neighbors_data[];
}__attribute__((packed)) topo_data;


typedef struct ST_SURVEYINFO
{
	int id;        // 频谱记录ID
	int fre[301];  // 频谱点数据
}stSurveyInfo;
typedef struct ST_NODE{
	int id;          // 节点ID
	char time[24];   // 时间戳
	char ip[16];     // 节点IP
	int txpower;     // 发射功率
	int bw;          // 带宽
	int freq;        // 频率
	int mcs;         // MCS
	int mode;        // 工作模式
	int interval;    // 路由周期
	int max;         // 最大跳数
	int nbor;        // 邻居数量
	char    lotd[20];			//节点经度
	char    latd[20];			//节点纬度
	char softver[12]; // 软件版本
	char harver[12];  // 硬件版本
}stNode;

typedef struct udp_header {
	short sport; // Source port
	short dport; // Destination port
	short len; // Datagram length
	short crc; // Checksum
}__attribute__((packed)) udp_header;


typedef struct {
	uint8_t                       dest_mac_addr[ETH_ADDR_SIZE];                      // Destination MAC address
	uint8_t                       src_mac_addr[ETH_ADDR_SIZE];                       // Source MAC address
	uint16_t                      ethertype;                                        // EtherType
}__attribute__((packed)) ethernet_header_t;

typedef struct ip_header {
	char ver_ihl; // Version (4 bits) + Internet header length (4 bits)
	char tos; // Type of service
	short tlen; // Total length
	short identification; // Identification
	short flags_fo; // Flags (3 bits) + Fragment offset (13 bits)
	char ttl; // Time to live
	char proto; // Protocol
	short crc; // Header checksum
	unsigned int saddr; // Source address
	unsigned int daddr; // Destination address
}__attribute__((packed)) ip_header;

typedef struct mgmt_status_data {
	uint32_t selfip;             // 本机IP
	uint16_t selfid;             // 本机ID
	uint16_t tv_route;           // 路由更新周期
	uint16_t maxHop;             // 最大跳数
	uint16_t num_queues;         // 队列数
	uint16_t depth_queues;       // 队列深度
	uint16_t qos_policy;         // QoS策略
	uint8_t mcs_unicast;         // 单播MCS
	uint8_t mcs_broadcast;       // 广播MCS
	uint8_t bw;                  // 带宽
	uint8_t reserved;            // 预留
	uint32_t freq;               // 频率
	uint16_t txpower;            // 功率
	uint16_t work_mode;          // 工作模式
	double longitude;            // 经度
	double latitude;             // 纬度
	char software_version[16];   // 软件版本
	char hardware_version[16];   // 硬件版本
}__attribute__((packed)) mgmt_status_data;
typedef struct mgmt_status_header {
	uint16_t flag;                     // 协议标识
	uint16_t len;                      // 数据长度
	uint16_t type;                     // 消息类型
	uint16_t reserved;                 // 预留
	struct mgmt_status_data status_data;// 状态数据体
}__attribute__((packed)) mgmt_status_header;

typedef enum EMGMT_FIRMWARE_TYPE {
	MGMT_NAME = 0x0901,            // 固件名分片
	MGMT_CONTENT = 0x0902,         // 固件内容分片
	MGMT_END = 0x0903,             // 固件传输结束
	MGMT_UPDATE_FIRMWARE = 0x0904  // 执行固件更新
} Emgmt_firmware_type;

typedef enum EMGMT_FILE_TYPE {
	MGMT_FILENAME = 0x0a01,      // 文件名分片
	MGMT_FILECONTENT = 0x0a02,   // 文件内容分片
	MGMT_FILEEND = 0x0a03,       // 文件传输结束
	MGMT_UPDATE_FILE = 0x0a04    // 执行文件更新
} Emgmt_file_type;

typedef struct {
	uint32_t  srcIp; // 请求源IP
	//uint16_t msg_type;//拓扑请求类型
}__attribute__((packed)) topology_request;

typedef struct ST_INDATA{
    char name[20];        // 字段名/键名 (Key)。比如 "ipaddr", "device", "rf_freq", "snr1"
    char value[24];       // 字段值 (Value)。为了通用，所有类型的数据（包括整型、浮点型）在这里都被 sprintf 转成了字符串保存
    char state[4];        // 状态标志位 (State)。通常填 "1" 或 "0"，用于标记该数据是否刚被更新（脏位），或者是否有效
    char lib[4];          // 库/来源标志 (Lib)。在特定的数据库操作中，用来标识该数据的来源或者权限类型（比如 '0' 代表内部默认，'1' 代表外部修改等）
} stInData;


typedef struct BCAST_MESHINFO{
	uint16_t m_txpower;                         // 单值发射功率
	uint16_t m_txpower_ch[POWER_CHANNEL_NUM];   // 分通道功率
	uint32_t rf_freq;                           // 频率
	uint8_t m_chanbw;                           // 带宽
	uint8_t m_rate;                             // 速率/MCS
	uint8_t m_bcastmode;                        // 广播模式 1:非组播广播 0:组播
	uint8_t workmode;                           // 工作模式
	uint8_t m_route;                            // 路由模式
	uint8_t m_slot_len;                         // 时隙长度
	uint16_t m_trans_mode;                      // 传输模式
	uint32_t m_select_freq_1;	                 // 自适应频点1
	uint32_t m_select_freq_2;	                 // 自适应频点2
	uint32_t m_select_freq_3;	                 // 自适应频点3
	uint32_t m_select_freq_4;	                 // 自适应频点4
	uint8_t power_level;                        // 功率等级
	uint8_t power_attenuation;                  // 功率衰减
	uint8_t rx_channel_mode;                    // 接收通道模式
		
	// int m_distance;
	// int m_ssid;
	uint8_t txpower_isset;            // 功率是否有效
	uint8_t freq_isset;               // 频率是否有效
	uint8_t chanbw_isset;             // 带宽是否有效
	uint8_t rate_isset;               // 速率是否有效
	uint8_t workmode_isset;           // 工作模式是否有效
	uint8_t route_isset;              // 路由设置是否有效
	uint8_t slot_isset;               // 时隙设置是否有效
	uint8_t trans_mode_isset;	       // 传输模式是否有效
	uint8_t select_freq_isset;        // 自适应频点是否有效
	uint8_t power_level_isset;        // 功率等级是否有效
	uint8_t power_attenuation_isset;  // 功率衰减是否有效
	uint8_t rx_channel_mode_isset;    // 接收通道模式是否有效
	//bool bcastmode_isset;

// 用作更新systeminfo库
	int sys_power;             // systeminfo库：功率
	int sys_power_level;       // systeminfo库：功率等级
	int sys_power_attenuation; // systeminfo库：功率衰减
	int sys_freq;              // systeminfo库：频率
	int sys_rate;              // systeminfo库：速率
	int sys_bw;                // systeminfo库：带宽
	int sys_workmode;          // systeminfo库：工作模式
	int sys_rx_channel_mode;   // systeminfo库：接收通道模式

}bcMeshInfo;


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

typedef struct {
	//	uint8_t  packet_class[3]; //0x01_0xaa_0xbb
	//	uint8_t  node_id;
	uint16_t time_jitter[NET_SIZE]; // 抖动
	uint8_t  snr[NET_SIZE];         // 信噪比
	uint8_t  rssi[NET_SIZE];        // 信号强度
	uint8_t  mcs[NET_SIZE];         // 调制编码方式
	short    good[NET_SIZE];        // 好包计数
	short    bad[NET_SIZE];         // 坏包计数
	uint8_t  ucds[NET_SIZE];        // UCDS值
	uint8_t  noise[NET_SIZE];       // 噪声
}ob_state_part2;

typedef struct {
	uint16_t tx_in[MAX_QUEUE_NUM];
	uint16_t tx_qlen[MAX_QUEUE_NUM];
	uint32_t tx_inall;
	uint32_t tx_outall;
	uint32_t tx_in_lose;
	uint32_t tx_out_lose;
	uint32_t rx_inall;
	uint32_t rx_outall;
	uint32_t rx_in_lose;
	uint32_t rx_out_lose;
	uint32_t mac_list_tx_cnt;
	uint32_t tx_in_cnt;
	uint32_t phy_tx_done_cnt;
	uint32_t phy_rx_done_cnt;
	uint32_t ogm_in;
	uint32_t ogm_in_len;
	uint32_t ogm_slot;
	uint32_t ogm_out_len;
	uint32_t ping_in;
	uint32_t ping_in_len;
	uint32_t ping_slot;
	uint32_t ping_out_len;
	uint32_t bcast_in;
	uint32_t bcast_in_len;
	uint32_t bcast_slot;
	uint32_t bcast_out_len;
	uint32_t ucast_in;
	uint32_t ucast_in_len;
	uint32_t ucast_slot;
	uint32_t ucast_out_len;
}virt_eth_jgk_info;

typedef struct {
	uint8_t  veth_version[4];
	uint8_t  agent_version[4];
	uint8_t  ctrl_version[4];
	uint32_t  enqueue_bytes[MCS_NUM]; //每个mcs队列对应的入队列比特数
	uint32_t  outqueue_bytes[MCS_NUM]; //每个mcs队列对应的处队列比特数
	ob_state_part1 mac_information_part1; //mac层监管控信息part1
	ob_state_part2 mac_information_part2; //mac层监管控信息part2
	virt_eth_jgk_info traffic_queue_information; //业务模块监管控信息
#ifdef	Radio_SWARM_S2
	DEVICE_SC_STATUS_REPORT amp_infomation;     //add by sdg 功放数据结构
#endif
}jgk_report_infor;

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

#ifdef Radio_SWARM_S2
typedef struct __attribute__((__packed__)){
	uint8_t rf_agc_framelock_en; // 帧锁AGC使能
	uint8_t phy_cfo_bypass_en;   // CFO旁路使能
	uint16_t phy_pre_STS_thresh; // STS阈值
	uint16_t phy_pre_LTS_thresh; // LTS阈值
	uint16_t phy_tx_iq0_scale;   // IQ0发射缩放
	uint16_t phy_tx_iq1_scale;   // IQ1发射缩放
	uint8_t  phy_msc_length_mode;// MSC长度模式
 	uint8_t  phy_sfbc_en;        // SFBC使能
 	uint8_t  phy_cdd_num;        // CDD数量
 	uint8_t  reserved;           // 预留
}Smgmt_phy;

#else
typedef struct __attribute__((__packed__)){
	uint8_t rf_agc_framelock_en; // 帧锁AGC使能
	uint8_t phy_cfo_bypass_en;   // CFO旁路使能
	uint16_t phy_pre_STS_thresh; // STS阈值
	uint16_t phy_pre_LTS_thresh; // LTS阈值
	uint16_t phy_tx_iq0_scale;   // IQ0发射缩放
	uint16_t phy_tx_iq1_scale;   // IQ1发射缩放
}Smgmt_phy;
#endif

typedef struct __attribute__((__packed__)) {
	uint16_t mgmt_id;                                  // 节点ID
	uint16_t mgmt_route_interval;                      // 路由广播周期
	uint16_t mgmt_route_ttl;                           // 路由TTL
	uint16_t mgmt_virt_queue_num;                      // 虚拟网卡队列数量
	uint16_t mgmt_virt_queue_length;                   // 虚拟网卡队列长度
	uint16_t mgmt_virt_qos_stategy;                    // QoS策略
	uint8_t mgmt_virt_unicast_mcs;                     // 单播MCS
	uint8_t mgmt_virt_multicast_mcs;                   // 组播MCS
	uint8_t  mgmt_mac_bw;                              // 带宽
	uint8_t  reserved;                                 // 预留
	uint32_t mgmt_mac_freq;                            // 频率
	uint16_t mgmt_mac_txpower;                         // 单值发射功率
	uint16_t mgmt_mac_txpower_ch[POWER_CHANNEL_NUM];   // 分通道发射功率
	uint8_t  mgmt_mac_power_level;                     // 功率等级
	uint8_t  mgmt_mac_power_attenuation;               // 功率衰减
	uint8_t  mgmt_rx_channel_mode;                     // 接收通道模式
	uint16_t mgmt_mac_work_mode;                       // 测试工作模式
	Smgmt_net_work_mode  mgmt_net_work_mode;           // 网络模式/跳频参数
	Smgmt_IQ_Catch  mgmt_mac_iq_catch;                 // IQ抓取参数
#ifdef Radio_CEC

	uint8_t  mgmt_NCU_node_id;                         // CEC模式NCU节点ID
	uint16_t reserved2;                                // 预留
#endif
	Smgmt_phy mgmt_phy;                                // PHY参数
	uint8_t  u8Slotlen;                                // 时隙长度
	uint8_t resv;                                      // 预留
}Smgmt_set_param;
typedef struct __attribute__((__packed__)) {
	uint32_t mgmt_ip;                                // 节点IP
	uint16_t mgmt_id;                                // 节点ID
	uint16_t mgmt_route_interval;                    // 路由广播周期
	uint16_t mgmt_route_ttl;                         // 路由TTL
	uint16_t mgmt_virt_queue_num;                    // 队列数量
	uint16_t mgmt_virt_queue_length;                 // 队列深度
	uint16_t mgmt_virt_qos_stategy;                  // QoS策略
	uint8_t mgmt_virt_unicast_mcs;                   // 单播MCS
	uint8_t mgmt_virt_multicast_mcs;                 // 组播MCS
	uint8_t  mgmt_mac_bw;                            // 带宽
	uint8_t  reserved;                               // 预留
	uint32_t mgmt_mac_freq;                          // 频率
	uint16_t mgmt_mac_txpower;                       // 发射功率
	uint16_t mgmt_mac_txpower_ch[POWER_CHANNEL_NUM]; // 分通道功率
	uint8_t  mgmt_rx_channel_mode;                   // 接收通道模式
	uint16_t mgmt_mac_work_mode;                     // MAC工作模式
	double   mgmt_longitude;                         // 经度
	double   mgmt_latitude;                          // 纬度
}Smgmt_param;

typedef struct ST_NBIFNO{
	int nbid1;		//邻居ID1
	int snr1;	   //邻居1信噪比
	int getlv1;    //邻居1信号强度
	int flowrate1; //邻居1流量
}stNbInfo;

typedef struct ST_LINK{
	int id;                 // 节点ID
	char time[24];          // 时间戳
	stNbInfo m_stNbInfo[32];// 邻居链路信息
}stLink;

#endif