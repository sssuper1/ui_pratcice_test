#include "mgmt_netlink.h"
#include "mgmt_types.h"
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include "mgmt_transmit.h"
#include <stdbool.h>
#include <errno.h>
#include "sim_heartbeat.h"

#define MIN_TXPOWER 0
#define MAX_TXPOWER 70
#define MIN_BW		0
#define MAX_BW		4


static int last_err;
struct print_opts {
	int read_opt;
	float orig_timeout;
	float watch_interval;
	nl_recvmsg_msg_cb_t callback;
	char* remaining_header;      // 存放接收数据的内存指针
	const char* static_header;   // 静态的头部信息
	uint8_t nl_cmd;              // 当前执行的 Netlink 命令字
};


static const struct nla_policy mgmt_netlink_policy[NUM_MGMT_ATTR] = {
	[MGMT_ATTR_GET_INFO] = {.type = NLA_U32 },
	[MGMT_ATTR_SET_PARAM] = {.type = NLA_U32 },
	[MGMT_ATTR_NODEID] = {.type = NLA_U8 },
	[MGMT_ATTR_VERSION_ROUTE] = {.type = NLA_STRING },
	[MGMT_ATTR_PKTNUMB_ROUTE] = {.type = NLA_STRING },
	[MGMT_ATTR_TABLE_ROUTE] = {.maxlen = sizeof(struct routetable) },
	[MGMT_ATTR_UCDS_ROUTE] = {.type = NLA_STRING },

	[MGMT_ATTR_VERSION_VETH] = {.type = NLA_STRING },
	[MGMT_ATTR_INFO_VETH] = {.type = NLA_STRING,.maxlen = sizeof(jgk_report_infor) },

//	[MGMT_ATTR_INFO_VETH2]	      = {
//									  .type = NLA_STRING,
//									  .maxlen = sizeof(jgk_report_infor12) },
	[MGMT_ATTR_VETH_ADDRESS] = {.type = NLA_UNSPEC,.minlen = ETH_ALEN,.maxlen = ETH_ALEN },
	[MGMT_ATTR_VETH_TX] = {.type = NLA_U32 },
	[MGMT_ATTR_VETH_RX] = {.type = NLA_U32 },
	[MGMT_ATTR_VERSION_MAC_PHY] = {.type = NLA_STRING },
	[MGMT_ATTR_FREQ] = {.type = NLA_U32 },
	[MGMT_ATTR_BW] = {.type = NLA_U16 },
	[MGMT_ATTR_TXPOWER] = {.type = NLA_S16 },
	[MGMT_ATTR_SET_NODEID] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_INTERVAL] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_TTL] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_QUEUE_NUM] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_QUEUE_LENGTH] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_QOS_STATEGY] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_UNICAST_MCS] = {.type = NLA_U8 },
	[MGMT_ATTR_SET_MULTICAST_MCS] = {.type = NLA_U8 },
	[MGMT_ATTR_SET_FREQUENCY] = {.type = NLA_U32 },
	[MGMT_ATTR_SET_POWER] = {.type = NLA_BINARY}, 
	[MGMT_ATTR_SET_BANDWIDTH] = {.type = NLA_U8 },
	[MGMT_ATTR_SET_TEST_MODE] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_TEST_MODE_MCS] = {.type = NLA_U16 },
	[MGMT_ATTR_SET_PHY] = {.type = NLA_STRING,.maxlen = sizeof(Smgmt_phy)},
	[MGMT_ATTR_SET_WORKMODE] = {.type = NLA_STRING,.maxlen = sizeof(Smgmt_net_work_mode)},
	[MGMT_ATTR_SET_IQ_CATCH] = {.type = NLA_STRING,.maxlen = sizeof(Smgmt_IQ_Catch)},
	[MGMT_ATTR_SET_SLOTLEN] = {.type = NLA_U8 },
	[MGMT_ATTR_SET_POWER_LEVEL] = {.type = NLA_U8 },
	[MGMT_ATTR_SET_POWER_ATTENUATION] = {.type = NLA_U8 },
	[MGMT_ATTR_SET_RX_CHANNEL_MODE] = {.type = NLA_U8 },
#ifdef Radio_CEC
	[MGMT_ATTR_SET_NCU] = {.type = NLA_U8 },

#endif
};

// 补全缺失的设置参数回调函数 ssq
static int mgmt_netlink_param_callback(struct nl_msg* msg, void* arg)
{
    struct print_opts* opts = arg;
    
    // 调试打印：确认内核已经成功收到了刚才下发的 SET_PARAM 命令
    printf("[Netlink] 内核已成功响应命令 (cmd: %d)\n", opts->nl_cmd);

    // 对于下发配置而言，收到内核合法的回复就已经代表成功了，
    // 不需要像 get_info 那样去解析一大堆网卡数据，直接告诉 libnl 停止接收即可。
    return NL_STOP;
}

static int print_error(struct sockaddr_nl* nla, struct nlmsgerr* nlerr, void* arg)
{
	if (nlerr->error != -EOPNOTSUPP)
		fprintf(stderr, "Error received: %s\n",
			strerror(-nlerr->error));

	last_err = nlerr->error;

	return NL_STOP;
}

