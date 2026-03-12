#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "sim_heartbeat.h"
#include "mgmt_types.h"

// Mock for system() if needed, but standard library has it.
// We can just let it print to stdout since sim_heartbeat.c calls it.

void test_init() {
    printf("Testing Initialization...\n");
    sim_init();
    assert(sim_get_mode() == 1);
    printf("Initialization OK.\n");
}

void test_data_generation() {
    printf("Testing Data Generation...\n");
    struct mgmt_send status1, status2;
    sim_get_veth_info(&status1);
    
    // Check if data is populated
    assert(status1.node_id == 1);
    assert(status1.neigh_num == 3);
    printf("Initial Node ID: %d, Neighbors: %d\n", status1.node_id, status1.neigh_num);

    sim_get_veth_info(&status2);
    
    // Check if data changes (simulation works)
    printf("TX1: %d, TX2: %d\n", status1.tx, status2.tx);
    if (status2.tx <= status1.tx) {
        printf("Warning: TX counter did not increase (might be random chance or bug)\n");
    } else {
        printf("TX counter increased OK.\n");
    }
    
    printf("Data Generation OK.\n");
}

void test_param_update() {
    printf("Testing Parameter Update...\n");
    
    // Construct a fake parameter update message
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    Smgmt_header* hmsg = (Smgmt_header*)buffer;
    Smgmt_set_param* sparam = (Smgmt_set_param*)hmsg->mgmt_data;
    
    hmsg->mgmt_type = htons(MGMT_SET_FREQUENCY | MGMT_SET_POWER);
    sparam->mgmt_mac_freq = htonl(5200000);
    sparam->mgmt_mac_txpower = htons(25);
    
    sim_set_param(buffer, sizeof(buffer), 0);
    
    // Verify by getting info again
    struct mgmt_send status;
    sim_get_veth_info(&status);
    
    printf("Frequency: %d (Expected: 5200000)\n", status.freq);
    printf("Tx Power: %d (Expected: 25)\n", status.txpower);

    assert(status.freq == 5200000);
    assert(status.txpower == 25);
    
    printf("Parameter Update OK.\n");
}

void test_param_update_bulk() {
    printf("Testing Bulk Parameter Update...\n");
    
    Smgmt_param sparam;
    memset(&sparam, 0, sizeof(sparam));
    sparam.mgmt_mac_freq = htonl(5805000);
    sparam.mgmt_mac_txpower = htons(33);
    sparam.mgmt_mac_bw = 40;
    
    sim_set_param_bulk((char*)&sparam, sizeof(sparam), MGMT_SET_PARAM);
    
    struct mgmt_send status;
    sim_get_veth_info(&status);
    
    assert(status.freq == 5805000);
    assert(status.txpower == 33);
    assert(status.bw == 40);
    
    printf("Bulk Parameter Update OK.\n");
}

int main() {
    test_init();
    test_data_generation();
    test_param_update();
    test_param_update_bulk();
    printf("All Tests Passed!\n");
    return 0;
}
