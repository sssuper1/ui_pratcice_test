#include "mgmt_transmit.h"
#include "mgmt_types.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "wg_config.h"
#include "socketUDP.h"
#include "Lock.h"
#include "SocketTCP.h"
#include "mgmt_netlink.h"
#include <sys/types.h>
#include <net/if.h>
#include <sys/socket.h>
#include "ui_get.h"
#include "Lock.h"
#include <asm/socket.h>
#include "ui_get.h"
#include <sys/select.h>
#include "sqlite_unit.h"


#define BUFLEN 1500
#define GROUND_PORT 16001
#define CTRL_PORT   16000
#define MGMT_PORT   16001
#define WG_RX_UDP_PORT  7600
#define WG_TX_UDP_PORT  7700
#define WG_RX_TCP_PORT  7601
#define WG_TX_TCP_PORT  7701
#define BCAST_RECV_PORT   7901
#define CONNECTNUM 10


static pthread_cond_t TCPCLIENTCOND_WG;
static pthread_mutex_t TCPCLIENTMUTEX_WG;
static TcpClient TCPCLIENT_WG[CONNECTNUM];
static int SOCKET_MGMT;
static int SOCKET_GROUND;
static struct sockaddr_in S_GROUND_STD;
static struct sockaddr_in S_GROUND_PC;
static struct sockaddr_in S_GROUND;
static struct sockaddr_in S_GROUND_WG;
static struct sockaddr_in S_OTHER_NODE;
static struct sockaddr_in S_GROUND_BCAST; 
static int SOCKET_BCAST_RECV;
static int SOCKET_MEM_REQUEST_RECV; 
static int SOCKET_UDP_WG;
static int SOCKET_TCP_WG;

static int MSG_MGMT;
static BOOL ISLOGIN;


int is_conned = 0;//网关节点判断是否和网管连接
int gotRequest = 0;//邻居节点判断是否收到拓扑请求，收到后持续向网关发送拓扑信息
struct sockaddr_in wg_addr;//用来存放网管的地址信息
struct sockaddr_in gate_addr;//用于邻居节点存放网关的地址信息
double longitude;
double latitude;
char version[20];

uint8_t SELFID;
uint32_t SELFIP;
uint8_t SELFIP_s[4];

uint8_t MCS_INIT;
uint8_t NET_WORKMOD_INIT;
uint32_t FREQ_INIT;
uint16_t POWER_INIT;
uint8_t BW_INIT;
uint8_t RX_CHANNEL_MODE_INIT;
uint16_t MACMODE_INIT;
uint8_t DEVICETYPE_INIT;
uint8_t POWER_LEVEL_INIT;
uint8_t POWER_ATTENUATION_INIT;
uint32_t HOP_FREQ_TB_INIT[HOP_FREQ_NUM];
uint8_t g_u8SLOTLEN;
Global_Radio_Param g_radio_param;

Smgmt_phy PHY_MSG;

#define TXPOWER_TABLE_FILE "/etc/9361_table"

static bool g_txpower_table_initialized;//
static uint16_t g_txpower_table[POWER_TABLE_SIZE][POWER_CHANNEL_NUM];


char *id[]={"id1","id2","id3","id4","id5","id6","id7","id8","id9","id10","id11","id12","id13","id14","id15","id16",
"id17","id18","id19","id20","id21","id22","id23","id24","id25","id26","id27","id28","id29","id30","id31","id32"};

char *id2hop[]={"2id1","2id2","2id3","2id4","2id5","2id6","2id7","2id8","2id9","2id10","2id11","2id12","2id13","2id14","2id15","2id16",
"2id17","2id18","2id19","2id20","2id21","2id22","2id23","2id24","2id25","2id26","2id27","2id28","2id29","2id30","2id31","2id32"};

char *ip[]={"ip1","ip2","ip3","ip4","ip5","ip6","ip7","ip8","ip9","ip10","ip11","ip12","ip13","ip14","ip15","ip16",
"ip17","ip18","ip19","ip20","ip21","ip22","ip23","ip24","ip25","ip26","ip27","ip28","ip29","ip30","ip31","ip32"};

char *ip2hop[]={"2ip1","2ip2","2ip3","2ip4","2ip5","2ip6","2ip7","2ip8","2ip9","2ip10","2ip11","2ip12","2ip13","2ip14","2ip15","2ip16",
"2ip17","2ip18","2ip19","2ip20","2ip21","2ip22","2ip23","2ip24","2ip25","2ip26","2ip27","2ip28","2ip29","2ip30","2ip31","2ip32"};

char *rssi[]={"rssi1","rssi2","rssi3","rssi4","rssi5","rssi6","rssi7","rssi8","rssi9","rssi10","rssi11","rssi12","rssi13","rssi14","rssi15","rssi16",
"rssi17","rssi18","rssi19","rssi20","rssi21","rssi22","rssi23","rssi24","rssi25","rssi26","rssi27","rssi28","rssi29","rssi30","rssi31","rssi32"};

char *snr[]={"snr1","snr2","snr3","snr4","snr5","snr6","snr7","snr8","snr9","snr10","snr11","snr12","snr13","snr14","snr15","snr16",
"snr17","snr18","snr19","snr20","snr21","snr22","snr23","snr24","snr25","snr26","snr27","snr28","snr29","snr30","snr31","snr32"};

char *timejitter[]={"timejitter1","timejitter2","timejitter3","timejitter4","timejitter5","timejitter6","timejitter7","timejitter8",
"timejitter9","timejitter10","timejitter11","timejitter12","timejitter13","timejitter14","timejitter15","timejitter16",
"timejitter17","timejitter18","timejitter19","timejitter20","timejitter21","timejitter22","timejitter23","timejitter24",
"timejitter25","timejitter26","timejitter27","timejitter28","timejitter29","timejitter30","timejitter31","timejitter32"};

char *linkquality[]={"linkquality1","linkquality2","linkquality3","linkquality4","linkquality5","linkquality6","linkquality7","linkquality8",
"linkquality9","linkquality10","linkquality11","linkquality12","linkquality13","linkquality14","linkquality15","linkquality16",
"linkquality17","linkquality18","linkquality19","linkquality20","linkquality21","linkquality22","linkquality23","linkquality24",
"linkquality25","linkquality26","linkquality27","linkquality28","linkquality29","linkquality30","linkquality31","linkquality32"};

char *bad[]={"bad1","bad2","bad3","bad4","bad5","bad6","bad7","bad8","bad9","bad10","bad11","bad12","bad13","bad14","bad15","bad16",
"bad17","bad18","bad19","bad20","bad21","bad22","bad23","bad24","bad25","bad26","bad27","bad28","bad29","bad30","bad31","bad32"};

char *good[]={"good1","good2","good3","good4","good5","good6","good7","good8","good9","good10","good11","good12","good13","good14",
	"good15","good16","good17","good18","good19","good20","good21","good22","good23","good24","good25","good26","good27","good28",
	"good29","good30","good31","good32"};

char *ucds[]={"ucds1","ucds2","ucds3","ucds4","ucds5","ucds6","ucds7","ucds8","ucds9","ucds10","ucds11","ucds12","ucds13","ucds14",
	"ucds15","ucds16","ucds17","ucds18","ucds19","ucds20","ucds21","ucds22","ucds23","ucds24","ucds25","ucds26","ucds27","ucds28",
	"ucds29","ucds30","ucds31","ucds32"};


char *signalevel[]={"signallevel","signallevel2","signallevel3","signallevel4","signallevel5","signallevel6","signallevel7","signallevel8","signallevel9",
"signallevel10","signallevel11","signallevel12","signallevel13","signallevel14","signallevel15","signallevel16","signallevel17","signallevel18","signallevel19",
"signallevel20","signallevel21","signallevel22","signallevel23","signallevel24","signallevel25","signallevel26","signallevel27","signallevel28","signallevel29",
"signallevel30","signallevel31","signallevel32"};

char *noise[]={"noise1","noise2","noise3","noise4","noise5","noise6","noise7","noise8","noise9",
"noise10","noise11","noise12","noise13","noise14","noise15","noise16","noise17","noise18","noise19",
"noise20","noise21","noise22","noise23","noise24","noise25","noise26","noise27","noise28","noise29",
"noise30","noise31","noise32"};

double htond(double val) {
	double dval = val;
	uint64_t tmp = 0;
	memcpy(&tmp, &dval, sizeof(dval));
	tmp = htobe64(tmp);
	memcpy(&dval, &tmp, sizeof(tmp));
	return dval;
}

uint16_t ipCksum(void* ip, int len) {
	uint16_t* buf = (uint16_t*)ip;
	uint32_t cksum = 0;

	while (len > 1)
	{
		cksum += *buf++;
		len -= sizeof(uint16_t);
	}

	if (len)
		cksum += *(uint16_t*)buf;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (uint16_t)(~cksum);
}

//将信息打包 下发内核
void mgmt_param_init(){
    INT8 buffer[sizeof(Smgmt_header) + sizeof(Smgmt_set_param)];
	INT32 buflen = sizeof(Smgmt_header) + sizeof(Smgmt_set_param);
	Smgmt_header* mhead = (Smgmt_header*)buffer;
	Smgmt_set_param* mparam = (Smgmt_set_param*)mhead->mgmt_data;
	bzero(buffer, buflen);
	mhead->mgmt_head = htons(HEAD);
	mhead->mgmt_len = sizeof(Smgmt_set_param);
	mhead->mgmt_type = 0;

    mhead->mgmt_type |= MGMT_SET_POWER;
	mhead->mgmt_type |= MGMT_SET_BANDWIDTH;
	mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
	mhead->mgmt_type |= MGMT_SET_TEST_MODE;
	mhead->mgmt_type |= MGMT_SET_PHY;
	mhead->mgmt_keep |= MGMT_SET_SLOTLEN;
	mhead->mgmt_keep |= MGMT_SET_POWER_LEVEL;
	mhead->mgmt_keep |= MGMT_SET_POWER_ATTENUATION;
	mhead->mgmt_keep |= MGMT_SET_RX_CHANNEL_MODE;
    if(NET_WORKMOD_INIT == FIX_FREQ_MODE)
	{
		mhead->mgmt_type |= MGMT_SET_FREQUENCY;
	}
    mhead->mgmt_type |= MGMT_SET_WORKMODE;

    mhead->mgmt_type = htons(mhead->mgmt_type);
	mhead->mgmt_keep = htons(mhead->mgmt_keep);

    mparam->mgmt_mac_freq = htonl(FREQ_INIT);
    mparam->mgmt_mac_txpower = htons(POWER_INIT);
    txpower_lookup_channels_be(POWER_INIT, mparam->mgmt_mac_txpower_ch);
    mparam->mgmt_mac_bw = BW_INIT;
    mparam->mgmt_virt_unicast_mcs = MCS_INIT;
    mparam->mgmt_mac_work_mode = htons(MACMODE_INIT);
    mparam->mgmt_phy = PHY_MSG;
    mparam->mgmt_phy.phy_pre_STS_thresh = htons(mparam->mgmt_phy.phy_pre_STS_thresh);
	mparam->mgmt_phy.phy_pre_LTS_thresh = htons(mparam->mgmt_phy.phy_pre_LTS_thresh);
	mparam->mgmt_phy.phy_tx_iq0_scale = htons(mparam->mgmt_phy.phy_tx_iq0_scale);
	mparam->mgmt_phy.phy_tx_iq1_scale = htons(mparam->mgmt_phy.phy_tx_iq1_scale);
    mparam->u8Slotlen = g_u8SLOTLEN;
    mparam->mgmt_mac_power_level = POWER_LEVEL_INIT;
	mparam->mgmt_mac_power_attenuation = POWER_ATTENUATION_INIT;
    mparam->mgmt_rx_channel_mode = RX_CHANNEL_MODE_INIT;

    mgmt_netlink_set_param(buffer, buflen, NULL);

    //初始化全局参数结构体
    memset(&g_radio_param,0,sizeof(Global_Radio_Param));
	g_radio_param.g_chanbw=BW_INIT;
	g_radio_param.g_rate=MCS_INIT;
	g_radio_param.g_rf_freq=FREQ_INIT;
	g_radio_param.g_route=3;
	g_radio_param.g_slot_len=g_u8SLOTLEN;
	g_radio_param.g_txpower=POWER_INIT;
	g_radio_param.g_workmode=NET_WORKMOD_INIT;

}

