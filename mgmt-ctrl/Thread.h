#ifndef __THERAD_H
#define __THERAD_H

#include "mgmt_types.h"


pthread_t Create_Thread(void (pFun)(void *),void *arg);
pthread_t Create_ThreadAndPriority(INT32 priority,void (pFun)(void *),void *arg);
INT32 Close_Thread(pthread_t threadid);
void ThreadSleep(INT32 tparam);

#endif 