char mgmt_netlink_set_param(char* buffer, int buflen, const char* header)
{
	//测试打印------------------------------
	printf("调用mgmt_netlink_set_param函数\n");
    
    if (sim_get_mode()) {
        sim_set_param(buffer, buflen, 0);
        return 0;
    }

	//测试打印------------------------------
	Smgmt_header* hmsg;
	Smgmt_set_param* sparam;
	int paramdata = 0;
	//	int routeparamdata = 0;
	//	int virtparamdata = 0;
	struct nl_sock* sock;
	struct nl_msg* msg;
	struct nl_cb* cb;
	int family;
	uint8_t cmd[200];
	char ret = -1;
	int i,j;
	int row_cnt;
	struct print_opts opts = {
		.read_opt = 0,
		.nl_cmd = MGMT_CMD_SET_PARAM,
		.remaining_header = NULL,
		.static_header = header,
	};
	memset(cmd,0,sizeof(cmd));
	hmsg = (Smgmt_header*)buffer;
	sparam = (Smgmt_set_param*)hmsg->mgmt_data;

	//测试打印-------------------------------
	printf("Smgmt_headerr数据---------------------\n");
//	printf("sizeof(Smgmt_header) = %d\n", sizeof(hmsg));
	printf("%04x ", hmsg->mgmt_head);
	printf("%04x ", hmsg->mgmt_len);
	printf("%04x ", hmsg->mgmt_type);
	printf("%04x ", hmsg->mgmt_keep);
	printf("数据内容：\n");
	printf("%04x ", sparam->mgmt_id);
	printf("%04x ", sparam->mgmt_route_interval);
	printf("%04x ", sparam->mgmt_route_ttl);
	printf("%04x ", sparam->mgmt_virt_queue_num);
	printf("%04x ", sparam->mgmt_virt_queue_length);
	printf("%04x ", sparam->mgmt_virt_qos_stategy);
	printf("%02x ", sparam->mgmt_virt_unicast_mcs);
	printf("%02x ", sparam->mgmt_virt_multicast_mcs);
	printf("%02x ", sparam->mgmt_mac_bw);
	printf("%02x ", sparam->reserved);
	printf("%08x ", sparam->mgmt_mac_freq);
	printf("%04x ", sparam->mgmt_mac_txpower);
	printf("%04x ", sparam->mgmt_mac_work_mode);
	printf("%02x ", sparam->mgmt_net_work_mode);
	printf("%02x ", sparam->u8Slotlen);
	printf("\n");
	//测试打印-------------------------------
	//TODO: 解析参数并下发到内核
	sock = nl_socket_alloc();//分配一个netlink套接字
	if (!sock)
		return ret;

	genl_connect(sock);//连接到generic netlink子系统

	family = genl_ctrl_resolve(sock, MGMT_NL_NAME);
	if (family < 0) {
		printf("family error\n");
		nl_socket_free(sock);
		return ret;
	}

	msg = nlmsg_alloc();//分配一个netlink消息体空间
	if (!msg) {
		nl_socket_free(sock);
		return ret;
	}

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, 0,
		MGMT_CMD_SET_PARAM, 1);
	paramdata = ntohs(hmsg->mgmt_type);//获取参数类型

	if(ntohs(hmsg->mgmt_keep) & MGMT_SET_SLOTLEN)
	{
		paramdata |= MGMT_SET_SLOTLEN << 16;
		//printf("Smgmt_header paramdata = %#x\n",paramdata);
	}
	if(ntohs(hmsg->mgmt_keep) & MGMT_SET_POWER_LEVEL)
	{
		paramdata |= MGMT_SET_POWER_LEVEL << 16;
	}
	if(ntohs(hmsg->mgmt_keep) & MGMT_SET_POWER_ATTENUATION)
	{
		paramdata |= MGMT_SET_POWER_ATTENUATION << 16;
	}
	if(ntohs(hmsg->mgmt_keep) & MGMT_SET_RX_CHANNEL_MODE)
	{
		paramdata |= MGMT_SET_RX_CHANNEL_MODE << 16;
	}


	if (ntohs(hmsg->mgmt_type) & MGMT_SET_ID) {
		//		paramdata |= SET_ID;
		//		nla_put_u16(msg,MGMT_ATTR_SET_NODEID,ntohs(sparam->mgmt_id));
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_INTERVAL) {
		printf("设置MGMT_SET_INTERVAL参数:%04x\n", sparam->mgmt_route_interval);
		//		routeparamdata |= ROUTE_SET_INTERVAL;
		nla_put_u16(msg, MGMT_ATTR_SET_INTERVAL, ntohs(sparam->mgmt_route_interval));
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_TTL) {
		printf("设置MGMT_SET_TTL参数:%04x\n", sparam->mgmt_route_ttl);
		//		routeparamdata |= ROUTE_SET_TTL;
		nla_put_u16(msg, MGMT_ATTR_SET_TTL, ntohs(sparam->mgmt_route_ttl));
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_QUEUE_NUM) {
		printf("设置MGMT_SET_QUEUE_NUM参数:%04x\n", sparam->mgmt_virt_queue_num);
		//		virtparamdata |= VETH_SET_QUEUE_NUM;
		nla_put_u16(msg, MGMT_ATTR_SET_QUEUE_NUM, ntohs(sparam->mgmt_virt_queue_num));
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_QUEUE_LENGTH) {
		printf("设置MGMT_SET_QUEUE_LENGTH参数:%04x\n", sparam->mgmt_virt_queue_length);
		//		virtparamdata |= VETH_SET_QUEUE_LENGTH;
		nla_put_u16(msg, MGMT_ATTR_SET_QUEUE_LENGTH, ntohs(sparam->mgmt_virt_queue_length));
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_QOS_STATEGY) {
		printf("设置MGMT_SET_QOS_STATEGY参数:%04x\n", sparam->mgmt_virt_qos_stategy);
		//		virtparamdata |= VETH_SET_QOS_STATEGY;
		nla_put_u16(msg, MGMT_ATTR_SET_QOS_STATEGY, ntohs(sparam->mgmt_virt_qos_stategy));
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_UNICAST_MCS) {
		printf("设置MGMT_SET_UNICAST_MCS参数%02x\n", sparam->mgmt_virt_unicast_mcs);
		sprintf(cmd,
			"sed -i \"s/mcs .*/mcs %d/g\" /etc/node_xwg",
			sparam->mgmt_virt_unicast_mcs);
		system(cmd);
		
		nla_put_u8(msg, MGMT_ATTR_SET_UNICAST_MCS, sparam->mgmt_virt_unicast_mcs);
		MCS_INIT = sparam->mgmt_virt_unicast_mcs;
	}
	
	
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_MULTICAST_MCS) {
		printf("设置MGMT_SET_MULTICAST_MCS参数%02x\n", sparam->mgmt_virt_multicast_mcs);
		nla_put_u8(msg, MGMT_ATTR_SET_MULTICAST_MCS, sparam->mgmt_virt_multicast_mcs);
	}

	if (ntohs(hmsg->mgmt_type) & MGMT_SET_WORKMODE) {
		// 1表示定频，2表示跳频，3表示认知跳频
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,
			"sed -i \"s/networkmode.*/networkmode %d/g\" /etc/node_xwg",
			sparam->mgmt_net_work_mode.NET_work_mode);
		
		system(cmd);
