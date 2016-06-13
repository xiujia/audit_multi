#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>
#include <unistd.h>  
#include <sys/types.h>  
#include <pthread.h>  
#include <assert.h>  

#include "audit_multi.h"


/* 
*线程池里所有运行和等待的任务都是一个CThread_worker 
*由于所有任务都在链表里，所以是一个链表结构 
*/  




typedef struct worker  
{  
    /*回调函数，任务运行时会调用此函数，注意也可声明成其它形式*/  
    void *(*process) (char *,AUDIT_MULTI_THREAD_PARM *);  
    char fname[128];/*回调函数的参数*/  
//    struct worker *next;  
} CThread_worker;  
  
/*线程池结构*/  
typedef struct  
{  
    pthread_mutex_t queue_lock;  
    pthread_cond_t queue_ready;  
	pthread_cond_t queue_full_ready;
  
    /*链表结构，线程池中所有等待任务*/  
    CThread_worker *queue_head;
//    CThread_worker *queue_tail;
	unsigned int write;
	unsigned int read;
  
    /*是否销毁线程池*/  
    int shutdown;  
    pthread_t *threadid;  
    /*线程池中允许的活动线程数目*/  
    unsigned short max_thread_num;  
    /*当前等待队列的任务数目*/  
    unsigned int cur_queue_size;  
  
} CThread_pool;

static CThread_pool *pool;
AUDIT_MULTI_THREAD_PARM * parm;
struct multi_parm * mp;


int  
pool_add_worker (void *(*process) (char *,AUDIT_MULTI_THREAD_PARM * ), char *arg,char *dir);  

int  
pool_destroy () ;
  
void *  
thread_routine (void *arg)  ;

int  
pool_init (int )  ;



#endif