//读出id号 读出xwg的参数，初始化参数 开启服务器
void mgmt_mysql_init(void){

    FILE* file;
    FILE* file_node;
    FILE* file_hop;

    char nodemessage[100];
	char messagename[100];
    char data[10];
    int id;
    int param[9];

    int i;
    int on = 1;
    int row_cnt;
    char data_hop[100];
    uint8_t cmd[200];
    //char ifname[] = "br0";
	char ifname[] = "eth0";//ssq
    bzero(param, sizeof(param));
    POWER_LEVEL_INIT = 0;//默认功率等级为0
	POWER_ATTENUATION_INIT = 0;//默认功率衰减为0
	RX_CHANNEL_MODE_INIT = 0;//默认接收通道模式为0

    file = popen("cat /etc/node_id", "r");
    fread(data,sizeof(char),sizeof(data),file);
    sscanf(data,"%d",&id);//从文件里读出节点ID
    pclose(file);
    
    if((file_node = fopen("/etc/node_xwg","r")) == NULL){
        FREQ_INIT = 1478;
    }
    else{
        while(fgets(nodemessage,sizeof(nodemessage),file_node)!=NULL){
            memset(messagename,0,sizeof(messagename));
            sscanf(nodemessage,"%s",messagename);
            if(strcmp(messagename,"channel")==0){
                sscanf(nodemessage,"channel %d",&FREQ_INIT);
                printf("set ------- channel = %d\n", FREQ_INIT);
            }
            else if(strcmp(messagename,"networkmode")==0){
                sscanf(nodemessage,"networkmode %d",&param[0]);
                NET_WORKMOD_INIT = param[0];
                printf("set ------- networkmode = %d\n", NET_WORKMOD_INIT);
            }
            else if(strcmp(messagename,"devicetype")==0){
                sscanf(nodemessage, "devicetype %d", &param[0]);
				DEVICETYPE_INIT = param[0];
				printf("set ------- devicetype = %d\n", DEVICETYPE_INIT);
            }
            else if(strcmp(messagename,"power")==0){
                sscanf(nodemessage, "power %d", &param[0]);
				POWER_INIT = param[0];
				printf("set ------- power = %d\n", POWER_INIT);
            }
            else if(strcmp(messagename,"power_level")==0){
                sscanf(nodemessage, "power_level %d", &param[0]);
                POWER_LEVEL_INIT = param[0];
                printf("set ------- power_level = %d\n", POWER_LEVEL_INIT);
            }
            else if(strcmp(messagename,"power_attenuation")==0){
                sscanf(nodemessage, "power_attenuation %d", &param[0]);
                POWER_ATTENUATION_INIT = param[0];
                printf("set ------- power_attenuation = %d\n", POWER_ATTENUATION_INIT);
            }
            else if(strcmp(messagename,"rx_channel_mode")==0){
                sscanf(nodemessage, "rx_channel_mode %d", &param[0]);
                if (param[0] < 0)
					param[0] = 0;
				else if (param[0] > 4)
					param[0] = 4;
                RX_CHANNEL_MODE_INIT = param[0];
                printf("set ------- rx_channel_mode = %d\n", RX_CHANNEL_MODE_INIT);
            }
            else if(strcmp(messagename,"bw") == 0){
                sscanf(nodemessage,"bw %d",&param[0]);
                BW_INIT = param[0];
                printf("set ------- bw = %d\n", BW_INIT);
            }
            else if(strcmp(messagename,"version") == 0){
                sscanf(nodemessage,"version %s",&version);
                printf("set ------- version = %s\n", version);
            }
            else if(strcmp(messagename,"mcs") == 0){
                sscanf(nodemessage,"mcs %d",&param[0]);
                MCS_INIT = param[0];
                printf("set ------- mcs = %d\n", MCS_INIT);
            }
            else if(strcmp(messagename,"macmode") == 0){
                sscanf(nodemessage,"macmode %d",&param[0]);
                MACMODE_INIT = param[0];
                printf("set ------- macmode = %d\n", MACMODE_INIT);
            }
            else if(strcmp(messagename,"phymsg") == 0){
                sscanf(nodemessage, "phymsg %d %d %d %d %d %d %d %d %d", &param[0],
					&param[1], &param[2],
					&param[3], &param[4],
					&param[5],&param[6],
					&param[7],&param[8]);
				PHY_MSG.rf_agc_framelock_en = param[0];
				PHY_MSG.phy_cfo_bypass_en = param[1];
				PHY_MSG.phy_pre_STS_thresh = param[2];
				PHY_MSG.phy_pre_LTS_thresh = param[3];
				PHY_MSG.phy_tx_iq0_scale = param[4];
				PHY_MSG.phy_tx_iq1_scale = param[5];
				PHY_MSG.phy_msc_length_mode = param[6];
				PHY_MSG.phy_sfbc_en         = param[7];
				PHY_MSG.phy_cdd_num         = param[8];

                printf("set ------- phymsg = %d %d %d %d %d %d %d %d %d\n", PHY_MSG.rf_agc_framelock_en, PHY_MSG.phy_cfo_bypass_en,
					PHY_MSG.phy_pre_STS_thresh, PHY_MSG.phy_pre_LTS_thresh,
					PHY_MSG.phy_tx_iq0_scale, PHY_MSG.phy_tx_iq1_scale,
					PHY_MSG.phy_msc_length_mode,PHY_MSG.phy_sfbc_en,
					PHY_MSG.phy_cdd_num);
            }
            else if (strcmp(messagename, "ip_addr") == 0) {
				sscanf(nodemessage, "ip_addr %d.%d.%d.%d", &SELFIP_s[0],&SELFIP_s[1],&SELFIP_s[2],&SELFIP_s[3]);
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,
						"ifconfig eth0 %d.%d.%d.%d",
						SELFIP_s[0],SELFIP_s[1],SELFIP_s[2],SELFIP_s[3]);
				system(cmd);
				
				printf("set ------- eth0 ip address = %d.%d.%d.%d\n", SELFIP_s[0],SELFIP_s[1],SELFIP_s[2],SELFIP_s[3]);				
			}
            else if (strcmp(messagename, "slotlen") == 0) {
				sscanf(nodemessage, "slotlen %d", &g_u8SLOTLEN);
				printf("set ------- slotlen = %d\n", g_u8SLOTLEN);	
			}
        }
        fclose(file);
    }

    row_cnt = 0;
    if((file_hop =fopen("/etc/node_hop","r")) !=NULL){
        while(fgets(data_hop,sizeof(data_hop),file_hop)!=NULL){
            sscanf(data_hop, "%d %d %d %d %d %d %d %d", &param[0],
					&param[1], &param[2],
					&param[3], &param[4],
					&param[5], &param[6],
					&param[7]);
            for(i=0;i<8;i++)
			{
				HOP_FREQ_TB_INIT[row_cnt*8+i] = param[i];
			}
			row_cnt++;		
			bzero(data_hop, sizeof(data_hop));
        }
        fclose(file_hop);
    }
    printf("set ------- hop freq table= ");
	for(i=0;i<HOP_FREQ_NUM;i++)
	{
		printf(" %d", HOP_FREQ_TB_INIT[i]);
	}
	printf("\n");
	SELFID = id;
	g_radio_param.device_id=SELFID;
    SOCKET_MGMT = CreateUDPServer(MGMT_PORT);
	if (SOCKET_MGMT <= 0)
	{
		printf("SOCKET_MGMT create error\n");
		exit(1);
	}
    S_GROUND_STD.sin_family = AF_INET;
    S_GROUND_STD.sin_addr.s_addr = inet_addr("192.168.2.1");
    S_GROUND_STD.sin_port = htons(MGMT_PORT);

    S_GROUND_WG.sin_family = AF_INET;
	S_GROUND_WG.sin_addr.s_addr = inet_addr("255.255.255.255");
	S_GROUND_WG.sin_port = htons(WG_TX_UDP_PORT);

    S_OTHER_NODE.sin_family = AF_INET;
	S_OTHER_NODE.sin_addr.s_addr = inet_addr("192.168.255.255");
	S_OTHER_NODE.sin_port = htons(WG_RX_UDP_PORT);

    SOCKET_GROUND = CreateUDPServer(CTRL_PORT);
	if (SOCKET_GROUND <= 0) {
		printf("SOCKET_GROUND create error\n");
		exit(1);
	}
    SOCKET_BCAST_RECV=CreateUDPServer(BCAST_RECV_PORT);
	if (SOCKET_BCAST_RECV <= 0)
	{
		printf("SOCKET_BCAST_RECV create error\n");
		exit(1);
	}
    if (setsockopt(SOCKET_BCAST_RECV, SOL_SOCKET, SO_BINDTODEVICE, ifname, 4) < 0) {
		printf("SOCKET_BCAST_RECV bindtodevice error\n");
		exit(1);
	}
    if (setsockopt(SOCKET_BCAST_RECV, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
        printf("SOCKET_BCAST_RECV broadcast enable error\n");
        exit(1);
    }

    SOCKET_MEM_REQUEST_RECV=CreateUDPServer(MEM_REQUEST_PORT);
	if (SOCKET_MEM_REQUEST_RECV <= 0)
	{
		printf("SOCKET_MEM_REQUEST_RECV create error\n");
		exit(1);
	}
    SOCKET_UDP_WG = CreateUDPServer(WG_RX_UDP_PORT);
	//测试打印：
	printf("创建SOCKET_UDP_WG的socket,端口为WG_RX_UDP_PORT=%d\n", WG_RX_UDP_PORT);

	if (setsockopt(SOCKET_UDP_WG, SOL_SOCKET, SO_BINDTODEVICE, ifname, 4) < 0) {
		printf("SOCKET_UDP_WG bindtodevice error\n");
		exit(1);
	}
	if (setsockopt(SOCKET_UDP_WG, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
		printf("SOCKET_UDP_WG BROADCAST error\n");
		exit(1);
	}	
    //SOCKET_TCP_WG = CreateTCPClient(WG_RX_TCP_PORT);
    SOCKET_TCP_WG = CreateTCPServer(WG_RX_TCP_PORT); //ssq
	printf("SOCKET_TCP_WG = %d\n", SOCKET_TCP_WG);
	for (int i = 0; i < CONNECTNUM; i++) {
		TCPCLIENT_WG[i].useful = FALSE;
		TCPCLIENT_WG[i].sockfd = 0;
		TCPCLIENT_WG[i].srcip = 0;
		TCPCLIENT_WG[i].time.tv_sec = 0;
		TCPCLIENT_WG[i].time.tv_usec = 0;
	}
	TCPCLIENTCOND_WG = CreateEvent();//创建条件变量
	TCPCLIENTMUTEX_WG = CreateLock();//创建互斥锁


	S_GROUND_PC.sin_family = AF_INET;
	S_GROUND_PC.sin_addr.s_addr = inet_addr("192.168.2.100");
	S_GROUND_PC.sin_port = htons(GROUND_PORT);
	S_GROUND = S_GROUND_PC;
	ISLOGIN = FALSE;
    mgmt_param_init();
	printf("init 1\n", SOCKET_TCP_WG);
}


void mgmt_get_msg(void){
    // ===== 管理消息结构体 =====
	struct mgmt_send self_msg;        // 自身节点消息结构体
	struct routetable route_msg;      // 路由表信息
	
	// ===== 拓扑相关结构体（网关和邻居间的拓扑交互）=====
	Smgmt_header topo_header;         // 拓扑消息头
	Smgmt_header* topo_header_ptr;    // 拓扑消息头指针
	struct topo_data topomsg;         // 拓扑数据
	struct topo_data* topomsgptr;     // 拓扑数据指针
	char topobuff[2048];              // 拓扑数据缓冲区
	
	// ===== 邻居信息统计数组 =====
	int neighid_info[32];             // 存放邻居ID信息
	uint8_t mcs_all[NET_SIZE];        // 存放邻居MCS编码方式信息
	
	// ===== 报文缓冲区和报文长度 =====
	char buf[2048];                   // 完整报文缓冲区（包含所有协议层）
	int len = sizeof(buf);            // 报文长度
	int ret = 0, i = 0, j = 0, k=0;   // 循环计数器
	int id_index=0;                   // 节点索引
	uint32_t seqno = 0;               // 报文序号（递增）
	uint8_t dstmac[ETH_ADDR_SIZE] = { 0xff,0xff,0xff,0xff,0xff,0xff };
	uint8_t srcmac[ETH_ADDR_SIZE] = { 0x00,0x0a,0x35,0x00,0x1e,0x54 };
	char dstip[4] = { 0xc0,0xa8,0xff,0xff };
	char srcip[4] = { 0xc0,0xa8,0x02,0x01 };
	srcmac[5] = SELFID;
	ethernet_header_t* ehdr = (ethernet_header_t*)buf;
	ip_header* iphdr = (ip_header*)(buf + sizeof(ethernet_header_t));
	udp_header* udphdr = (udp_header*)(buf + sizeof(ethernet_header_t) + sizeof(ip_header));

	Smgmt_header* hmsg;
	Snodefind* snodefind;
	int node_num = 0;
	int offset = 0;
	int* ipaddr = 0;

	uint8_t  ts_time_slot_color[NET_SIZE*2];
	

	static uint8_t mcs_stat;          // MCS统计状态
	char bcrecv_buf[BUFLEN];          // 广播接收缓冲区
	int bc_len=0;                     // 广播数据长度
	int socklen;                      // socket长度
	struct sockaddr_in from;          // 发送者地址
	bcMeshInfo *meshinfo_recv;        // 接收的mesh信息指针
	printf("调用 mgmt_get_msg函数\r\n");

    // ===== 设置参数相关 =====
	uint8_t cmd[200];                 // 系统命令缓冲区
	INT8 buffer[sizeof(Smgmt_header) + sizeof(Smgmt_set_param)];  // 参数设置报文缓冲区
	INT32 buflen = sizeof(Smgmt_header) + sizeof(Smgmt_set_param);  // 缓冲区长度
	Smgmt_header* mhead = (Smgmt_header*)buffer;    // 管理头指针
	Smgmt_set_param* mparam = (Smgmt_set_param*)mhead->mgmt_data;  // 参数设置指针
	uint8_t version_compare[20];      // 版本比较字符串

    // ===== 初始化报文缓冲区 =====
	bzero(buffer, buflen);            // 清空参数设置报文缓冲区
	memset(cmd,0,sizeof(cmd));        // 清空命令缓冲区
	mhead->mgmt_head = htons(HEAD);   // 设置管理报文头标识
	mhead->mgmt_len = sizeof(Smgmt_set_param);    // 设置报文长度
	mhead->mgmt_type = 0;             // 初始化报文类型

    // ===== 数据库更新数据结构初始化 =====
	stInData stsysteminfodata;        // 系统信息数据结构
    stSurveyInfo stsurveyinfodata;    // 调查信息数据结构 no use
	stLink   stlinkdata;              // 链路信息数据结构
	stNode   stnode;                  // 节点信息数据结构 no use
	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));  // 清空系统信息
	memset((char*)&stsurveyinfodata,0,sizeof(stsurveyinfodata));  // 清空调查信息
	memset((char*)&stlinkdata,0,sizeof(stLink));  // 清空链路信息
	memset((char*)&stnode,0,sizeof(stNode));      // 清空节点信息

	// ===== 初始化邻居信息数组 =====
	memset(neighid_info,0,sizeof(neighid_info));  // 清空邻居ID信息
    
    // ===== 构造管理报文 =====
	hmsg = (Smgmt_header*)(buf + sizeof(ethernet_header_t) + sizeof(ip_header) + sizeof(udp_header));  // 指向管理头位置
	snodefind = (Snodefind*)hmsg->mgmt_data;   // 指向节点发现数据位置
	hmsg->mgmt_head = htons(HEAD);    // 设置管理报文标识
	hmsg->mgmt_type = htons(MGMT_NODEFIND);  // 设置报文类型为节点发现
	hmsg->mgmt_keep = 0;              

	srcip[3] = SELFID;                // 设置源IP最后一字节为本节点ID


	memcpy(&SELFIP, SELFIP_s, sizeof(uint32_t));  // 复制本节点IP地址 
    // ===== 测试打印本节点IP =====
	struct in_addr selfaddr;
	selfaddr.s_addr = SELFIP;
	printf("SELFIP为:%s\n", inet_ntoa(selfaddr));

    // ===== 构造以太网帧头 =====
	memcpy(ehdr->dest_mac_addr, dstmac, ETH_ADDR_SIZE);  // 设置目的MAC地址（广播）
	memcpy(ehdr->src_mac_addr, srcmac, ETH_ADDR_SIZE);   // 设置源MAC地址
	ehdr->ethertype = htons(0x0800);                          // 设置以太网类型（IPv4协议）

    // ===== 构造IP头 =====
	iphdr = (ip_header*)((void*)ehdr + ETH_HLEN);
	iphdr->ver_ihl = (4 << 4 | sizeof(ip_header) / sizeof(unsigned long));  // 版本4，IHL=5
	iphdr->tos = 0;               // 不区分服务
	iphdr->tlen = htons(sizeof(ip_header));  // 初始化总长度
	iphdr->identification = 1;    // 标识字段
	iphdr->flags_fo = 0;          // 标志和片偏移为0
	iphdr->ttl = 50;              // 生存时间
	iphdr->proto = IPPROTO_UDP;   // 协议类型为UDP
	iphdr->crc = 0;               // 初始化校验和
	memcpy((char*)&iphdr->saddr, srcip, 4);    // 设置源IP地址
	memcpy((char*)&iphdr->daddr, dstip, 4);    // 设置目的IP地址（广播）

	// ===== 构造UDP头 =====
	udphdr = (udp_header*)((void*)iphdr + sizeof(ip_header));
	udphdr->sport = htons(CTRL_PORT);              // 设置UDP源端口（16000）
	//udphdr->dport = htons(15008);
	udphdr->dport = htons(WG_TX_UDP_PORT);               // 设置UDP目的端口（7700）
	udphdr->len = 0;                           // UDP长度初始化
	udphdr->crc = 0x0000;                      // UDP校验和初始化

	// ===== 初始化所有邻居链路信息 =====
	/* 预初始化所有32个节点的链路表，将其清零 */
	for(j=1;j<33;j++)
	{
		// 清空该节点在系统表中的信息
		reset_systeminfo_table(j);
		
		// 清空该节点在链路表中的信息（SNR、接收电平、吞吐率）
		memset((char*)&stlinkdata,0,sizeof(stLink));
		stlinkdata.m_stNbInfo[j-1].nbid1=j;            // 邻居节点ID
		stlinkdata.m_stNbInfo[j-1].snr1=0;             // 信噪比为0
		stlinkdata.m_stNbInfo[j-1].getlv1=0;           // 接收电平为0
		stlinkdata.m_stNbInfo[j-1].flowrate1=0;        // 吞吐率为0
		updateData_linkinfo(&stlinkdata,j-1,SELFID);   // 更新数据库
	}

	while(TRUE){
		// ===== 清空消息结构体 =====
		bzero(&self_msg, sizeof(struct mgmt_send));  // 清空自身消息结构
		node_num = 1;           // 初始化节点计数为1（本节点）
		offset = sizeof(ethernet_header_t) + sizeof(ip_header) + sizeof(udp_header) + sizeof(Smgmt_header) + sizeof(Snodefind);
		// ===== 从内核获取网络信息 =====
		mgmt_netlink_get_info(0, MGMT_CMD_GET_ROUTE_INFO, NULL, (char*)&route_msg);   // 获取路由表信息
		mgmt_netlink_get_info(0, MGMT_CMD_GET_VETH_INFO, NULL, (char*)&self_msg);    // 获取虚拟网卡信息
		
		// ===== 设置报文序列号和节点ID =====
		self_msg.seqno = seqno;        // 设置自身消息序列号
		self_msg.node_id = SELFID;     // 设置自身节点ID
		if (seqno == 0xffffffff)       // 序列号溢出处理
			seqno = 0;
		else
			seqno++;

		// ===== 构造节点发现报文 =====
		snodefind->selfid = htons(SELFID);        // 设置本节点ID（网络字节序）
		snodefind->selfip = iphdr->saddr;         // 设置本节点IP地址
		printf("node_%d has %d neigh\r\n",SELFID,self_msg.neigh_num);  // 打印邻居节点数
		
		// ===== 统计邻居信息 =====
		memset(neighid_info,0,sizeof(neighid_info));  // 清空邻居ID数组
		for (i = 0; i < self_msg.neigh_num; i++)      // 遍历所有邻居
		{
			if (self_msg.msg[i].mcs != 0x0f && self_msg.msg[i].node_id != SELFID)  // 有效邻居判断
				{
					//printf("%d mcs %d\n", i, self_msg.msg[i].mcs);
					node_num++;
					ipaddr = (int*)(buf + offset); 
					*ipaddr = htonl(0xc0a80200 + self_msg.msg[i].node_id);
					offset += sizeof(int);
				/* 存下邻居信息 */
					neighid_info[i]=self_msg.msg[i].node_id;
					mcs_all[i]=self_msg.msg[i].mcs;  //存放组网的所有mcs
					//rssi_all[i]=self_msg.msg[i].rssi;
					// printf("neigh:%d mcs:%d\t",i,mcs_all[i]);
				}
		}

		// ===== 计算报文长度并设置各层协议头 =====
		snodefind->node_num = htons(node_num);   // 设置节点总数（包括本身）
		len = offset;                             // 计算最终报文长度
		
		// 计算各层报文长度（从后向前）
		hmsg->mgmt_len = htons(len - (sizeof(ethernet_header_t) + sizeof(ip_header) + sizeof(udp_header) + sizeof(Smgmt_header)));
		udphdr->len = htons(len - sizeof(ethernet_header_t) - sizeof(ip_header));
		iphdr->tlen = htons(len - sizeof(ethernet_header_t));
		iphdr->crc = ipCksum((void*)iphdr, 20);   // 计算IP头校验和

		// ===== 根据节点角色发送拓扑信息 =====
		// 网关节点持续向网管发送自身拓扑包
		if (is_conned == 1) {
			// 网管查询完节点信息就和节点连接上，网管地址会赋值到全局的wg_addr，节点持续发送
			send_topo_msg(wg_addr, self_msg);     // 发送拓扑消息给网管
			send_topo_request();                   // 发送拓扑请求

		}
		// 邻居节点收到网关节点的拓扑信息请求，则持续向请求节点发送拓扑信息
		if (gotRequest == 1) {
			send_topo_msg(gate_addr, self_msg);   // 发送拓扑消息给网关
		}
		// ===== 版本一致性检查 =====
		/* 检查veth（虚拟网卡）、agent（代理）、ctrl（控制）模块版本是否一致 */
		
		// 检查虚拟网卡版本
		memset(version_compare,0,sizeof(version_compare));
		sprintf(version_compare,"V%d.%d.%d",self_msg.veth_version[1],self_msg.veth_version[2],self_msg.veth_version[3]);
		if(strcmp(version, version_compare) != 0)
		{
			// 版本不一致，可在此处处理
		}

		// 检查代理模块版本
		memset(version_compare,0,sizeof(version_compare));
		sprintf(version_compare,"V%d.%d.%d",self_msg.agent_version[1],self_msg.agent_version[2],self_msg.agent_version[3]);
		if(strcmp(version, version_compare) != 0)
		{
			// agent版本不一致
		}

		// 检查控制模块版本
		memset(version_compare,0,sizeof(version_compare));
		sprintf(version_compare,"V%d.%d.%d",self_msg.ctrl_version[1],self_msg.ctrl_version[2],self_msg.ctrl_version[3]);
			if(strcmp(version, version_compare) != 0)
			{
				//printf("The version of mac-ctrl is inconsistent %s\n",version_compare);
			}

		
		//更新数据库
		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","ipaddr");
		sprintf(stsysteminfodata.value,"%d.%d.%d.%d",SELFIP_s[0],SELFIP_s[1],SELFIP_s[2],SELFIP_s[3]);
		stsysteminfodata.state[0] = '1';
		updateData_systeminfo(stsysteminfodata);

		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","device");
		sprintf(stsysteminfodata.value,"%d",SELFID);
		stsysteminfodata.state[0] = '1';
		updateData_systeminfo(stsysteminfodata);

		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","g_ver");
		sprintf(stsysteminfodata.value,"%s",version);
		stsysteminfodata.state[0] = '1';
		updateData_systeminfo(stsysteminfodata);	
		
		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","rf_freq");
		sprintf(stsysteminfodata.value,"%d",FREQ_INIT);
		stsysteminfodata.state[0] = '0';
		updateData_systeminfo(stsysteminfodata);

		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","m_chanbw");
		sprintf(stsysteminfodata.value,"%d",BW_INIT);
		stsysteminfodata.state[0] = '0';
		updateData_systeminfo(stsysteminfodata);

		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","m_txpower");
		sprintf(stsysteminfodata.value,"%d",POWER_INIT);
		stsysteminfodata.state[0] = '0';
		updateData_systeminfo(stsysteminfodata);

		memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
		sprintf(stsysteminfodata.name,"%s","devicetype");
		sprintf(stsysteminfodata.value,"%d",DEVICETYPE_INIT);
		stsysteminfodata.state[0] = '0';
		updateData_meshinfo(stsysteminfodata);

		/*
		*  根据邻居数量更新数据库中的邻居信息
		 *  如果没有邻居，则将所有邻居信息置0
		 *  如果有邻居，则检查每个节点是否为邻居，非邻居节点信息置0，邻居节点信息保持不变
		 *  通过neighid_info数组判断哪些节点是邻居，哪些不是邻居
		 *  更新数据库中的系统信息表和链路信息表
		 *  系统信息表中存储邻居数量等基本信息，链路信息表中存储每个邻居的信噪比、接收电平、吞吐率等详细信息
		 *
		*/
		if(self_msg.neigh_num==0)
		{
			/* 不存在邻居，刷新数据库*/
			//printf("refresh systeminfo neighbor info \r\n");
			for(j=1;j<33;j++)
			{
				/*  clear systemInfo table */

				reset_systeminfo_table(j);
				
				/*  clear link table */
				memset((char*)&stlinkdata,0,sizeof(stLink));
				stlinkdata.m_stNbInfo[j-1].nbid1=j;
				stlinkdata.m_stNbInfo[j-1].snr1=0;
				stlinkdata.m_stNbInfo[j-1].getlv1=0;
				stlinkdata.m_stNbInfo[j-1].flowrate1=0;
				updateData_linkinfo(&stlinkdata,j-1,SELFID);

			}
		}
		else{
			for(j=1;j<33;j++){
				id_index=neighid_isexit(neighid_info,sizeof(neighid_info),j);
				if(id_index<0)
				{
					/* 非邻居节点，全部置0*/
					//update systemInfo table
					reset_systeminfo_table(j);

					//update link table
					memset((char*)&stlinkdata,0,sizeof(stLink));
					stlinkdata.m_stNbInfo[j-1].nbid1=j;
					stlinkdata.m_stNbInfo[j-1].snr1=0;
					stlinkdata.m_stNbInfo[j-1].getlv1=0;
					stlinkdata.m_stNbInfo[j-1].flowrate1=0;
					updateData_linkinfo(&stlinkdata,j-1,SELFID);
					//continue;
				}
				else{
					if(self_msg.msg[id_index].mcs != 0x0f && self_msg.msg[id_index].node_id != SELFID)
					{
						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",id[self_msg.msg[id_index].node_id-1]);  //name : deviceX
						//printf("update name %s\r\n",stsysteminfodata.name);
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].node_id);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",ip[j-1]);  //name : ipX
						sprintf(stsysteminfodata.value,"192.168.2.%d",self_msg.msg[id_index].node_id);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",timejitter[j-1]);  //name : timejitterX
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].time_jitter);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",snr[j-1]);  //name : snrX
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].snr);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",rssi[j-1]);  //name : rssiX
						sprintf(stsysteminfodata.value,"%d",-self_msg.msg[id_index].rssi);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",linkquality[j-1]);  //name : linkqualityX
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].mcs);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);//更新本地 SQLite 数据库中 systemInfo 表数据

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",good[j-1]);  //name : goodX
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].good);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",noise[j-1]);  //name : ucdsX
						sprintf(stsysteminfodata.value,"%d",-self_msg.msg[id_index].noise);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",bad[j-1]);  //name : ucdsX
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].bad);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
						sprintf(stsysteminfodata.name,"%s",ucds[j-1]);  //name : deviceX
						sprintf(stsysteminfodata.value,"%d",self_msg.msg[id_index].ucds);
						stsysteminfodata.state[0] = '1';
						updateData_systeminfo(stsysteminfodata);

						for(k=1;k<33;k++)
						{
							if(self_msg.mac_information_part1.nbr_list[k] == LinkSt_h2)
							{
								memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
								sprintf(stsysteminfodata.name,"%s",id2hop[j-1]);  //name : id2X
								sprintf(stsysteminfodata.value,"%d",k);
								stsysteminfodata.state[0] = '1';
								updateData_systeminfo(stsysteminfodata);


								memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
								sprintf(stsysteminfodata.name,"%s",ip2hop[j-1]);  //name : ip2X
								sprintf(stsysteminfodata.value,"192.168.2.%d",k);
								stsysteminfodata.state[0] = '1';
								updateData_systeminfo(stsysteminfodata);
							}
						}

						//update link table
						memset((char*)&stlinkdata,0,sizeof(stLink));
						stlinkdata.m_stNbInfo[id_index].nbid1=self_msg.msg[id_index].node_id;
						//printf("update neigh_%d link info\r\n ",stlinkdata.m_stNbInfo[id_index].nbid1);
						stlinkdata.m_stNbInfo[id_index].snr1=self_msg.msg[id_index].snr;
						stlinkdata.m_stNbInfo[id_index].getlv1=-self_msg.msg[id_index].rssi;
						stlinkdata.m_stNbInfo[id_index].flowrate1=10;
						updateData_linkinfo(&stlinkdata,id_index,SELFID);

					}
				}
			}
			////rate auto mode
			if(rate_auto==1){
				if(mcs_stat!=find_minMcs(mcs_all,self_msg.neigh_num))
				{
					/* mcs档位需要切换 */
					printf("mcs update ");
					bzero(buffer, buflen);//清空参数设置报文缓冲区
					memset(cmd,0,sizeof(cmd));
					mhead->mgmt_head = htons(HEAD);
					mhead->mgmt_len = sizeof(Smgmt_set_param);
					mhead->mgmt_type = 0;
					//mhead->mgmt_type |= MGMT_SET_MULTICAST_MCS;
					//mparam->mgmt_virt_multicast_mcs=mcs;

					mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
					mparam->mgmt_virt_unicast_mcs=find_minMcs(mcs_all,self_msg.neigh_num);
					printf("set param mcs:%d\r\n",mparam->mgmt_virt_unicast_mcs);
					mhead->mgmt_type = htons(mhead->mgmt_type);
					mgmt_netlink_set_param(buffer, buflen,NULL);
					sleep(1);		
					if (!persist_test_db()) {
						printf("[mgmt_transmit] persist test.db failed after auto MCS update\n");
					}

					mcs_stat=find_minMcs(mcs_all,self_msg.neigh_num);

					memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
					sprintf(stsysteminfodata.name,"%s","m_rate");
					sprintf(stsysteminfodata.value,"%d",mparam->mgmt_virt_unicast_mcs);
					stsysteminfodata.state[0] = '1';
					
					updateData_systeminfo(stsysteminfodata);
				}
			}



		}

		memset(ts_time_slot_color,1,NET_SIZE*2);

		ts_time_slot_color[SELFID] = 0;

		update_time_slot_table(&(self_msg.mac_information_part1),ts_time_slot_color);//更新数据库中的时隙表
		
		for(j=0;j<NET_SIZE*2;j++)
		{
			updateData_timeslotinfo(ts_time_slot_color[j],j+1);
		}	
		sleep(5);  //间隔5秒写库
	}

}