//		nla_put_u8(msg, MGMT_ATTR_SET_WORKMODE, sparam->mgmt_net_work_mode);
		printf("设置工作模式为%d\n",sparam->mgmt_net_work_mode.NET_work_mode);
		NET_WORKMOD_INIT = sparam->mgmt_net_work_mode.NET_work_mode;
	
		if(sparam->mgmt_net_work_mode.NET_work_mode == HOP_FREQ_MODE){
			//将跳频表初始化为默认值
			memcpy((void *)sparam->mgmt_net_work_mode.hop_freq_tb,(void *)HOP_FREQ_TB_INIT,sizeof(HOP_FREQ_TB_INIT));
			row_cnt = 0;
			for(i=0;i<4;i++)
			{
				row_cnt++;
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,
					"sed -i \"%ds/.*/%d %d %d %d %d %d %d %d /g\" /etc/node_hop",
					row_cnt,
					HOP_FREQ_TB_INIT[i*8], HOP_FREQ_TB_INIT[i*8+1],
					HOP_FREQ_TB_INIT[i*8+2], HOP_FREQ_TB_INIT[i*8+3],
					HOP_FREQ_TB_INIT[i*8+4], HOP_FREQ_TB_INIT[i*8+5],
					HOP_FREQ_TB_INIT[i*8+6], HOP_FREQ_TB_INIT[i*8+7]);
				system(cmd);
			}
			sparam->mgmt_net_work_mode.fh_len = 0;
			for(i = 0 ; i < HOP_FREQ_NUM ; i ++)
			{
				if(0 == HOP_FREQ_TB_INIT[i])
				{
					break;
				}
				else
				{
					sparam->mgmt_net_work_mode.fh_len ++;
				}
			}//观察到默认跳频表中非0的频点个数就是有效的跳频表长度，更新到结构体中
		}
		else if(sparam->mgmt_net_work_mode.NET_work_mode == FIX_FREQ_MODE)
		{
			/*如果只有定频模式，但是没有具体的频率值，就会下发之前的频率值，不会更新文件*/
			if((ntohs(hmsg->mgmt_type) & MGMT_SET_FREQUENCY) == 0)
			{
				nla_put_u32(msg, MGMT_ATTR_SET_FREQUENCY, FREQ_INIT);
				paramdata |= MGMT_SET_FREQUENCY;
			}
		}

		nla_put(msg, MGMT_ATTR_SET_WORKMODE, sizeof(Smgmt_net_work_mode), (void *)&(sparam->mgmt_net_work_mode));

	}
	//设置了新频率 同时处在定频模式下，需要更新配置文件中的频率信息，并将频率参数下发到内核
	if((ntohs(hmsg->mgmt_type) & MGMT_SET_FREQUENCY) && (NET_WORKMOD_INIT == FIX_FREQ_MODE)){
		memset(cmd,0,sizeof(cmd));
		printf("设置MGMT_SET_FREQUENCY参数--%d\n", ntohl(sparam->mgmt_mac_freq));

		sprintf(cmd,
			"sed -i \"s/channel .*/channel %d/g\" /etc/node_xwg",
			ntohl(sparam->mgmt_mac_freq));
		system(cmd);
		nla_put_u32(msg, MGMT_ATTR_SET_FREQUENCY, ntohl(sparam->mgmt_mac_freq));
		FREQ_INIT = ntohl(sparam->mgmt_mac_freq);
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_POWER) {
		memset(cmd,0,sizeof(cmd));

		if (ntohs(sparam->mgmt_mac_txpower) < MIN_TXPOWER)
			sparam->mgmt_mac_txpower = htons(MIN_TXPOWER);
		if (ntohs(sparam->mgmt_mac_txpower) > MAX_TXPOWER)
			sparam->mgmt_mac_txpower = htons(MAX_TXPOWER);

		printf("设置MGMT_SET_POWER参数--%d\n", ntohs(sparam->mgmt_mac_txpower));

		sprintf(cmd,
			"sed -i \"s/power .*/power %d/g\" /etc/node_xwg",
			ntohs(sparam->mgmt_mac_txpower));
		system(cmd);
		uint16_t txpower_payload[POWER_CHANNEL_NUM];
		bool has_per_channel = false;
		for (int idx = 0; idx < POWER_CHANNEL_NUM; ++idx) {
			if (sparam->mgmt_mac_txpower_ch[idx] != 0) {
				has_per_channel = true;
			}
			txpower_payload[idx] = ntohs(sparam->mgmt_mac_txpower_ch[idx]);
		}
		//检查是否已经设置了每个信道的功率值,配置了就不去本地表查询，全是0才去查询
		if (!has_per_channel) {
			txpower_lookup_channels(ntohs(sparam->mgmt_mac_txpower), txpower_payload);
		}

		nla_put(msg, MGMT_ATTR_SET_POWER, sizeof(txpower_payload), txpower_payload);
		POWER_INIT = ntohs(sparam->mgmt_mac_txpower);
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_BANDWIDTH) {
		memset(cmd,0,sizeof(cmd));
		if(sparam->mgmt_mac_bw<=MIN_BW)
		{
			sparam->mgmt_mac_bw=MIN_BW;
		}
		if(sparam->mgmt_mac_bw>=MAX_BW)
		{
			sparam->mgmt_mac_bw=MAX_BW;
		}
		printf("MGMT_SET_BANDWIDTH参数--%d\n", sparam->mgmt_mac_bw);

		sprintf(cmd,
			"sed -i \"s/bw .*/bw %d/g\" /etc/node_xwg",
			sparam->mgmt_mac_bw);
		system(cmd);
		nla_put_u8(msg, MGMT_ATTR_SET_BANDWIDTH, sparam->mgmt_mac_bw);
		BW_INIT = sparam->mgmt_mac_bw;
	}

	if (ntohs(hmsg->mgmt_type) & MGMT_SET_TEST_MODE) {
		memset(cmd,0,sizeof(cmd));
		printf("MGMT_SET_TEST_MODE参数--%d\n", sparam->mgmt_mac_work_mode);
		if (ntohs(sparam->mgmt_mac_work_mode) < 100) {
			sprintf(cmd,
				"sed -i \"s/macmode .*/macmode %d/g\" /etc/node_xwg",
				ntohs(sparam->mgmt_mac_work_mode));
			system(cmd);
		}
		nla_put_u16(msg, MGMT_ATTR_SET_TEST_MODE, ntohs(sparam->mgmt_mac_work_mode));
		MACMODE_INIT = ntohs(sparam->mgmt_mac_work_mode);
	}
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_TEST_MODE_MCS) {
		//		virtparamdata |= VETH_SET_TEST_MODE_MCS;
		nla_put_u16(msg, MGMT_ATTR_SET_TEST_MODE_MCS, ntohs(sparam->mgmt_mac_work_mode));
	}

	if (ntohs(hmsg->mgmt_type) & MGMT_SET_PHY) {
		//		virtparamdata |= VETH_SET_TEST_MODE_MCS;

		memset(cmd,0,sizeof(cmd));
		sparam->mgmt_phy.phy_pre_STS_thresh = ntohs(sparam->mgmt_phy.phy_pre_STS_thresh);
		sparam->mgmt_phy.phy_pre_LTS_thresh = ntohs(sparam->mgmt_phy.phy_pre_LTS_thresh);
		sparam->mgmt_phy.phy_tx_iq0_scale = ntohs(sparam->mgmt_phy.phy_tx_iq0_scale);
		sparam->mgmt_phy.phy_tx_iq1_scale = ntohs(sparam->mgmt_phy.phy_tx_iq1_scale);

	#ifdef Radio_SWARM_S2
		sprintf(cmd,
			"sed -i \"s/phymsg .*/phymsg %d %d %d %d %d %d %d %d %d/g\" /etc/node_xwg",
			sparam->mgmt_phy.rf_agc_framelock_en, sparam->mgmt_phy.phy_cfo_bypass_en,
			sparam->mgmt_phy.phy_pre_STS_thresh, sparam->mgmt_phy.phy_pre_LTS_thresh,
			sparam->mgmt_phy.phy_tx_iq0_scale, sparam->mgmt_phy.phy_tx_iq1_scale,
			sparam->mgmt_phy.phy_msc_length_mode,sparam->mgmt_phy.phy_sfbc_en,
			sparam->mgmt_phy.phy_cdd_num);
	#else 
			sprintf(cmd,
			"sed -i \"s/phymsg .*/phymsg %d %d %d %d %d %d %d %d %d/g\" /etc/node_xwg",
			sparam->mgmt_phy.rf_agc_framelock_en, sparam->mgmt_phy.phy_cfo_bypass_en,
			sparam->mgmt_phy.phy_pre_STS_thresh, sparam->mgmt_phy.phy_pre_LTS_thresh,
			sparam->mgmt_phy.phy_tx_iq0_scale, sparam->mgmt_phy.phy_tx_iq1_scale);
	#endif
		system(cmd);
		nla_put(msg, MGMT_ATTR_SET_PHY, sizeof(Smgmt_phy), &(sparam->mgmt_phy));
	}

	if (ntohs(hmsg->mgmt_type) & MGMT_SET_IQ_CATCH) {
		// set iq
		nla_put(msg, MGMT_SET_IQ_CATCH, sizeof(Smgmt_IQ_Catch),&(sparam->mgmt_mac_iq_catch));
	}

	if (ntohs(hmsg->mgmt_keep) & MGMT_SET_SLOTLEN) {
		printf("MGMT_SET_SLOTLEN:%#x\n", sparam->u8Slotlen);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,
			"sed -i \"s/slotlen .*/slotlen %d/g\" /etc/node_xwg",
			sparam->u8Slotlen);
		system(cmd);
		nla_put_u8(msg, MGMT_ATTR_SET_SLOTLEN, sparam->u8Slotlen);
		
	}
	if (ntohs(hmsg->mgmt_keep) & MGMT_SET_POWER_LEVEL) {
		printf("MGMT_SET_POWER_LEVEL:%#x\n", sparam->mgmt_mac_power_level);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,
			"sed -i \"s/power_level .*/power_level %d/g\" /etc/node_xwg",
			sparam->mgmt_mac_power_level);
		system(cmd);
		nla_put_u8(msg, MGMT_ATTR_SET_POWER_LEVEL, sparam->mgmt_mac_power_level);
	}
	if (ntohs(hmsg->mgmt_keep) & MGMT_SET_POWER_ATTENUATION) {
		printf("MGMT_SET_POWER_ATTENUATION:%#x\n", sparam->mgmt_mac_power_attenuation);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,
			"sed -i \"s/power_attenuation .*/power_attenuation %d/g\" /etc/node_xwg",
			sparam->mgmt_mac_power_attenuation);
		system(cmd);
		nla_put_u8(msg, MGMT_ATTR_SET_POWER_ATTENUATION, sparam->mgmt_mac_power_attenuation);
	}
	if (ntohs(hmsg->mgmt_keep) & MGMT_SET_RX_CHANNEL_MODE) {
		printf("MGMT_SET_RX_CHANNEL_MODE:%#x\n", sparam->mgmt_rx_channel_mode);
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,
			"sed -i \"s/rx_channel_mode .*/rx_channel_mode %d/g\" /etc/node_xwg",
			sparam->mgmt_rx_channel_mode);
		system(cmd);
		nla_put_u8(msg, MGMT_ATTR_SET_RX_CHANNEL_MODE, sparam->mgmt_rx_channel_mode);
		RX_CHANNEL_MODE_INIT = sparam->mgmt_rx_channel_mode;
	}
