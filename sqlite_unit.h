#ifndef __SQLITE_UNIT_H_
#define __SQLITE_UNIT_H_

#include "mgmt_transmit.h"
#include <stdbool.h>

extern uint8_t  rate_auto; 

#define SQLDATALEN           1024


int sqliteinit(void);
int systemexit(void);
void updateData_systeminfo_qk(const char* name,const int value);
int busyHandle(void* ptr, int retry_times);
int sqlite_set_param(void);
void updateData_meshinfo_qk(const char* name,const int value);
void updateData_meshinfo(stInData data);
#endif