#ifndef __MGMTM_TRANSMIT_H
#define __MGMTM_TRANSMIT_H
#include <netinet/in.h>
#include "wg_config.h"
#include "mgmt_types.h"


extern uint8_t SELFIP_s[4];
extern uint8_t MCS_INIT;
extern uint8_t NET_WORKMOD_INIT;
extern uint32_t FREQ_INIT;
extern uint16_t POWER_INIT;
extern uint8_t BW_INIT;
extern uint8_t RX_CHANNEL_MODE_INIT;
extern uint16_t MACMODE_INIT;
extern uint8_t DEVICETYPE_INIT;
extern uint8_t POWER_LEVEL_INIT;
extern uint8_t POWER_ATTENUATION_INIT;
extern uint8_t g_u8SLOTLEN;
extern uint8_t SELFID;
extern double longitude;
extern double latitude;
extern uint32_t HOP_FREQ_TB_INIT[HOP_FREQ_NUM];

void read_node_xwg_file(const char * filename,Node_Xwg_Pairs* xwg_info,int num_pairs);
const char *get_value(Node_Xwg_Pairs* pairs,const char *key);
double get_double_value(Node_Xwg_Pairs* pairs,const char *key);
int get_int_value(Node_Xwg_Pairs* pairs,const char* key);
void mgmt_mysql_init(void);
void mgmt_recv_web(void);
void mgmt_recv_msg(void);
void mgmt_get_msg(void);
uint8_t find_minMcs(uint8_t *arr,int size);
uint32_t find_max(uint32_t *arr,int size);
double htond(double val);

void updateData_init(void);
void txpower_lookup_channels(uint16_t single, uint16_t channel_power[POWER_CHANNEL_NUM]);
void txpower_lookup_channels_be(uint16_t single, uint16_t channel_power[POWER_CHANNEL_NUM]);
void send_topo_msg(struct sockaddr_in from, struct mgmt_send self_msg);
void send_topo_request(void);
int neighid_isexit(int *buf,int size,int target);
void update_time_slot_table(ob_state_part1 * part1_data, uint8_t * time_slot_tb_infor);
void reset_systeminfo_table(int j);
void mgmt_status_report(struct sockaddr_in from);
void SendNodeMsg(void* data, int datalen);
void mgmt_recv_from_qkwg(void);
void thread_report_test(void);

#endif // !__MGMTM_TRANSMIT_H
