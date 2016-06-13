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
*�̳߳����������к͵ȴ���������һ��CThread_worker 
*�������������������������һ������ṹ 
*/  




typedef struct worker  
{  
    /*�ص���������������ʱ����ô˺�����ע��Ҳ��������������ʽ*/  
    void *(*process) (char *,AUDIT_MULTI_THREAD_PARM *);  
    char fname[128];/*�ص������Ĳ���*/  
//    struct worker *next;  
} CThread_worker;  
  
/*�̳߳ؽṹ*/  
typedef struct  
{  
    pthread_mutex_t queue_lock;  
    pthread_cond_t queue_ready;  
	pthread_cond_t queue_full_ready;
  
    /*����ṹ���̳߳������еȴ�����*/  
    CThread_worker *queue_head;
//    CThread_worker *queue_tail;
	unsigned int write;
	unsigned int read;
  
    /*�Ƿ������̳߳�*/  
    int shutdown;  
    pthread_t *threadid;  
    /*�̳߳�������Ļ�߳���Ŀ*/  
    unsigned short max_thread_num;  
    /*��ǰ�ȴ����е�������Ŀ*/  
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