#ifdef Radio_CEC
	if (ntohs(hmsg->mgmt_type) & MGMT_SET_NCU) {
		sprintf(cmd,
			"sed -i \"s/ncunodeid.*/ncunodeid %d/g\" /etc/node_xwg",
			sparam->mgmt_NCU_node_id);
		system(cmd);
		nla_put_u8(msg, MGMT_ATTR_SET_NCU, sparam->mgmt_NCU_node_id);
		NCU_NODE_ID_INIT = sparam->mgmt_NCU_node_id;
	}

#endif
	if (nla_put_u32(msg, MGMT_ATTR_SET_PARAM, paramdata) < 0) {
		printf("ERROR: 无法添加PARAM属性\n");
		nlmsg_free(msg);
		nl_socket_free(sock);
		return -1;
	}

	// 检查消息发送
	int send_ret = nl_send_auto_complete(sock, msg);
	if (send_ret < 0) {
		printf("ERROR: nl_send_auto_complete失败: %d\n", send_ret);
		nlmsg_free(msg);
		nl_socket_free(sock);
		return -1;
	}

	nlmsg_free(msg);

	system("sync");

	cb = nl_cb_alloc(NL_CB_DEFAULT);//分配一个netlink回调结构体，用于接收内核的回复消息
	if (!cb)
	{
	    printf("ERROR: nl_cb_alloc失败\n");
		nl_socket_free(sock);
		return -1;	
	}

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, mgmt_netlink_param_callback, &opts);
	nl_cb_err(cb, NL_CB_CUSTOM, print_error, NULL);//设置错误回调函数

	nl_recvmsgs(sock, cb);

	nl_cb_put(cb);//释放回调结构体
