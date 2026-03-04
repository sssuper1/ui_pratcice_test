#include "mgmt_netlink.h"
#include "mgmt_types.h"
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include "mgmt_transmit.h"
#include <stdbool.h>


#define MIN_TXPOWER 0
#define MAX_TXPOWER 70
#define MIN_BW		0
#define MAX_BW		4

#if 0
static int mgmt_netlink_param_callback(struct nl_msg* msg, void* arg)
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
	uint8_t ttvn = 0;
	uint16_t bla_group_id = 0;
	const char* algo_name;
	const char* extra_header;
	int ret;
	int value;

	if (!genlmsg_valid_hdr(nlh, 0)) {
		fputs("Received invalid data from kernel.\n", stderr);
		exit(1);
	}

	ghdr = nlmsg_data(nlh);

	if (ghdr->cmd != MGMT_CMD_SET_PARAM)
		return NL_OK;

	if (nla_parse(attrs, MGMT_ATTR_MAX, genlmsg_attrdata(ghdr, 0),
		genlmsg_len(ghdr), mgmt_netlink_policy)) {
		fputs("Received invalid data from kernel.\n", stderr);
		exit(1);
	}

	//set ok
//	printf("mgmt_netlink_param_set ok\n");

	return NL_STOP;
}

char mgmt_netlink_set_param(char* buffer, int buflen, const char* header)
{
	//测试打印------------------------------
	printf("调用mgmt_netlink_set_param函数\n");
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


	sock = nl_socket_alloc();
	if (!sock)
		return ret;

	genl_connect(sock);

	family = genl_ctrl_resolve(sock, MGMT_NL_NAME);
	if (family < 0) {
		printf("family error\n");
		nl_socket_free(sock);
		return ret;
	}

	msg = nlmsg_alloc();
	if (!msg) {
		nl_socket_free(sock);
		return ret;
	}

	genlmsg_put(msg, NL_AUTO_PID, NL_AUTO_SEQ, family, 0, 0,
		MGMT_CMD_SET_PARAM, 1);

	paramdata = ntohs(hmsg->mgmt_type);
	//printf("Smgmt_header paramdata = %#x\n",paramdata);
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

	
	//	printf("mgmt_type %d\n",ntohs(hmsg->mgmt_type));

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
#ifdef Radio_SWARM_WNW
        if(sparam->mgmt_virt_unicast_mcs == 7){
           sparam->mgmt_virt_unicast_mcs = 6;
		}
#endif
		nla_put_u8(msg, MGMT_ATTR_SET_UNICAST_MCS, sparam->mgmt_virt_unicast_mcs);
		MCS_INIT = sparam->mgmt_virt_unicast_mcs;
	}

	if (ntohs(hmsg->mgmt_type) & MGMT_SET_MULTICAST_MCS) {
#ifdef Radio_SWARM_WNW
		 if(sparam->mgmt_virt_multicast_mcs == 7){
           sparam->mgmt_virt_multicast_mcs = 6;
		}
#endif
		printf("设置MGMT_SET_MULTICAST_MCS参数%02x\n", sparam->mgmt_virt_multicast_mcs);

		//		virtparamdata |= VETH_SET_MULTICAST_MCS;
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
//		for(i=0;i<32;i++)
//		{
//			HOP_FREQ_TB_INIT[i] = 1400 + i*50;
//		}
		
		if(sparam->mgmt_net_work_mode.NET_work_mode == HOP_FREQ_MODE) //跳频模式
		{
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
			}
		}
		else if(sparam->mgmt_net_work_mode.NET_work_mode == FIX_FREQ_MODE)
		{
			if((ntohs(hmsg->mgmt_type) & MGMT_SET_FREQUENCY) == 0)
			{
				nla_put_u32(msg, MGMT_ATTR_SET_FREQUENCY, FREQ_INIT);
				paramdata |= MGMT_SET_FREQUENCY;
			}
		}

		nla_put(msg, MGMT_ATTR_SET_WORKMODE, sizeof(Smgmt_net_work_mode), (void *)&(sparam->mgmt_net_work_mode));
	}

	if ((ntohs(hmsg->mgmt_type) & MGMT_SET_FREQUENCY) && (NET_WORKMOD_INIT == FIX_FREQ_MODE)) {
		memset(cmd,0,sizeof(cmd));
		printf("设置MGMT_SET_FREQUENCY参数--%d\n", ntohl(sparam->mgmt_mac_freq));



		//		virtparamdata |= VETH_SET_FREQUENCY;
		//		printf("mgmt_freq %d\n",ntohl(sparam->mgmt_mac_freq));
		sprintf(cmd,
			"sed -i \"s/channel .*/channel %d/g\" /etc/node_xwg",
			ntohl(sparam->mgmt_mac_freq));
		system(cmd);
		nla_put_u32(msg, MGMT_ATTR_SET_FREQUENCY, ntohl(sparam->mgmt_mac_freq));
		FREQ_INIT = ntohl(sparam->mgmt_mac_freq);
	}

	if (ntohs(hmsg->mgmt_type) & MGMT_SET_POWER) {
		//		virtparamdata |= VETH_SET_POWER;
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

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb)
	{
	    printf("ERROR: nl_cb_alloc失败\n");
		nl_socket_free(sock);
		return -1;	
	}
		// goto err_free_sock;

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, mgmt_netlink_param_callback, &opts);
	nl_cb_err(cb, NL_CB_CUSTOM, print_error, NULL);

	nl_recvmsgs(sock, cb);

	nl_cb_put(cb);
	return 0;
}
#endif