void mgmt_recv_web(void){
	INT8 buffer[BUFLEN];
	INT32 buflen = 0;
	Smgmt_header* hmsg;
	struct mgmt_header* hmgmt;
	Smgmt_set_param* sparam;

	while(TRUE){
		buflen = msgrcv(MSG_MGMT, buffer, BUFLEN, MSG_TYPE, 0);

		//		printf("MSG_MGMT getdata\n");

		if (buflen < sizeof(Smgmt_header))
			continue;
		hmsg = (Smgmt_header*)(buffer + sizeof(long));

		if (ntohs(hmsg->mgmt_head) != HEAD) {
			//			printf("1\n");
			continue;
		}
		switch (ntohs(hmsg->mgmt_type)) {
			case MGMT_LOGIN: {
				//			S_GROUND = from;
				ISLOGIN = TRUE;
				//			printf("login\n");
				break;
			}
			default: {
				//			printf("2\n");
				sparam = (Smgmt_set_param*)hmsg->mgmt_data;
				//			printf("%d %d %d %d\n",ntohl(sparam->mgmt_mac_freq),ntohs(sparam->mgmt_mac_txpower),sparam->mgmt_virt_unicast_mcs,sparam->mgmt_mac_bw);
				mgmt_netlink_set_param(buffer + sizeof(long), buflen - sizeof(long), NULL);
				break;
			}
		}
	}


}