// 	ret = 0;
// err_free_sock:
// 	nl_socket_free(sock);
	return 0;
}

char mgmt_netlink_set_param_wg(char* buffer, int buflen, const char* header,int type)
{
	printf("调用mgmt_netlink_set_param_wg函数\n");
	printf("buffer长度buflen为:%d\n", buflen);

    if (sim_get_mode()) {
        sim_set_param_bulk(buffer, buflen, type);
        return 0;
    }

	Smgmt_param* sparam;
	Smgmt_param sparam_for_set;
	int paramdata = 0;

	struct nl_sock* sock;
	struct nl_msg* msg;
	struct nl_cb* cb;//回调函数结构体指针
	int family;
	uint8_t cmd[200];
	char ret = -1;
	struct print_opts opts = {
		.read_opt = 0,
		.nl_cmd = MGMT_CMD_SET_PARAM,
		.remaining_header = NULL,
		.static_header = header,
	};//回调函数参数结构体初始化

	memset(&sparam_for_set, 0, sizeof(sparam_for_set));
	if(type == MGMT_SET_PARAM)
		{
			memcpy((uint8_t*)&sparam_for_set, buffer , buflen);
		}
	else if(type == MGMT_MULTIPOINT_SET)
		{
			//Smgmt_param结构体中的ip和id跳过。因为多点配置的ip在Smgmt_header中传入了。
			memcpy((uint8_t*)&sparam_for_set + sizeof(uint32_t) + sizeof(uint16_t), buffer, buflen);
		}
	sparam = &sparam_for_set;
	
	//测试打印---------------------------------------------------------------
	printf("数据内容：\n");
	printf("sparam->mgmt_ip 08x:%08x \n", sparam->mgmt_ip);
	printf("sparam->mgmt_id 04x:%04x \n", sparam->mgmt_id);
	printf("sparam->mgmt_route_interval 04x:%04x \n", sparam->mgmt_route_interval);
	printf("sparam->mgmt_route_ttl 04x:%04x\n ", sparam->mgmt_route_ttl);
	printf("param->mgmt_virt_queue_num 04x:%04x \n", sparam->mgmt_virt_queue_num);
	printf("sparam->mgmt_virt_queue_length 04x:%04x\n ", sparam->mgmt_virt_queue_length);
	printf("sparam->mgmt_virt_qos_stateg 04x:%04x \n", sparam->mgmt_virt_qos_stategy);
	printf("sparam->mgmt_virt_unicast_mcs 02x:%02x \n", sparam->mgmt_virt_unicast_mcs);
	printf("sparam->mgmt_virt_multicast_mcs 02x:%02x \n", sparam->mgmt_virt_multicast_mcs);
	printf("sparam->mgmt_mac_bw 02x:%02x \n", sparam->mgmt_mac_bw);
	printf("sparam->reserved 02x:%02x \n", sparam->reserved);
	printf("sparam->mgmt_mac_freq 08x:%08x \n", sparam->mgmt_mac_freq);
	printf("sparam->mgmt_mac_txpower 04x:%04x \n", sparam->mgmt_mac_txpower);
	printf("sparam->mgmt_mac_work_mode 04x:%04x \n", sparam->mgmt_mac_work_mode);
	printf("\n");
	//测试打印---------------------------------------------------------------

	sock = nl_socket_alloc();//分配一个netlink套接字
	if (!sock)
		return ret;
	
	genl_connect(sock);//连接到generic netlink子系统

	family = genl_ctrl_resolve(sock, MGMT_NL_NAME);
	if (family < 0) {
		printf("family error\n");
		nl_socket_free(sock);
		return ret;
	}

	msg = nlmsg_alloc();//分配一个netlink消息体空间
	if (!msg) {
		nl_socket_free(sock);
		return ret;
	}

	if(!genlmsg_put(msg,NL_AUTO_PID,NL_AUTO_SEQ,family,0,0,MGMT_CMD_SET_PARAM,1))
	{
		printf("genlmsg_put error\n");
		nlmsg_free(msg);
		nl_socket_free(sock);
		return -1;
	}

	nla_put_u16(msg, MGMT_ATTR_SET_INTERVAL, ntohs(sparam->mgmt_route_interval));
	nla_put_u16(msg, MGMT_ATTR_SET_TTL, ntohs(sparam->mgmt_route_ttl));
	nla_put_u16(msg, MGMT_ATTR_SET_QUEUE_NUM, ntohs(sparam->mgmt_virt_queue_num));
	nla_put_u16(msg, MGMT_ATTR_SET_QUEUE_LENGTH, ntohs(sparam->mgmt_virt_queue_length));
	nla_put_u16(msg, MGMT_ATTR_SET_QOS_STATEGY, ntohs(sparam->mgmt_virt_qos_stategy));
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,
		"sed -i \"s/mcs .*/mcs %d/g\" /etc/node_xwg",
		sparam->mgmt_virt_unicast_mcs);
	system(cmd);
	nla_put_u8(msg, MGMT_ATTR_SET_UNICAST_MCS, sparam->mgmt_virt_unicast_mcs);
	MCS_INIT = sparam->mgmt_virt_unicast_mcs;
	nla_put_u8(msg, MGMT_ATTR_SET_MULTICAST_MCS, sparam->mgmt_virt_multicast_mcs);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,
		"sed -i \"s/channel .*/channel %d/g\" /etc/node_xwg",
		ntohl(sparam->mgmt_mac_freq));
	system(cmd);
	nla_put_u32(msg, MGMT_ATTR_SET_FREQUENCY, ntohl(sparam->mgmt_mac_freq));
	FREQ_INIT = ntohl(sparam->mgmt_mac_freq);

	if (ntohs(sparam->mgmt_mac_txpower) < MIN_TXPOWER)
		sparam->mgmt_mac_txpower = htons(MIN_TXPOWER);
	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,
		"sed -i \"s/power .*/power %d/g\" /etc/node_xwg",
		ntohs(sparam->mgmt_mac_txpower));
	system(cmd);
	uint16_t txpower_payload_wg[POWER_CHANNEL_NUM];
	bool has_per_channel_wg = false;
	for (int idx = 0; idx < POWER_CHANNEL_NUM; ++idx) {
		if (sparam->mgmt_mac_txpower_ch[idx] != 0) {
			has_per_channel_wg = true;
		}
		txpower_payload_wg[idx] = ntohs(sparam->mgmt_mac_txpower_ch[idx]);
	}
	if (!has_per_channel_wg) {
		txpower_lookup_channels(ntohs(sparam->mgmt_mac_txpower), txpower_payload_wg);
	}
	nla_put(msg, MGMT_ATTR_SET_POWER, sizeof(txpower_payload_wg), txpower_payload_wg);
	POWER_INIT = ntohs(sparam->mgmt_mac_txpower);

	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,
		"sed -i \"s/bw .*/bw %d/g\" /etc/node_xwg",
		sparam->mgmt_mac_bw);
	system(cmd);
	nla_put_u8(msg, MGMT_ATTR_SET_BANDWIDTH, sparam->mgmt_mac_bw);
	BW_INIT = sparam->mgmt_mac_bw;

	memset(cmd,0,sizeof(cmd));
	if (ntohs(sparam->mgmt_mac_work_mode) < 100) {
		sprintf(cmd,
			"sed -i \"s/macmode .*/macmode %d/g\" /etc/node_xwg",
			ntohs(sparam->mgmt_mac_work_mode));
		system(cmd);
	}

	nla_put_u16(msg, MGMT_ATTR_SET_TEST_MODE, ntohs(sparam->mgmt_mac_work_mode));
	MACMODE_INIT = ntohs(sparam->mgmt_mac_work_mode);
	nla_put_u16(msg, MGMT_ATTR_SET_TEST_MODE_MCS, ntohs(sparam->mgmt_mac_work_mode));

	memset(cmd,0,sizeof(cmd));
	longitude = htond(sparam->mgmt_longitude);
			sprintf(cmd,
			"sed -i \"s/longitude .*/longitude %.8f/g\" /etc/node_xwg",
			longitude);
		system(cmd);

		memset(cmd,0,sizeof(cmd));
	latitude = htond(sparam->mgmt_latitude);
				sprintf(cmd,
			"sed -i \"s/latitude .*/latitude %.8f/g\" /etc/node_xwg",
			latitude);
		system(cmd);

	memset(cmd,0,sizeof(cmd));
	sprintf(cmd,
		"sed -i \"s/rx_channel_mode .*/rx_channel_mode %d/g\" /etc/node_xwg",
		sparam->mgmt_rx_channel_mode);
	system(cmd);

	nla_put_u8(msg, MGMT_ATTR_SET_RX_CHANNEL_MODE, sparam->mgmt_rx_channel_mode);
	RX_CHANNEL_MODE_INIT = sparam->mgmt_rx_channel_mode;

	nla_put_u32(msg, MGMT_ATTR_SET_PARAM, paramdata);

	nl_send_auto_complete(sock, msg);

	nlmsg_free(msg);

	system("sync");
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb)
		goto err_free_sock;

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, mgmt_netlink_param_callback, &opts);
	nl_cb_err(cb, NL_CB_CUSTOM, print_error, NULL);

	nl_recvmsgs(sock, cb);
	ret = 0;
