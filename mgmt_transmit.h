#ifndef __MGMTM_TRANSMIT_H
#define __MGMTM_TRANSMIT_H
#include "wg_config.h"
#include "mgmt_types.h"

extern uint8_t MCS_INIT;
extern uint8_t NET_WORKMOD_INIT;
extern uint32_t FREQ_INIT;
extern uint16_t POWER_INIT;
extern uint8_t BW_INIT;
extern uint8_t RX_CHANNEL_MODE_INIT;
extern uint16_t MACMODE_INIT;
extern uint32_t HOP_FREQ_TB_INIT[HOP_FREQ_NUM];

void read_node_xwg_file(const char * filename,Node_Xwg_Pairs* xwg_info,int num_pairs);
const char *get_value(Node_Xwg_Pairs* pairs,const char *key);
double get_double_value(Node_Xwg_Pairs* pairs,const char *key);
int get_int_value(Node_Xwg_Pairs* pairs,const char* key);
#endif // !__MGMTM_TRANSMIT_H
