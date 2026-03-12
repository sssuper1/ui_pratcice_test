#ifndef SIM_HEARTBEAT_H
#define SIM_HEARTBEAT_H

#include "mgmt_types.h"
#include "mgmt_transmit.h"
#include "mgmt_netlink.h"

// Simulation control structure
typedef struct {
    int enabled; // 0: off, 1: on
    int update_interval_ms; // Heartbeat interval
} SimConfig;

// Initialize simulation
void sim_init(void);

// Get simulated route info
void sim_get_route_info(struct routetable *route_msg);

// Get simulated veth info (device status)
void sim_get_veth_info(struct mgmt_send *self_msg);

// Update simulation parameters based on command
void sim_set_param(const char *buffer, int buflen, int type);

// Bulk update simulation parameters
void sim_set_param_bulk(const char *buffer, int buflen, int type);

// Toggle simulation mode
void sim_set_mode(int enable);
int sim_get_mode(void);

#endif // SIM_HEARTBEAT_H