err_free_sock:
	nl_socket_free(sock);
	return ret;

}

static int mgmt_netlink_info_callback(struct nl_msg* msg, void* arg)
{
	struct nlattr* attrs[MGMT_ATTR_MAX + 1];
	struct nlmsghdr* nlh = nlmsg_hdr(msg);
	struct print_opts* opts = arg;
	const uint8_t* primary_mac;
	struct genlmsghdr* ghdr;
	const uint8_t* mesh_mac;
	const char* primary_if;
	const char* mesh_name;
	const char* version;
	char* extra_info = NULL;
	char* jgk_node;
	char* croutet;
	char* veth_jgk_data;
	uint8_t ttvn = 0;
	uint16_t bla_group_id = 0;
	const char* algo_name;
	const char* extra_header;
	uint8_t i = 0;
	int neigh_num = 0;

	struct batadv_jgk_node* bat_jgk_node;
	struct routetable* routet;

	struct mgmt_send* smsg = (struct mgmt_send*)(opts->remaining_header);
	jgk_report_infor* jgk_information_data;
	//检验接收到的消息是否合法 
	//genlmsg_valid_hdr函数会检查消息头部的长度和类型等信息，确保消息格式正确，避免处理无效或恶意的消息。
	if (!genlmsg_valid_hdr(nlh, 0)) {
		fputs("Received invalid data from kernel.\n", stderr);
		exit(1);
	}

	ghdr = nlmsg_data(nlh);//获取消息头部数据，准备解析属性
	//解析属性，nla_parse函数会根据定义的属性政策（mgmt_netlink_policy）将消息中的属性解析到attrs数组中，方便后续访问。
	//genlmsg_attrdata函数用于获取属性数据的起始位置，genlmsg_len函数用于获取属性数据的长度。解析完成后，attrs数组中对应索引的位置将指向相应的属性数据。
	if (nla_parse(attrs, MGMT_ATTR_MAX, genlmsg_attrdata(ghdr, 0),
		genlmsg_len(ghdr), mgmt_netlink_policy)) {
		fputs("Received invalid data from kernel.\n", stderr);
		exit(1);
	}
	switch (opts->nl_cmd) {
		case MGMT_CMD_GET_ROUTE_INFO:{
			smsg = opts->remaining_header;//提前申请好的内存，用来接数据
			jgk_node = nla_data(attrs[MGMT_ATTR_PKTNUMB_ROUTE]);
			bat_jgk_node = (struct batadv_jgk_node*)jgk_node;
			//printf("mgmt_ctrl bat_jgk_node nodeid: %d, s_ogm: %d, r_ogm: %d, f_ogm: %d, s_bcast: %d, r_bcast: %d, f_bcast: %d\n", bat_jgk_node->nodeid,
			//bat_jgk_node->s_ogm, bat_jgk_node->r_ogm, bat_jgk_node->f_ogm, bat_jgk_node->s_bcast, bat_jgk_node->r_bcast, bat_jgk_node->f_bcast);
			croutet = nla_data(attrs[MGMT_ATTR_TABLE_ROUTE]);
			routet = (struct routetable*)croutet;
			memcpy(smsg, croutet, sizeof(struct routetable));
			break;
		}
		case MGMT_CMD_GET_VETH_INFO:
		{
			veth_jgk_data = nla_data(attrs[MGMT_ATTR_INFO_VETH]);
			jgk_information_data = (jgk_report_infor*)veth_jgk_data;
			smsg->freq = FREQ_INIT;
			smsg->bw = BW_INIT;
			smsg->txpower = POWER_INIT;
			memcpy(smsg->agent_version,jgk_information_data->agent_version,sizeof(int));
			memcpy(smsg->veth_version,jgk_information_data->veth_version,sizeof(int));
			memcpy(smsg->ctrl_version,jgk_information_data->ctrl_version,sizeof(int));
			//amp_infomation 功放状态数据
			memcpy(&smsg->amp_infomation,&jgk_information_data->amp_infomation,sizeof(DEVICE_SC_STATUS_REPORT));

			for (i = 1; i < MCS_NUM; i++) {
				smsg->rx += jgk_information_data->enqueue_bytes[i];
				smsg->tx += jgk_information_data->outqueue_bytes[i];
			}
			/*邻居节点ID（node_id）、使用的调制编码策略（mcs）、接收信号强度指示（rssi）、
			*信噪比（snr）、底噪（noise）、可用信道时隙相关状态（ucds）、时间抖动（time_jitter）
			*以及收发良好/损坏的数据包统计（good 和 bad）。最后，有效邻居的数量 neigh_num*/
		for (i = 1; i < NET_SIZE; i++) {
//						printf("i %d %d\n",i,jgk_information_data->mac_information_part2.mcs[i]);
			if (jgk_information_data->mac_information_part2.mcs[i] != NO_MCS) {
				smsg->msg[neigh_num].node_id = i;
				//				smsg->msg[neigh_num].enqueue_bytes = jgk_information_data->enqueue_bytes[4]/1000;
				//				smsg->msg[neigh_num].outqueue_bytes = jgk_information_data->outqueue_bytes[4]/1000;
				//smsg->msg[neigh_num].mcs = mgmt_q_2_mcs(smooth_mcs(jgk_information_data->mac_information_part2.mcs[i]));//mgmt_q_2_mcs(smooth_mcs(jgk_information_data->mac_information_part2.mcs[i]))
				smsg->msg[neigh_num].mcs=jgk_information_data->mac_information_part2.mcs[i];
				smsg->msg[neigh_num].rssi = jgk_information_data->mac_information_part2.rssi[i];//mgmt_q_2_rssi(smmoth_rssi(jgk_information_data->mac_information_part2.rssi[i]))
				smsg->msg[neigh_num].snr = jgk_information_data->mac_information_part2.snr[i];
				smsg->msg[neigh_num].noise = jgk_information_data->mac_information_part2.noise[i];
				smsg->msg[neigh_num].ucds = jgk_information_data->mac_information_part2.ucds[i];
				smsg->msg[neigh_num].time_jitter = jgk_information_data->mac_information_part2.time_jitter[i];
				smsg->msg[neigh_num].good = jgk_information_data->mac_information_part2.good[i];
				smsg->msg[neigh_num].bad = jgk_information_data->mac_information_part2.bad[i];
//				printf("neigh i %d node %d mcs %d rssi %d snr %d noise %d good %d bad %d \n", i, smsg->msg[neigh_num].node_id, smsg->msg[neigh_num].mcs,
//					smsg->msg[neigh_num].rssi, smsg->msg[neigh_num].snr,smsg->msg[neigh_num].noise,smsg->msg[neigh_num].good,smsg->msg[neigh_num].bad);
				neigh_num++;
			}
		}
			smsg->neigh_num = neigh_num;
			memcpy((void *)&smsg->mac_information_part1,(void *)&jgk_information_data->mac_information_part1,sizeof(ob_state_part1));

			break;
		}

	}
	return NL_STOP;
}