void send_topo_msg(struct sockaddr_in from, struct mgmt_send self_msg) {
	struct sockaddr_in towg = from;
	printf("send_topo_msg往网管:%s 发送拓扑数据包\n", inet_ntoa(towg.sin_addr));
	Smgmt_header* topo_header_ptr;
	struct topo_data* topomsgptr;
	struct neighbor_data* neighbormsgptr;
	char topobuff[2048];

	// 拓扑数据包包头->网关节点信息包->网关节点邻居的数据包
	topo_header_ptr = (Smgmt_header*)topobuff;
	//1、拓扑数据包包头
	topo_header_ptr->mgmt_head = htons(HEAD);
	topo_header_ptr->mgmt_type = htons(MGMT_TOPOLOGY_INFO);
	topo_header_ptr->mgmt_keep = 0;

	topomsgptr = (topo_data*)topo_header_ptr->mgmt_data;

	//读取配置文件中的经纬度
	FILE* file_node;
	char nodemessage[100];
	char messagename[100];

	if ((file_node = fopen("/etc/node_xwg", "r")) == NULL) {
		longitude = 118.76;
		latitude = 32.04;

	}
	else {

		while (fgets(nodemessage, sizeof(nodemessage), file_node) != NULL) {
			bzero(messagename, sizeof(messagename));
			sscanf(nodemessage, "%s ", messagename);
			//add by yang
			if (strcmp(messagename, "longitude") == 0) {
				sscanf(nodemessage, "longitude %lf", &longitude);
				//printf("set ------- longitude = %lf\n", longitude);
			}
			if (strcmp(messagename, "latitude") == 0) {
				sscanf(nodemessage, "latitude %lf", &latitude);
				//printf("set ------- latitude = %lf\n", latitude);
			}
		}
	}
	fclose(file_node);

	//邻居数据包
	neighbormsgptr = (struct neighbor_data*)(topobuff + sizeof(Smgmt_header) + sizeof(topo_data));
	int topo_len = 0;

	//2、拓扑数据包数据内容1：网关节点信息包
	topomsgptr->selfip = SELFIP;
	topomsgptr->longitude = htond(longitude);
	topomsgptr->latitude = htond(latitude);
	topomsgptr->noise = 0;
	topomsgptr->tx_traffic = htonl(self_msg.tx);
	topomsgptr->rx_traffic = htonl(self_msg.rx);
	topomsgptr->neighbors_num = htonl(self_msg.neigh_num);

	if (self_msg.neigh_num == 0) {
		topo_len = sizeof(Smgmt_header) + sizeof(topo_data);
		topo_header_ptr->mgmt_len = htons(topo_len - sizeof(Smgmt_header));
	}
	else {
		//3、拓扑数据包数据内容2：邻居的数据包
		topo_len = sizeof(Smgmt_header) + sizeof(topo_data) + sizeof(neighbor_data) * self_msg.neigh_num;
		char neigh_ip[4] = { 0xc0,0xa8,0x02,0x01 };
		for (int i = 0; i < self_msg.neigh_num; i++) {
			neigh_ip[3] = self_msg.msg[i].node_id;
			//printf("节点id:%d\n", self_msg.msg[i].node_id);//
			//printf("-----------------------------------------拷贝前邻居IP：%08x---------------------------------------\n", neigh_ip);
			memcpy((char*)&neighbormsgptr->neighbor_ip, neigh_ip, 4);
			//neighbormsgptr->neighbor_ip = htonl(neighbormsgptr->neighbor_ip);
			//printf("-----------------------------------------拷贝后邻居IP：%08x---------------------------------------\n", neighbormsgptr->neighbor_ip);
			neighbormsgptr->neighbor_rssi = htonl(self_msg.msg[i].rssi);
			//
			neighbormsgptr->neighbor_tx = 0;
			if ((i + 1) < self_msg.neigh_num) {
				neighbormsgptr++;
			}

		}
		topo_header_ptr->mgmt_len = htons(topo_len - sizeof(Smgmt_header));
	}
	int ret = SendUDPClient(SOCKET_UDP_WG, topobuff, topo_len, &towg);
}

