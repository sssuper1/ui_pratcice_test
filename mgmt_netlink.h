#ifndef __MGMT_NETLINK_H
#define __MGMT_NETLINK_H

#include <stdint.h>


char mgmt_netlink_set_param(char* buffer, int buflen, const char* header);
char mgmt_netlink_set_param_wg(char* buffer, int buflen, const char* header,int type);
char* mgmt_netlink_get_info(int ifindex, uint8_t nl_cmd, const char* header, char* remaining);  

#endif