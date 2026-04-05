#include "sim_heartbeat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "mgmt_transmit.h"

static SimConfig g_sim_config = {1, 1000}; // Default enabled
static struct mgmt_send g_sim_status;
static struct routetable g_sim_route;
static int g_sim_initialized = 0;
static time_t g_sim_start_ts = 0;
static int g_last_neigh_num = -1;

#define SIM_DYNAMIC_NEIGH_MAX 3
#define SIM_NEIGHBOR_STEP_SEC 10

// Helper to generate random float
float get_random_float(float min, float max) {
    return min + (float)rand() / (float)(RAND_MAX / (max - min));
}

// Helper to generate random int
int get_random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

static int sim_get_target_neigh_num(void) {
    time_t now = time(NULL);
    int phase;

    if (g_sim_start_ts == 0) {
        g_sim_start_ts = now;
    }

    phase = (int)((now - g_sim_start_ts) / SIM_NEIGHBOR_STEP_SEC) % 5;
    if (phase >= 0 && phase <= SIM_DYNAMIC_NEIGH_MAX) {
        return phase;
    }
    return 0;
}

void sim_init(void) {
    srand(time(NULL));
    memset(&g_sim_status, 0, sizeof(g_sim_status));
    memset(&g_sim_route, 0, sizeof(g_sim_route));
    
    // Initialize with some default values
    g_sim_status.node_id = 1; // Default ID
    g_sim_status.bw = 20;
    g_sim_status.txpower = 30;
    g_sim_status.freq = 5800000;
    
    // Initialize neighbor slots; online count is driven by timer state machine.
    g_sim_status.neigh_num = 0;
    for (int i = 0; i < SIM_DYNAMIC_NEIGH_MAX; i++) {
        g_sim_status.msg[i].node_id = i + 2; // Neighbors start from ID 2
        g_sim_status.msg[i].rssi = 60;
        g_sim_status.msg[i].snr = 25;
        g_sim_status.msg[i].mcs = 4;
        g_sim_status.msg[i].good = 100;
        g_sim_status.msg[i].bad = 0;
        g_sim_status.msg[i].ucds = 10;
        g_sim_status.msg[i].time_jitter = 5;
        g_sim_status.msg[i].noise = 90;
    }
    g_sim_start_ts = time(NULL);
    g_last_neigh_num = -1;
    g_sim_initialized = 1;
    
    printf("[SIM] Simulation initialized.\n");
}

void sim_update_status(void) {
    int target_neigh_num = sim_get_target_neigh_num();

    // Simulate dynamic changes
    g_sim_status.tx += get_random_int(100, 1000);
    g_sim_status.rx += get_random_int(100, 1000);
    g_sim_status.neigh_num = target_neigh_num;

    if (g_last_neigh_num != target_neigh_num) {
        printf("[SIM] Dynamic neighbor state switched: %d online\n", target_neigh_num);
        g_last_neigh_num = target_neigh_num;
    }
    
    for (int i = 0; i < g_sim_status.neigh_num; i++) {
        // Vary RSSI slightly
        int rssi_change = get_random_int(-2, 2);
        g_sim_status.msg[i].rssi += rssi_change;
        if (g_sim_status.msg[i].rssi < 10) g_sim_status.msg[i].rssi = 10;
        if (g_sim_status.msg[i].rssi > 90) g_sim_status.msg[i].rssi = 90;
        
        // Vary SNR
        g_sim_status.msg[i].snr = g_sim_status.msg[i].rssi / 2 + get_random_int(-2, 2);
        
        // Update good/bad packets
        g_sim_status.msg[i].good += get_random_int(0, 10);
        if (get_random_int(0, 100) > 95) {
             g_sim_status.msg[i].bad += 1;
        }
        
        // Update time jitter
        g_sim_status.msg[i].time_jitter = get_random_int(1, 20);
    }

    // Clear offline slots to avoid stale values leaking into debug displays.
    for (int i = g_sim_status.neigh_num; i < SIM_DYNAMIC_NEIGH_MAX; i++) {
        g_sim_status.msg[i].rssi = 0;
        g_sim_status.msg[i].snr = 0;
        g_sim_status.msg[i].mcs = 0;
        g_sim_status.msg[i].good = 0;
        g_sim_status.msg[i].bad = 0;
        g_sim_status.msg[i].ucds = 0;
        g_sim_status.msg[i].time_jitter = 0;
        g_sim_status.msg[i].noise = 0;
    }
}

void sim_get_route_info(struct routetable *route_msg) {
    if (!g_sim_initialized) {
        sim_init();
    }
    // For now, just return static route info or implement dynamic if needed
    // In a real scenario, we would update routing tables based on neighbor connectivity
    memcpy(route_msg, &g_sim_route, sizeof(struct routetable));
}