void send_topo_request() {
	//printf("网关节点，发送拓扑请求\n");
	Smgmt_header* hmsg;
	topology_request* request;
	char buffer[1024];
	int broadcast = 1;
	int ret;

	hmsg = (Smgmt_header*)buffer;
	hmsg->mgmt_head = htons(HEAD);
	hmsg->mgmt_type = htons(MGMT_TOPOLOGY_REQUEST);
	hmsg->mgmt_keep = 0;
	hmsg->mgmt_len = sizeof(topology_request);
	char myIp[4] = { 0xc0,0xa8,0x02,0x01 };
	myIp[3] = SELFID;

	request = (topology_request*)hmsg->mgmt_data;
	request->srcIp = htonl(SELFIP);

	struct sockaddr_in toNeigh;
	toNeigh.sin_family = AF_INET;
	toNeigh.sin_addr.s_addr = INADDR_BROADCAST;
	//toNeigh.sin_addr.s_addr = inet_addr("192.168.255.255");
	toNeigh.sin_port = htons(WG_RX_UDP_PORT);//7600端口
	int len = sizeof(Smgmt_header) + sizeof(topology_request);
	// Enable broadcasting on socket
	//setsockopt函数的作用是设置套接字选项，这里我们使用它来启用套接字的广播功能。具体来说，SO_BROADCAST选项允许套接字发送广播消息，即发送到网络上的所有主机。
	ret = setsockopt(SOCKET_MGMT, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
	if (ret < 0) {
		perror("setsockopt(SO_BROADCAST) failed");
		return -1;
	}
	int sret = SendUDPBrocast(SOCKET_MGMT, buffer, len, &toNeigh);
}


int neighid_isexit(int *buf,int size,int target)
{
	int i=0;
	int index=-1;
	//printf("size:%d,compare %d is exit\r\n",size,target);
	for(i=0;i<32;i++)
	{
		
		if(*(buf+i)==target)
		{
			index=i;
//			printf("find no.%d neigh,id=%d\r\n",i,*(buf+i));
		}
		else
		{	
			//printf("%d is not neigh\r\n",i);
			continue;
		}
	}
	return index;
}

uint8_t find_minMcs(uint8_t *arr,int size)
{
	uint8_t min=0x0f;   
	int i=0;
	for(i=0;i<size;i++)
	{
		if(arr[i]<min)
		{
			min=arr[i];
		}
	}
	return min;
}

uint32_t find_max(uint32_t *arr,int size)
{
	uint32_t max=0;
	int i=0;
	for(i=0;i<size;i++)
	{
		if(arr[i]>max)
		{
			max=arr[i];
		}
	}
	return max;

}
void update_time_slot_table(ob_state_part1 * part1_data, uint8_t * time_slot_tb_infor)
{
	uint8_t  ts_n_used_l0[NET_SIZE*2];
	uint8_t  ts_n_free_hx[NET_SIZE*2];
	uint8_t  ts_n_ol0_hx[NET_SIZE*2];
	uint8_t  ts_n_free_h1[NET_SIZE*2];
	uint8_t  ts_n_ol0_h1[NET_SIZE*2];
	uint8_t  ts_n_free_h2[NET_SIZE*2];
	uint8_t  ts_n_ol0_h2[NET_SIZE*2];
	uint8_t  ts_n_used_l1[NET_SIZE*2];

	int offset = 0;
	int i,j;

	memset(ts_n_used_l0,0,NET_SIZE*2);
	memset(ts_n_free_hx,0,NET_SIZE*2);
	memset(ts_n_ol0_hx,0,NET_SIZE*2);
	memset(ts_n_free_h1,0,NET_SIZE*2);
	memset(ts_n_ol0_h1,0,NET_SIZE*2);
	memset(ts_n_free_h2,0,NET_SIZE*2);
	memset(ts_n_ol0_h2,0,NET_SIZE*2);
	memset(ts_n_used_l1,0,NET_SIZE*2);

//	printf("n_used_l0 = %d,n_ol0_hx=%d,n_free_hx=%d,n_free_h1=%d,n_ol0_h1=%d,n_free_h2=%d,n_ol0_h2=%d,n_used_l1=%d\n",
//	part1_data->n_used_l0,
//	part1_data->n_ol0_hx,
//	part1_data->n_free_hx,
//	part1_data->n_free_h1,
//	part1_data->n_ol0_h1,		
//	part1_data->n_free_h2,
//	part1_data->n_ol0_h2,			
//	part1_data->n_used_l1);

	//2
	memcpy((void *)ts_n_used_l0,(void *)&part1_data->slot_list[offset],part1_data->n_used_l0);

	//3
	offset = part1_data->n_used_l0;
	memcpy((void *)ts_n_free_hx,(void *)&part1_data->slot_list[offset],part1_data->n_free_hx);

	//4
	offset = part1_data->n_used_l0+part1_data->n_free_hx;
	memcpy((void *)ts_n_ol0_hx,(void *)&part1_data->slot_list[offset],part1_data->n_ol0_hx);

	//5
	offset = part1_data->n_used_l0+part1_data->n_free_hx+part1_data->n_ol0_hx;
	memcpy((void *)ts_n_free_h1,(void *)&part1_data->slot_list[offset],part1_data->n_free_h1);

	//6
	offset = part1_data->n_used_l0+part1_data->n_free_hx+part1_data->n_ol0_hx+part1_data->n_free_h1;
	memcpy((void *)ts_n_ol0_h1,(void *)&part1_data->slot_list[offset],part1_data->n_ol0_h1);

	//7
	offset = part1_data->n_used_l0+part1_data->n_free_hx+part1_data->n_ol0_hx+part1_data->n_free_h1+part1_data->n_ol0_h1;
	memcpy((void *)ts_n_free_h2,(void *)&part1_data->slot_list[offset],part1_data->n_free_h2);

	//8
	offset = part1_data->n_used_l0+part1_data->n_free_hx+part1_data->n_ol0_hx+part1_data->n_free_h1
		+part1_data->n_ol0_h1+part1_data->n_free_h2;
	memcpy((void *)ts_n_ol0_h2,(void *)&part1_data->slot_list[offset],part1_data->n_ol0_h2);

	//9
	offset = part1_data->n_used_l0+part1_data->n_free_hx+part1_data->n_ol0_hx+part1_data->n_free_h1
		+part1_data->n_ol0_h1+part1_data->n_free_h2+part1_data->n_ol0_h2;
	memcpy((void *)ts_n_used_l1,(void *)&part1_data->slot_list[offset],part1_data->n_used_l1);

	for(i=0;i<part1_data->n_used_l0;i++)
	{
		j = ts_n_used_l0[i];
		time_slot_tb_infor[j] = 2;
	}

	for(i=0;i<part1_data->n_free_hx;i++)
	{
		j = ts_n_free_hx[i];
		time_slot_tb_infor[j] = 3;
	}
	
	for(i=0;i<part1_data->n_ol0_hx;i++)
	{
		j = ts_n_ol0_hx[i];
		time_slot_tb_infor[j] = 4;
	}

	for(i=0;i<part1_data->n_free_h1;i++)
	{
		j = ts_n_free_h1[i];
		time_slot_tb_infor[j] = 5;
	}

	for(i=0;i<part1_data->n_ol0_h1;i++)
	{
		j = ts_n_ol0_h1[i];
		time_slot_tb_infor[j] = 6;
	}
	for(i=0;i<part1_data->n_free_h2;i++)
	{
		j = ts_n_free_h2[i];
		time_slot_tb_infor[j] = 7;
	}
	for(i=0;i<part1_data->n_ol0_h2;i++)
	{
		j = ts_n_ol0_h2[i];
		time_slot_tb_infor[j] = 8;
	}
	for(i=0;i<part1_data->n_used_l1;i++)
	{
		j = ts_n_used_l1[i];
		time_slot_tb_infor[j] = 9;
	}

}


void reset_systeminfo_table(int j)
{
	stInData stsysteminfodata;

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",id[j-1]);  //name : idX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",ip[j-1]);  //name : ipX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",timejitter[j-1]);  //name : timejitterX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",snr[j-1]);  //name : snrX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",rssi[j-1]);  //name : rssiX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",linkquality[j-1]);  //name : linkqualityX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);	

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",good[j-1]);  //name : goodX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",bad[j-1]);  //name : ucdsX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);	

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",ucds[j-1]);  //name : deviceX
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);

	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",id2hop[j-1]);  //name : id2X
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);


	memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
	sprintf(stsysteminfodata.name,"%s",ip2hop[j-1]);  //name : ip2X
	sprintf(stsysteminfodata.value,"%d",0);
	stsysteminfodata.state[0] = '0';
	updateData_systeminfo(stsysteminfodata);
}