char* mgmt_netlink_get_info(int ifindex, uint8_t nl_cmd, const char* header, char* remaining)
{
    if (sim_get_mode()) {
        if (nl_cmd == MGMT_CMD_GET_ROUTE_INFO) {
             sim_get_route_info((struct routetable*)remaining);
             return remaining;
        } else if (nl_cmd == MGMT_CMD_GET_VETH_INFO) {
             sim_get_veth_info((struct mgmt_send*)remaining);
             return remaining;
        }
    }

	struct nl_sock* sock;//netlink套接字
	struct nl_msg* msg;//netlink消息体
	struct nl_cb* cb;//netlink回调函数
	int family;
	struct print_opts opts = {
		.read_opt = 0,
		.nl_cmd = nl_cmd,
		.remaining_header = remaining,
		.static_header = header,
	};//print_opts结构体用于存储回调函数需要的参数，包括读取选项、netlink命令、剩余的消息头部和静态消息头部。

	sock = nl_socket_alloc();
	if(!sock)
		return NULL;
	
	if (genl_connect(sock) < 0) {
		fprintf(stderr, "[MGMT ERROR] genl_connect failed\n");
		nl_socket_free(sock);
		return NULL;
	}
	family = genl_ctrl_resolve(sock, MGMT_NL_NAME);
	if (family < 0) {
		printf("family error\n");
		nl_socket_free(sock);
		return NULL;
	}

	msg = nlmsg_alloc();
	if (!msg) {
		nl_socket_free(sock);
		return NULL;
	}
	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, 0, nl_cmd, 1);
	
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb)
		goto err_free_sock;
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, mgmt_netlink_info_callback, &opts);
	nl_cb_err(cb, NL_CB_CUSTOM, print_error, NULL);

	nla_put_u8(msg, MGMT_ATTR_NODEID, SELFID);
	//printf("mgmt_netlink_get_info id %d\n",SELFID);

	if (nl_send_auto_complete(sock, msg) < 0) {
		fprintf(stderr, "[MGMT ERROR] nl_send_auto_complete failed, cmd=%u\n", nl_cmd);
		nlmsg_free(msg);
		nl_cb_put(cb);
		nl_socket_free(sock);
		return NULL;
	}

	nlmsg_free(msg);

	if (nl_recvmsgs(sock, cb) < 0) {
		fprintf(stderr, "[MGMT ERROR] nl_recvmsgs failed, cmd=%u\n", nl_cmd);
		nl_cb_put(cb);
		nl_socket_free(sock);
		return NULL;
	}
	nl_cb_put(cb);
	
err_free_sock:
	nl_socket_free(sock);

	return opts.remaining_header;	

}