void sim_get_veth_info(struct mgmt_send *self_msg) {
    if (!g_sim_initialized) {
        sim_init();
    }
    sim_update_status();
    memcpy(self_msg, &g_sim_status, sizeof(struct mgmt_send));
}

void sim_set_param(const char *buffer, int buflen, int type) {
    // Parse the buffer similar to mgmt_netlink_set_param
    // This allows web commands to update simulation state
    
    Smgmt_header* hmsg = (Smgmt_header*)buffer;
    Smgmt_set_param* sparam = (Smgmt_set_param*)hmsg->mgmt_data;
    
    printf("[SIM] Received parameter update request.\n");

    if (ntohs(hmsg->mgmt_type) & MGMT_SET_FREQUENCY) {
        g_sim_status.freq = ntohl(sparam->mgmt_mac_freq);
        FREQ_INIT = g_sim_status.freq;
        printf("[SIM] Frequency set to %d\n", g_sim_status.freq);
        
        char cmd[256];
        sprintf(cmd, "sed -i \"s/channel .*/channel %d/g\" /etc/node_xwg", g_sim_status.freq);
        system(cmd);
    }
    
    if (ntohs(hmsg->mgmt_type) & MGMT_SET_POWER) {
        g_sim_status.txpower = ntohs(sparam->mgmt_mac_txpower);
        POWER_INIT = g_sim_status.txpower;
        printf("[SIM] Tx Power set to %d\n", g_sim_status.txpower);
        
        char cmd[256];
        sprintf(cmd, "sed -i \"s/power .*/power %d/g\" /etc/node_xwg", g_sim_status.txpower);
        system(cmd);
    }
    
    if (ntohs(hmsg->mgmt_type) & MGMT_SET_BANDWIDTH) {
        g_sim_status.bw = sparam->mgmt_mac_bw;
        BW_INIT = g_sim_status.bw;
        printf("[SIM] Bandwidth set to %d\n", g_sim_status.bw);
        
        char cmd[256];
        sprintf(cmd, "sed -i \"s/bw .*/bw %d/g\" /etc/node_xwg", g_sim_status.bw);
        system(cmd);
    }

    if (ntohs(hmsg->mgmt_type) & MGMT_SET_UNICAST_MCS) {
        MCS_INIT = sparam->mgmt_virt_unicast_mcs;
        printf("[SIM] Unicast MCS set to %d\n", sparam->mgmt_virt_unicast_mcs);

        char cmd[256];
        sprintf(cmd, "sed -i \"s/mcs .*/mcs %d/g\" /etc/node_xwg", sparam->mgmt_virt_unicast_mcs);
        system(cmd);
    }

    if (ntohs(hmsg->mgmt_type) & MGMT_SET_WORKMODE) {
        Smgmt_net_work_mode *wm = &sparam->mgmt_net_work_mode;
        NET_WORKMOD_INIT = wm->NET_work_mode;
        printf("[SIM] Workmode set to %d (fh_len=%d)\n", wm->NET_work_mode, wm->fh_len);

        char cmd[256];
        sprintf(cmd, "sed -i \"s/workmode .*/workmode %d/g\" /etc/node_xwg", wm->NET_work_mode);
        system(cmd);

        if (wm->NET_work_mode == 4) {
            sprintf(cmd,
                "sed -i \"s/select_freq1 .*/select_freq1 %u/; s/select_freq2 .*/select_freq2 %u/; s/select_freq3 .*/select_freq3 %u/; s/select_freq4 .*/select_freq4 %u/\" /etc/node_xwg",
                wm->hop_freq_tb[0], wm->hop_freq_tb[1], wm->hop_freq_tb[2], wm->hop_freq_tb[3]);
            system(cmd);
            printf("[SIM] Adaptive freqs: %u %u %u %u\n",
                wm->hop_freq_tb[0], wm->hop_freq_tb[1], wm->hop_freq_tb[2], wm->hop_freq_tb[3]);
        }
    }
    
    if (ntohs(hmsg->mgmt_type) & MGMT_SET_TEST_MODE) {
        // g_sim_status.work_mode = ntohs(sparam->mgmt_mac_work_mode);
        MACMODE_INIT = ntohs(sparam->mgmt_mac_work_mode);
        printf("[SIM] Work mode set to %d\n", ntohs(sparam->mgmt_mac_work_mode));
        
        char cmd[256];
        sprintf(cmd, "sed -i \"s/macmode .*/macmode %d/g\" /etc/node_xwg", ntohs(sparam->mgmt_mac_work_mode));
        system(cmd);
    }
    
    if (ntohs(hmsg->mgmt_keep) & MGMT_SET_SLOTLEN) {
        g_u8SLOTLEN = sparam->u8Slotlen;
        printf("[SIM] Slot length set to %d\n", sparam->u8Slotlen);
        char cmd[256];
        sprintf(cmd, "sed -i \"s/slotlen .*/slotlen %d/g\" /etc/node_xwg", sparam->u8Slotlen);
        system(cmd);
    }
    
    if (ntohs(hmsg->mgmt_keep) & MGMT_SET_POWER_LEVEL) {
        POWER_LEVEL_INIT = sparam->mgmt_mac_power_level;
        printf("[SIM] Power level set to %d\n", sparam->mgmt_mac_power_level);
        char cmd[256];
        sprintf(cmd, "sed -i \"s/power_level .*/power_level %d/g\" /etc/node_xwg", sparam->mgmt_mac_power_level);
        system(cmd);
    }

    if (ntohs(hmsg->mgmt_keep) & MGMT_SET_POWER_ATTENUATION) {
        POWER_ATTENUATION_INIT = sparam->mgmt_mac_power_attenuation;
        printf("[SIM] Power attenuation set to %d\n", sparam->mgmt_mac_power_attenuation);
        char cmd[256];
        sprintf(cmd, "sed -i \"s/power_attenuation .*/power_attenuation %d/g\" /etc/node_xwg", sparam->mgmt_mac_power_attenuation);
        system(cmd);
    }

    if (ntohs(hmsg->mgmt_keep) & MGMT_SET_RX_CHANNEL_MODE) {
        RX_CHANNEL_MODE_INIT = sparam->mgmt_rx_channel_mode;
        printf("[SIM] RX channel mode set to %d\n", sparam->mgmt_rx_channel_mode);
        char cmd[256];
        sprintf(cmd, "sed -i \"s/rx_channel_mode .*/rx_channel_mode %d/g\" /etc/node_xwg", sparam->mgmt_rx_channel_mode);
        system(cmd);
    }
 }
 
 void sim_set_param_bulk(const char *buffer, int buflen, int type) {
    Smgmt_param sparam;
    memset(&sparam, 0, sizeof(sparam));
    
    // Logic from mgmt_netlink_set_param_wg
    if(type == MGMT_SET_PARAM) {
        if (buflen > sizeof(sparam)) buflen = sizeof(sparam);
        memcpy(&sparam, buffer, buflen);
    } else if(type == MGMT_MULTIPOINT_SET) {
        // Skip IP (4) + ID (2) = 6 bytes
        int offset = sizeof(uint32_t) + sizeof(uint16_t);
        if (buflen > sizeof(sparam) - offset) buflen = sizeof(sparam) - offset;
        memcpy((uint8_t*)&sparam + offset, buffer, buflen);
    }
    
        printf("[SIM] Bulk param update. Freq: %d, Power: %d, BW: %d, MCS: %d\n", 
            ntohl(sparam.mgmt_mac_freq), ntohs(sparam.mgmt_mac_txpower), sparam.mgmt_mac_bw,
            sparam.mgmt_virt_unicast_mcs);
    
    g_sim_status.freq = ntohl(sparam.mgmt_mac_freq);
    g_sim_status.txpower = ntohs(sparam.mgmt_mac_txpower);
    g_sim_status.bw = sparam.mgmt_mac_bw;
    
    FREQ_INIT = g_sim_status.freq;
    POWER_INIT = g_sim_status.txpower;
    BW_INIT = g_sim_status.bw;
    MCS_INIT = sparam.mgmt_virt_unicast_mcs;

    // Also update config files using system calls if needed
    char cmd[256];
    sprintf(cmd, "sed -i \"s/channel .*/channel %d/g\" /etc/node_xwg", g_sim_status.freq);
    system(cmd);
    
    sprintf(cmd, "sed -i \"s/power .*/power %d/g\" /etc/node_xwg", g_sim_status.txpower);
    system(cmd);
    
    sprintf(cmd, "sed -i \"s/bw .*/bw %d/g\" /etc/node_xwg", g_sim_status.bw);
    system(cmd);

    sprintf(cmd, "sed -i \"s/mcs .*/mcs %d/g\" /etc/node_xwg", sparam.mgmt_virt_unicast_mcs);
    system(cmd);
 }

 void sim_set_mode(int enable) {
    g_sim_config.enabled = enable;
    printf("[SIM] Simulation mode: %s\n", enable ? "ENABLED" : "DISABLED");
}

int sim_get_mode(void) {
    return g_sim_config.enabled;
}