void mgmt_recv_msg(void){
    INT32 GRet = -1, i;
    char selfAddr[4] = {0xc0,0xa8,0x02,0x01};
    uint32_t selfip;
    INT8 cmd[200];
    INT8 buffer[BUFLEN];
    fd_set fds;
    INT32 maxfdp = 0;
    INT32 buflen = 0;
    bool isset=FALSE;
    INT32 sock = 0;
    struct sockaddr_in from;
    INT32 socklen = sizeof(struct sockaddr_in);
    struct timeval TimeOut = { 1, 0 };  // 减少超时时间从20秒到1秒
	struct timeval timenow;
    int sysinfo_bw;
    int* ipdaddr;
    Smgmt_header* hmsg;
	struct mgmt_header* hmgmt;
    int ret = -1;
    Smgmt_header tcpmsg;
    INT8 filename[100];
    FILE* filefd = NULL;

    INT8 set_buf[sizeof(Smgmt_header) + sizeof(Smgmt_set_param)];
	INT32 set_len = sizeof(Smgmt_header) + sizeof(Smgmt_set_param);
	Smgmt_header* mhead = (Smgmt_header*)set_buf;
	Smgmt_set_param* mparam = (Smgmt_set_param*)mhead->mgmt_data;
	bzero(set_buf, set_len);
	memset(cmd,0,sizeof(cmd));
	mhead->mgmt_head = htons(HEAD);
	mhead->mgmt_len = sizeof(Smgmt_set_param);
	mhead->mgmt_type = 0;

    bcMeshInfo *meshinfo_recv=NULL;
	MEM_REQUEST_FRAME *info=NULL;


	//测试打印：
	printf("调用mgmt_recv_msg接收函数\n");

    selfAddr[3]=SELFID;//将id和ip绑定
	printf("seifid:%d\r\n",selfAddr[3]);
	memcpy(&selfip,selfAddr,sizeof(uint32_t));
	struct in_addr selfIP;
	selfIP.s_addr = selfip;

    stInData bc_systeminfoupdate;
	memset((char*)&bc_systeminfoupdate,0,sizeof(bc_systeminfoupdate));

    // 添加性能优化变量
	static int select_count = 0;
	static struct timeval last_select_time = {0, 0};
	struct timeval current_time;

    while(TRUE){
        sleep(1);
        GRet =1;
        memset(buffer,0,sizeof(buffer));
        // 优化：减少select调用频率，添加微秒级休眠
		while (GRet <= 0) {
			FD_ZERO(&fds);
			maxfdp = 0;
			FD_SET(SOCKET_MGMT, &fds);
			maxfdp = SOCKET_MGMT;
			FD_SET(SOCKET_GROUND, &fds);
			if (maxfdp < SOCKET_GROUND)
				maxfdp = SOCKET_GROUND;
			FD_SET(SOCKET_UDP_WG, &fds);
			if (maxfdp < SOCKET_UDP_WG)
				maxfdp = SOCKET_UDP_WG;
			FD_SET(SOCKET_TCP_WG, &fds);
			if (maxfdp < SOCKET_TCP_WG)
				maxfdp = SOCKET_TCP_WG;
			FD_SET(SOCKET_BCAST_RECV,&fds); 
			if (maxfdp < SOCKET_BCAST_RECV)
				maxfdp = SOCKET_BCAST_RECV;
			FD_SET(SOCKET_MEM_REQUEST_RECV,&fds);
			if (maxfdp < SOCKET_MEM_REQUEST_RECV)
				maxfdp = SOCKET_MEM_REQUEST_RECV;
			for (i = 0; i < CONNECTNUM; i++) {
				if (TCPCLIENT_WG[i].useful == TRUE) {
					FD_SET(TCPCLIENT_WG[i].sockfd, &fds);
					if (maxfdp < TCPCLIENT_WG[i].sockfd)
						maxfdp = TCPCLIENT_WG[i].sockfd;
				}
			}

			TimeOut.tv_sec = 1;  // 减少超时时间
			TimeOut.tv_usec = 500000;  // 增加500ms微秒级超时，减少CPU轮询
			GRet = select(maxfdp + 1, &fds, NULL, NULL, &TimeOut);
			
			// 如果没有事件，添加短暂休眠减少CPU占用
			if (GRet <= 0) {
				usleep(5000);  // 增加到5ms休眠，进一步降低CPU占用
			}
		}

        if(FD_ISSET(SOCKET_MEM_REQUEST_RECV, &fds))
		{
			buflen=	RecvUDPClient(SOCKET_MEM_REQUEST_RECV, buffer, BUFLEN, &from, &socklen);
			if(buflen>0)
			{
				if(selfip==inet_addr(inet_ntoa(from.sin_addr)))
				{
					continue;
				}
                //0a命令
				process_member_info(buffer,buflen);
			}
		}

        if(FD_ISSET(SOCKET_BCAST_RECV, &fds)){
            buflen = RecvUDPClient(SOCKET_BCAST_RECV,buffer,BUFLEN,&from,&socklen);
            if(buflen>0){
                //防止自己接受到自己发的消息
                if(selfip!=inet_addr(inet_ntoa(from.sin_addr))){
                    meshinfo_recv=(bcMeshInfo*)buffer;
                    memset(cmd,0,sizeof(cmd));
                    mhead->mgmt_head = htons(HEAD);
                    mhead->mgmt_len = sizeof(Smgmt_set_param);
                    mhead->mgmt_type = 0;
                    if(meshinfo_recv->txpower_isset){
                        isset=TRUE;
                        meshinfo_recv->txpower_isset = 0;
                        bool has_per_channel =false;
                        //检查是否已经设置了每个信道的功率值,配置了就不去本地表查询，全是0才去查询
                        for (int idx = 0; idx < POWER_CHANNEL_NUM; ++idx) {
							if (meshinfo_recv->m_txpower_ch[idx] != 0) {
								has_per_channel = true;
								break;
							}
						}
						if (!has_per_channel) {
							txpower_lookup_channels_be(ntohs(meshinfo_recv->m_txpower), meshinfo_recv->m_txpower_ch);
						}

						mhead->mgmt_type |= MGMT_SET_POWER;
						//mhead->mgmt_type = htons(mhead->mgmt_type);
						mparam->mgmt_mac_txpower=meshinfo_recv->m_txpower;
						memcpy(mparam->mgmt_mac_txpower_ch, meshinfo_recv->m_txpower_ch, sizeof(meshinfo_recv->m_txpower_ch));
						//mgmt_netlink_set_param(set_buf, set_len,NULL);
                        //接收到组播包后，更新systeminfo库
						memset((char*)&bc_systeminfoupdate,0,sizeof(bc_systeminfoupdate));
						sprintf(bc_systeminfoupdate.name,"%s","m_txpower");
						sprintf(bc_systeminfoupdate.value,"%d",39-meshinfo_recv->sys_power);
						bc_systeminfoupdate.state[0] = '1';
						updateData_systeminfo(bc_systeminfoupdate);
                    }
                    if(meshinfo_recv->freq_isset)
					{
						// printf("receive bcast packet,set freq\r\n");
						isset=TRUE;
						meshinfo_recv->freq_isset=0;
						mhead->mgmt_type |= MGMT_SET_FREQUENCY;
						//mhead->mgmt_type = htons(mhead->mgmt_type);
						mparam->mgmt_mac_freq=meshinfo_recv->rf_freq;
						//printf("set freq %d \r\n",mparam->mgmt_mac_freq);
						//mgmt_netlink_set_param(set_buf, set_len,NULL);

						memset((char*)&bc_systeminfoupdate,0,sizeof(bc_systeminfoupdate));
						sprintf(bc_systeminfoupdate.name,"%s","rf_freq");
						sprintf(bc_systeminfoupdate.value,"%d",meshinfo_recv->sys_freq);
						bc_systeminfoupdate.state[0] = '1';
						updateData_systeminfo(bc_systeminfoupdate);
                    
					}
                    if(meshinfo_recv->chanbw_isset)
					{
						printf("receive bcast packet,set chanbw\r\n");
						isset=TRUE;
						meshinfo_recv->chanbw_isset=0;
						mhead->mgmt_type |= MGMT_SET_BANDWIDTH;
						//mhead->mgmt_type = htons(mhead->mgmt_type);
						mparam->mgmt_mac_bw=meshinfo_recv->m_chanbw;
						//printf("set bw %d \r\n",mparam->mgmt_mac_bw);
						//mgmt_netlink_set_param(set_buf, set_len,NULL);
                        #if 0
						if(meshinfo_recv->sys_bw==0)
						{
							sysinfo_bw=20;
						}
						else if(meshinfo_recv->sys_bw==1)
						{
							sysinfo_bw=10;
						}
						else if(meshinfo_recv->sys_bw==2)
						{
							sysinfo_bw=5;
						}
						else;
                        #endif
                        sysinfo_bw=meshinfo_recv->sys_bw;
						memset((char*)&bc_systeminfoupdate,0,sizeof(bc_systeminfoupdate));
						sprintf(bc_systeminfoupdate.name,"%s","m_chanbw");
						sprintf(bc_systeminfoupdate.value,"%d",sysinfo_bw);
						bc_systeminfoupdate.state[0] = '1';
						updateData_systeminfo(bc_systeminfoupdate);						
					}
                    if(meshinfo_recv->rate_isset)
					{
						printf("receive bcast packet,set rate\r\n");
						isset=TRUE;
						meshinfo_recv->rate_isset=0;
						mhead->mgmt_type |= MGMT_SET_UNICAST_MCS;
						//mhead->mgmt_type = htons(mhead->mgmt_type);
						mparam->mgmt_virt_unicast_mcs=meshinfo_recv->m_rate;
						//printf("set rate %d \r\n",mparam->mgmt_virt_unicast_mcs);
						//mgmt_netlink_set_param(set_buf, set_len,NULL);
						memset((char*)&bc_systeminfoupdate,0,sizeof(bc_systeminfoupdate));
						sprintf(bc_systeminfoupdate.name,"%s","m_rate");
						sprintf(bc_systeminfoupdate.value,"%d",meshinfo_recv->sys_rate);
						bc_systeminfoupdate.state[0] = '1';
						updateData_systeminfo(bc_systeminfoupdate);						
					}
                    if(isset)
					{
						isset=FALSE;
						mhead->mgmt_type = htons(mhead->mgmt_type);
						mgmt_netlink_set_param(set_buf, set_len,NULL);
						sleep(1);
						if (!persist_test_db()) {
							printf("[mgmt_transmit] persist test.db failed after broadcast packet\n");
						}
					}
                }
            }
        }

        if(FD_ISSET(SOCKET_TCP_WG, &fds)){
            sock = OnlineMonitor(SOCKET_TCP_WG, &from);
			if (sock > 0) {
				gettimeofday(&timenow, NULL);
				Lock(&TCPCLIENTMUTEX_WG, 0);

				for (i = 0; i < CONNECTNUM; i++) {
					if (TCPCLIENT_WG[i].useful == FALSE) {
						TCPCLIENT_WG[i].sockfd = sock;
						TCPCLIENT_WG[i].useful = TRUE;
						TCPCLIENT_WG[i].srcip = from.sin_addr.s_addr;
						TCPCLIENT_WG[i].time.tv_sec = timenow.tv_sec;
						TCPCLIENT_WG[i].time.tv_usec = timenow.tv_usec;
						break;
					}
				}
				Unlock(&TCPCLIENTMUTEX_WG);
			}
        }

        if (FD_ISSET(SOCKET_GROUND, &fds)) {
			//测试打印：
			//printf("监听到SOCKET_GROUND变化\n");
			buflen = RecvUDPClient(SOCKET_GROUND, buffer, BUFLEN, &from, &socklen);//from为什么没绑定也能用
			if (buflen < sizeof(Smgmt_header))
				continue;
			hmsg = (Smgmt_header*)buffer;

			if (ntohs(hmsg->mgmt_head) != HEAD) {
				continue;
			}
			switch (ntohs(hmsg->mgmt_type)) {
			case MGMT_LOGIN: {
				S_GROUND = from;
				ISLOGIN = TRUE;
				printf("login\n");
				break;
			}
			default: {
				mgmt_netlink_set_param(buffer, buflen, NULL);
				break;
			}
			}
			//			printf("recv msg\n");
		}

        if (FD_ISSET(SOCKET_UDP_WG, &fds)){
            //测试打印： 
			//printf("监听到SOCKET_UDP_WG变化，端口是7600\n");
			buflen = RecvUDPClient(SOCKET_UDP_WG, buffer, BUFLEN, &from, &socklen);
            //测试打印：
			//printf("接收源IP:%s  接收源端口:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
			if (buflen < sizeof(Smgmt_header))
				continue;
			hmsg = (Smgmt_header*)buffer;

			if (ntohs(hmsg->mgmt_head) != HEAD) {
				continue;
			}
            switch (ntohs(hmsg->mgmt_type)) {
                case MGMT_DEVINFO:{
                    //测试打印：------------------------
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_DEVINFO %x\n", MGMT_DEVINFO);
                    ipdaddr = (int*)(hmsg->mgmt_data);
                    //打印selfip
                    struct in_addr selfaddr;
                    selfaddr.s_addr = (uint32_t)SELFIP;
                    //printf("selfip:%s\n", inet_ntoa(selfaddr));
                    //打印ipdaddr
                    //printf("hmsg->mgmt_data接收到的数据内容中IP为：");
                    struct in_addr recvaddr;
                    //recvaddr.s_addr = htonl((uint32_t)*ipdaddr);
                    recvaddr.s_addr = (uint32_t)*ipdaddr;
                    //printf("%s\n", inet_ntoa(recvaddr));
                    //测试打印--------------------------
                    if (SELFIP == *ipdaddr)
                    {
                        //测试打印
                        //printf("调用mgmt_status_report(from)函数\n");
                        //printf("收到状态查询包hmsg->mgmt_keep ：%u\n", hmsg->mgmt_keep);
                        //keep == 0为网关节点，1为其他邻居
                        if (hmsg->mgmt_keep == 0) {
                            is_conned = 1;
                        }
                        wg_addr = from;
                        
                        mgmt_status_report(from);
                    }
                    else {
                        //测试打印
                        //printf("调用SendUDPClient发送至S_OTHER_NODE\n");
                        hmsg->mgmt_keep = 1;
                        //printf("是否将hmsg->mgmt_keep置1？ ：%u\n", hmsg->mgmt_keep);
                        S_OTHER_NODE.sin_addr.s_addr = *ipdaddr;
                        SendUDPClient(SOCKET_UDP_WG, buffer, buflen, &S_OTHER_NODE);
                    }
				    break;
                }
                case MGMT_DEVINFO_REPORT:
                {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_DEVINFO_REPORT %x\n", MGMT_DEVINFO_REPORT);
                    SendUDPClient(SOCKET_UDP_WG, buffer, buflen, &wg_addr);
                    break;
                }
                case MGMT_SET_PARAM:
                {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_SET_PARAM %x\n", MGMT_SET_PARAM);
                    ipdaddr = (int*)(hmsg->mgmt_data);
                    if (SELFIP == *ipdaddr)
                    {
                        mgmt_netlink_set_param_wg(hmsg->mgmt_data + sizeof(int), ntohs(hmsg->mgmt_len) - sizeof(int), NULL,MGMT_SET_PARAM);
                    }
                    else {
                        S_OTHER_NODE.sin_addr.s_addr = *ipdaddr;
                        SendUDPClient(SOCKET_UDP_WG, buffer, buflen, &S_OTHER_NODE);
                    }
                    break;
                }
                case MGMT_SPECTRUM_QUERY:
                {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_SPECTRUM_QUERY %x\n", MGMT_SPECTRUM_QUERY);
                    //频谱查询
                    break;
                }
                case MGMT_POWEROFF:
                {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_POWEROFF %x\n", MGMT_POWEROFF);
                    system("poweroff");
                    break;
                }
                case MGMT_RESTART:
                {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type为：%x\n", MGMT_RESTART);
                    system("reboot");
                    break;
                }
                case MGMT_FACTORY_RESET:
                {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_FACTORY_RESET %x\n", MGMT_FACTORY_RESET);
                    system("sh /etc/init.sh");
                    break;
                }
                case MGMT_MULTIPOINT_SET:
                {
                    //测试打印：配置消息
                    //printf("接收到的消息类型hmsg->mgmt_type为：MGMT_MULTIPOINT_SET %x\n", MGMT_MULTIPOINT_SET);
                    uint16_t nodenum = ntohs(hmsg->mgmt_keep);
                    for (i = 0; i < nodenum; i++)
                    {
                        ipdaddr = (int*)(hmsg->mgmt_data);
                        //测试打印--------------------------------
                        struct in_addr prtaddr;
                        prtaddr.s_addr = (uint32_t)*ipdaddr;
                        //printf("接收到的ipdaddr地址是:%s\n", inet_ntoa(prtaddr));
                        prtaddr.s_addr = SELFIP;
                        //printf("SELFIP地址是:%s\n", inet_ntoa(prtaddr));
                        //测试打印-------------------------------
                        if (SELFIP == *ipdaddr)
                        {
                            //printf("进入SELFIP == *ipdaddr\n");
                            //printf("nodenum:%hd\n", nodenum);
                            //printf("(hmsg->mgmt_len:%hd\n", ntohs(hmsg->mgmt_len));
                            mgmt_netlink_set_param_wg(hmsg->mgmt_data + sizeof(int) * nodenum, ntohs(hmsg->mgmt_len) - sizeof(int) * nodenum, NULL,MGMT_MULTIPOINT_SET);
                        }
                        else {
                            //printf("进入else的S_OTHER_NODE\n");
                            S_OTHER_NODE.sin_addr.s_addr = *ipdaddr;
                            SendUDPClient(SOCKET_UDP_WG, buffer, buflen, &S_OTHER_NODE);
                        }
                        ipdaddr++;
                    }
                    break;
                }
                case MGMT_TOPOLOGY_REQUEST:
                {
                    //测试打印：
                    //printf("接收到拓扑请求：MGMT_TOPOLOGY_REQUEST %x\n", MGMT_TOPOLOGY_REQUEST);
                    //邻居节点接收到拓扑请求，将收到请求置为1
                    topology_request* trptr = (topology_request*)&hmsg->mgmt_data;
                    //测试打印----------
                    struct in_addr broaddr;
                    broaddr.s_addr = htonl(trptr->srcIp);
                    //printf("广播包携带IP：%s\n", inet_ntoa(broaddr));
                    //测试打印----------
                    if (htonl(trptr->srcIp) != SELFIP) {
                        //printf("gotRequest 置为 1\n");
                        gotRequest = 1;
                    }

                    memcpy(&gate_addr, &from, sizeof(from));
                    //将拓扑信息目的IP替换成广播包中携带的网关IP
                    gate_addr.sin_addr.s_addr = htonl(trptr->srcIp);
                    gate_addr.sin_port = htons(WG_RX_UDP_PORT);//7600端口为什么用htons？而不是htonl
                    break;
                }
			    case MGMT_TOPOLOGY_INFO:
                {
                    //测试打印：------------------------
                    //printf("接收到邻居拓扑信息：MGMT_TOPOLOGY_INFO %x\n", MGMT_TOPOLOGY_INFO);
                    ipdaddr = (int*)(hmsg->mgmt_data);
                    //打印selfip
                    struct in_addr selfaddr;
                    selfaddr.s_addr = (uint32_t)SELFIP;
                    //printf("selfip:%s\n", inet_ntoa(selfaddr));
                    //打印ipdaddr
                    //printf("hmsg->mgmt_data接收到的数据内容中IP为：\n");
                    struct in_addr recvaddr;
                    //recvaddr.s_addr = htonl((uint32_t)*ipdaddr);
                    recvaddr.s_addr = (uint32_t)*ipdaddr;
                    //printf("%s\n", inet_ntoa(recvaddr));
                    //测试打印--------------------------
                    if (SELFIP != *ipdaddr)
                    {
                        //测试打印
                        //printf("转发拓扑信息包\n");
                        //网关节点接收到邻居节点的拓扑信息，则转发到网管
                        SendUDPClient(SOCKET_UDP_WG, buffer, buflen, &wg_addr);
                    }
                    break;
                }
                default: {
                    //测试打印：
                    //printf("接收到的消息类型hmsg->mgmt_type跳转到了default\n");
                    break;
			    }


            }

        }
        if (FD_ISSET(SOCKET_MGMT, &fds)) {
			//测试打印：
			//printf("监听到SOCKET_MGMT变化\n");
			buflen = RecvUDPClient(SOCKET_MGMT, buffer, BUFLEN, &from, &socklen);
			//printf("recv SOCKET_MGMT buflen %d\n",buflen);
			if (buflen <= 12)
				continue;
			hmgmt = (struct mgmt_header*)buffer;
			//			mgmt_mysql_write((int)hmgmt->node_id,buffer,buflen);
						//SendUDPClient(SOCKET_GROUND,buffer,buflen,&S_GROUND_PC);
			SendNodeMsg(buffer, buflen);
		}
        for (i = 0; i < CONNECTNUM; i++) {
           if (TCPCLIENT_WG[i].useful != TRUE) {
				continue;
			} 
            else{
                if (FD_ISSET(TCPCLIENT_WG[i].sockfd, &fds)) {
                    ret = RecvTCPServer(TCPCLIENT_WG[i].sockfd,
						(INT8*)&tcpmsg, sizeof(tcpmsg));//读取包头，获得长度
					if (ret != RETURN_OK) {
						Lock(&TCPCLIENTMUTEX_WG, 0);
						CloseTCPSocket(TCPCLIENT_WG[i].sockfd);
						TCPCLIENT_WG[i].useful = FALSE;
						TCPCLIENT_WG[i].sockfd = 0;
						TCPCLIENT_WG[i].srcip = 0;
						TCPCLIENT_WG[i].time.tv_sec = 0;
						TCPCLIENT_WG[i].time.tv_usec = 0;
						Unlock(&TCPCLIENTMUTEX_WG);
						continue;
					}
                    if (ntohs(tcpmsg.mgmt_head) != HEAD) {
						//printf("tcp tcpmsg.head = %d\n", tcpmsg.mgmt_head);
						continue;
					}
                    bzero(buffer, sizeof(buffer));
					ret = RecvTCPServer(TCPCLIENT_WG[i].sockfd, buffer,
						ntohs(tcpmsg.mgmt_len));//读取载荷长度
					switch (ntohs(tcpmsg.mgmt_type)){
                    case MGMT_FIRMWARE_UPDATE: {
                            switch (ntohs(tcpmsg.mgmt_keep)) {
                            case MGMT_NAME: {
                                sprintf(filename, "/root/%s", buffer);
                                while ((filefd = fopen(filename, "wb")) == NULL) {
                                    usleep(10000);
                                }
                                break;
                            }
                            case MGMT_CONTENT: {
                                if (filefd != NULL)
                                    fwrite(buffer, tcpmsg.mgmt_len, 1,
                                        filefd);
                                break;
                            }
                            case MGMT_END: {
                                if (filefd == NULL)
                                    break;
                                fwrite(buffer, tcpmsg.mgmt_len, 1, filefd);
                                fclose(filefd);
                                bzero(cmd, sizeof(cmd));
                                sprintf(cmd, "opkg install %s", filename);
                                system(cmd);
                                bzero(cmd, sizeof(cmd));
                                sprintf(cmd, "rm %s", filename);
                                system(cmd);
                                bzero(cmd, sizeof(cmd));
                                bzero(filename, sizeof(filename));

                                break;
                            }
                            case MGMT_UPDATE_FIRMWARE: {
                                ret = system(buffer);
                                if (ret != -1) {
                                    bzero(cmd, sizeof(cmd));
                                    sprintf(cmd, "opkg remove %s", buffer);
                                    system(cmd);
                                }
                                break;
                            }
                            default:
                                break;
                            }
                            break;
                        }
					case MGMT_FILE_UPDATE: {
                            switch (ntohs(tcpmsg.mgmt_keep)) {
                                case MGMT_FILENAME: {
                                    sprintf(filename, "/root/%s", buffer);
                                    while ((filefd = fopen(filename, "wb")) == NULL) {
                                        usleep(10000);
                                    }
                                    break;
                                }
                                case MGMT_FILECONTENT: {
                                    if (filefd != NULL)
                                        fwrite(buffer, tcpmsg.mgmt_len, 1,
                                            filefd);
                                    break;
                                }
                                case MGMT_FILEEND: {
                                    if (filefd == NULL)
                                        break;
                                    fwrite(buffer, tcpmsg.mgmt_len, 1, filefd);
                                    fclose(filefd);
                                    bzero(cmd, sizeof(cmd));
                                    sprintf(cmd, "opkg install %s", filename);
                                    system(cmd);
                                    bzero(cmd, sizeof(cmd));
                                    sprintf(cmd, "rm %s", filename);
                                    system(cmd);
                                    bzero(cmd, sizeof(cmd));
                                    bzero(filename, sizeof(filename));

                                    break;
                                }
                                case MGMT_UPDATE_FILE: {
                                    ret = system(buffer);
                                    if (ret != -1) {
                                        bzero(cmd, sizeof(cmd));
                                        sprintf(cmd, "opkg remove %s", buffer);
                                        system(cmd);
                                    }
                                    break;
                                }
                                default:
                                    break;
                                }
                                break;
                            }
                            default:
                                break;
                    }
                    
                }
            }

        }
        usleep(1000);
    }
}


