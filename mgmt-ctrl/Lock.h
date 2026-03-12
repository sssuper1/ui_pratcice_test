#ifndef __LOCK_H
#define __LOCK_H_

#include "mgmt_types.h"
#include <pthread.h>
#include <time.h>
#include <errno.h>


pthread_mutex_t CreateLock();
INT32 Unlock(pthread_mutex_t* mutex);
INT32 Lock(pthread_mutex_t* mutex,INT32 passMicroSeconds);
INT32 SetEvent(pthread_cond_t* cond);
INT32 GetEvent(pthread_cond_t* cond,pthread_mutex_t* mutex);
pthread_cond_t CreateEvent();

#endif