void mgmt_status_report(struct sockaddr_in from) {
    struct sockaddr_in report_addr = from;
	mgmt_status_header ms_header;
	mgmt_status_data* ms_data;
	char buff[2048];

    char selfip[4] = { 0xc0,0xa8,0x02,0x01 };
	selfip[3] = SELFID;

    // 创建结构体指针
	mgmt_status_header* pms_header = malloc(sizeof(mgmt_status_header));
	//状态包头部填充
	pms_header->flag = htons(HEAD);
	pms_header->type = htons(MGMT_DEVINFO_REPORT);
	pms_header->reserved = 0;

    //状态包内容填充.
	ms_data = &(pms_header->status_data);
	ms_data->selfid = htons(SELFID);
	memcpy((char*)&ms_data->selfip, selfip, 4);
	//ms_data->selfip = htonl(ms_data->selfip);
	ms_data->tv_route = htons(1000);
	ms_data->maxHop = htons(50);
	ms_data->num_queues = htons(100);
	ms_data->depth_queues = htons(100);
	ms_data->qos_policy = 0;
	ms_data->mcs_unicast = MCS_INIT;
	ms_data->mcs_broadcast = 0;
	ms_data->bw = BW_INIT;
	ms_data->reserved = 0;
	ms_data->freq = htonl(FREQ_INIT);
	ms_data->txpower = htons(POWER_INIT);
	ms_data->work_mode = htons(MACMODE_INIT);
	//ms_data->longitude = double_to_network(118.76);
	//ms_data->latitude = double_to_network(32.04);
	ms_data->longitude = htond(longitude);
	ms_data->latitude = htond(latitude);
	strcpy(ms_data->software_version, "version");
	strcpy(ms_data->hardware_version, "version");

	pms_header->len = htons(sizeof(mgmt_status_data) + 8);

    //测试打印-------------------------------
	printf("ms_header数据---------------------\n");
	printf("sizeof(ms_header) = %d\n", sizeof(ms_header));
	printf("%04x ", pms_header->flag);
	printf("%04x ", pms_header->len);
	printf("%04x ", pms_header->type);
	printf("%04x ", pms_header->reserved);
	printf("数据内容：\n");
	printf("%08x ", ms_data->selfip);
	printf("%04x ", ms_data->selfid);
	printf("%04x ", ms_data->tv_route);
	printf("%04x ", ms_data->maxHop);
	printf("%04x ", ms_data->num_queues);
	printf("%04x ", ms_data->qos_policy);
	printf("%02x ", ms_data->mcs_unicast);
	printf("%02x ", ms_data->mcs_broadcast);
	printf("%02x ", ms_data->bw);
	printf("%02x ", ms_data->reserved);
	printf("freq: %d,%08x ", ms_data->freq, ms_data->freq);//00007805，正确应该是00000578
	printf("%04x ", ms_data->txpower);
	printf("%04x ", ms_data->work_mode);
	printf("%016x ", ms_data->longitude);
	printf("%016x ", ms_data->latitude);
	printf("%s ", ms_data->software_version);
	printf("%s ", ms_data->hardware_version);
	printf("\n");
	printf("ms_header数据---------------------\n");
	printf("buff拷贝前数据---------------------\n");
	printf("%s", buff);
	printf("\n");
	printf("buff拷贝前数据---------------------\n");
	printf("\n");
	//测试打印-------------------------------	

	memcpy(buff, pms_header, sizeof(mgmt_status_header));

	printf("buff拷贝后数据---------------------\n");
	printf("拷贝的数据大小：%d\n", sizeof(mgmt_status_header));
	printf("%s", buff);
	printf("\n");
	printf("buff拷贝后数据---------------------\n");

	int ret = SendUDPClient(SOCKET_UDP_WG, buff, sizeof(ms_header), &report_addr);
	printf("状态数据包socket发送情况: %d\n", ret);
	printf("状态数据包socket发送端口%u\n", ntohs(report_addr.sin_port));
}

void SendNodeMsg(void* data, int datalen) {
	int len = sizeof(Smgmt_header) + datalen;
	char buf[len];
	Smgmt_header* hmgmt = (Smgmt_header*)buf;
	memset(buf, 0, len);
	hmgmt->mgmt_head = htons(HEAD);
	hmgmt->mgmt_type = htons(NODEPARAMTYPE);
	hmgmt->mgmt_len = htons(datalen);
	memcpy(hmgmt->mgmt_data, data, datalen);

	//SendUDPClient(SOCKET_GROUND,buf,len,&S_GROUND);
//	printf("SendNodeMsg len %d\n",len);
}




static const uint16_t g_default_txpower_table[POWER_TABLE_SIZE][POWER_CHANNEL_NUM] = {
	{0, 0, 0, 0},
	{1, 1, 1, 1},
	{2, 2, 2, 2},
	{3, 3, 3, 3},
	{4, 4, 4, 4},
	{5, 5, 5, 5},
	{6, 6, 6, 6},
	{7, 7, 7, 7},
	{8, 8, 8, 8},
	{9, 9, 9, 9},
	{10, 10, 10, 10},
	{11, 11, 11, 11},
	{12, 12, 12, 12},
	{13, 13, 13, 13},
	{14, 14, 14, 14},
	{15, 15, 15, 15},
	{16, 16, 16, 16},
	{17, 17, 17, 17},
	{18, 18, 18, 18},
	{19, 19, 19, 19},
	{20, 20, 20, 20},
	{21, 21, 21, 21},
	{22, 22, 22, 22},
	{23, 23, 23, 23},
	{24, 24, 24, 24},
	{25, 25, 25, 25},
	{26, 26, 26, 26},
	{27, 27, 27, 27},
	{28, 28, 28, 28},
	{29, 29, 29, 29},
	{30, 30, 30, 30},
	{31, 31, 31, 31},
	{32, 32, 32, 32},
	{33, 33, 33, 33},
	{34, 34, 34, 34},
	{35, 35, 35, 35},
	{36, 36, 36, 36},
	{37, 37, 37, 37},
	{38, 38, 38, 38},
	{39, 39, 39, 39},
	{40, 40, 40, 40},
	{41, 41, 41, 41},
	{42, 42, 42, 42},
	{43, 43, 43, 43},
	{44, 44, 44, 44},
	{45, 45, 45, 45},
	{46, 46, 46, 46},
	{47, 47, 47, 47},
	{48, 48, 48, 48},
	{49, 49, 49, 49},
	{50, 50, 50, 50},
	{51, 51, 51, 51},
	{52, 52, 52, 52},
	{53, 53, 53, 53},
	{54, 54, 54, 54},
	{55, 55, 55, 55},
	{56, 56, 56, 56},
	{57, 57, 57, 57},
	{58, 58, 58, 58},
	{59, 59, 59, 59},
	{60, 60, 60, 60},
	{61, 61, 61, 61},
	{62, 62, 62, 62},
	{63, 63, 63, 63},
	{64, 64, 64, 64},
	{65, 65, 65, 65},
	{66, 66, 66, 66},
	{67, 67, 67, 67},
	{68, 68, 68, 68},
	{69, 69, 69, 69},
	{70, 70, 70, 70},
};

static void txpower_table_use_defaults(void){
    memcpy(g_txpower_table, g_default_txpower_table, sizeof(g_txpower_table));
}

static void txpower_table_init(void){
    if (g_txpower_table_initialized)
		return;
    txpower_table_use_defaults();

    FILE* fp=fopen(TXPOWER_TABLE_FILE,"r");
    if(!fp){
        printf("txpower: unable to open %s, using default table\n", TXPOWER_TABLE_FILE);
		g_txpower_table_initialized = true;//即使文件打开失败了，我们也不想每次都尝试打开了，所以标记为已初始化
        return;
    }
    char line[256];
    uint16_t row=0;
    while(row <POWER_TABLE_SIZE && fgets(line,sizeof(line),fp)){
        char* ptr=line;
        while(*ptr == ' '|| *ptr == '\t') ptr++;//跳过行首空白
        if(*ptr == '#' || *ptr == '\n' || *ptr == '\0') continue;//跳过注释行和空行
        unsigned int values[POWER_CHANNEL_NUM] = {0};

        int count = sscanf(ptr,"%u %u %u %u", &values[0], &values[1], &values[2], &values[3]);
        if(count <=0){
            continue;//解析失败，跳过
        }
        // 容错处理1：如果文件里只写了 1 个数 (比如该行只有 "20")，就让 4 个通道都等于这 1 个数
        if (count == 1) {
            values[1] = values[0];
            values[2] = values[0];
            values[3] = values[0];
        }
        // 容错处理2：如果写了 2 个或者 3 个数，缺失的通道就用最后一个读到的数补齐
        else {
            unsigned int last = values[count - 1];
            for (int i = count; i < POWER_CHANNEL_NUM; ++i)
                values[i] = last;
        }
        // 把读到的值装填进最终内存表，并防止越界溢出
        for (int i = 0; i < POWER_CHANNEL_NUM; ++i) {
            if (values[i] > UINT16_MAX)
                values[i] = UINT16_MAX;
            g_txpower_table[row][i] = (uint16_t)values[i];
        }
        row++; // 这行有效，准备解析下一行
    }

    if (row < POWER_TABLE_SIZE) {
		printf("txpower: %s provides %u rows (<%u), remaining rows use defaults\n",
			TXPOWER_TABLE_FILE, row, POWER_TABLE_SIZE);
	}

    fclose(fp);
	g_txpower_table_initialized = true;

}


//API：供业务逻辑调用（主机字节序）
void txpower_lookup_channels(uint16_t single, uint16_t channel_power[POWER_CHANNEL_NUM]){
    txpower_table_init();
    uint16_t index = single;
    if(index>=POWER_TABLE_SIZE)
    {
        index=POWER_TABLE_SIZE-1;
    }
    memcpy(channel_power, g_txpower_table[index], sizeof(uint16_t)*POWER_CHANNEL_NUM);
}
// API：供组包发送调用（网络大端序）
void txpower_lookup_channels_be(uint16_t single, uint16_t channel_power[POWER_CHANNEL_NUM])
{
    uint16_t host_values[POWER_CHANNEL_NUM];
    txpower_lookup_channels(single, host_values);
    for (int i = 0; i < POWER_CHANNEL_NUM; ++i) {
        channel_power[i] = htons(host_values[i]); // 转换成发给网卡的网络字节序
    }
}

//将这些初始值刷入数据库中，以便UI界面能正确显示当前的参数状态
void updateData_init(void){
    stInData stsysteminfodata;// 用来暂存从数据库读出的数据，准备下发到内核
    memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
    sprintf(stsysteminfodata.name,"%s","rf_freq");
    sprintf(stsysteminfodata.value,"%d",FREQ_INIT);
    stsysteminfodata.state[0] = '0';
    updateData_meshinfo(stsysteminfodata);
    
    memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
    sprintf(stsysteminfodata.name,"%s","m_txpower");
    sprintf(stsysteminfodata.value,"%d",POWER_INIT);
    stsysteminfodata.state[0] = '0';
    updateData_meshinfo(stsysteminfodata);
    
    memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
    sprintf(stsysteminfodata.name,"%s","m_chanbw");
    sprintf(stsysteminfodata.value,"%d",BW_INIT);
    stsysteminfodata.state[0] = '0';
    updateData_meshinfo(stsysteminfodata);
    
    memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
    sprintf(stsysteminfodata.name,"%s","workmode");
    sprintf(stsysteminfodata.value,"%d",NET_WORKMOD_INIT);
    stsysteminfodata.state[0] = '0';
    updateData_meshinfo(stsysteminfodata);

    memset((char*)&stsysteminfodata,0,sizeof(stsysteminfodata));
    sprintf(stsysteminfodata.name,"%s","devicetype");
    sprintf(stsysteminfodata.value,"%d",DEVICETYPE_INIT);
    stsysteminfodata.state[0] = '0';
    updateData_meshinfo(stsysteminfodata);
}



void read_node_xwg_file(const char * filename,Node_Xwg_Pairs* xwg_info,int num_pairs)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("无法打开文件");
        return;
    }	

	for (int i = 0; i < num_pairs; i++) {
        xwg_info[i].found = 0;
    }

    char line[1024];
    while(fgets(line,sizeof(line),file)){
        //移除行尾的换行符
        line[strcspn(line, "\r\n")] = 0;
        //解析key=value
        char *key = strtok(line," ");
        char *value_str = strtok(NULL, " ");

        if(key !=NULL && value_str !=NULL)
        {
            for(int i=0;i<num_pairs;i++)
            {
                if(strcmp(key,xwg_info[i].key)==0)
                {
                    memcpy(xwg_info[i].value,value_str,100);
                    xwg_info[i].found = 1;
                    break;
                }
            }
        }
    }
	fclose(file);
}

const char *get_value(Node_Xwg_Pairs* pairs,const char *key) {
    for (int i = 0; i < MAX_XWG_PAIRS; i++) {
        if (strcmp(key, pairs[i].key) == 0 && pairs[i].found) {
            return pairs[i].value;
        }
    }
    return NULL;
}

double get_double_value(Node_Xwg_Pairs* pairs,const char *key)
{
	const char *value = get_value(pairs,key);
    if (value != NULL) {
        return atof(value);
    }
    return 0.0;

}

int get_int_value(Node_Xwg_Pairs* pairs,const char* key)
{
    const char *value = get_value(pairs,key);
    if (value != NULL) {
        return atoi(value);
    }
    return -1;
}