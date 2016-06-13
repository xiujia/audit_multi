#ifndef _CSP_POLICY_H
#define _CSP_POLICY_H

#include<stdio.h>
#include "csp_db.h"
#include "audit_release.h"

#if GA_TEST
#define GONG_AN_2015 1
#else
#define GONG_AN_2015 0
#endif


#define CSP_POLICY_NUM 10
#define URL_POLICY_MAX 50

#define URL_ACCOUNT_MAP_SIZE 626
#define TIME_MAP_SIZE 1261
#define OPERATE_MAP_SIZE	100

struct request_url {
    int requesturl_id;
	unsigned char account[URL_ACCOUNT_MAP_SIZE];//account_id
};

/* �澯���� */
typedef struct _cache_policy {
    unsigned short valid_flag;
    unsigned short id;
    unsigned short alarm_type;
    unsigned int alarm_level;
    unsigned long time_flag:1,
        url_flag:1,
        account_flag:1,
        operate_flag:1,
		obj_id:3;
	int url_num;

    unsigned char time[TIME_MAP_SIZE];
    struct request_url url[URL_POLICY_MAX];
    unsigned char account[URL_ACCOUNT_MAP_SIZE];
     unsigned char operate[OPERATE_MAP_SIZE];
}CSP_POLICY;

/* ��Ʋ��� */
typedef struct _csp_audit_policy {
    int priority; /* ���ȼ�,1-10,��ӦCACHE_POLICY_CONF.csp_audit_policy[]���������λ�� */
    unsigned short valid_flag;
    unsigned short id;
    unsigned short monitor_obj_id;
    unsigned long obj_time_flag:1,
                  obj_user_flag:1,
                  obj_url_flag:1;

    unsigned char time[TIME_MAP_SIZE];
    //unsigned char ip[8192*32];
    unsigned char ip[256*256*256/8+1];
    unsigned char app_id[8];

    /* 20150401 - �¼�url_id */
    unsigned char requesturl_id[1000/8 +1];
    //unsigned char requesturl_group_id[1000/8 +1];
    #if GONG_AN_2015
    unsigned short security_level;
    #endif
}CSP_AUDIT_POLICY;

/* ���Խṹ */
typedef struct _cache_policy_conf {
    pthread_rwlock_t alarm_lock; /* ����flag��cache_policy */
    char flag; /* �澯���Եķ��ʱ�־,0=û�в���,1=�в���,2=�в��������¸Ķ��� */
    CSP_POLICY cache_policy[CSP_POLICY_NUM]; /* �澯���� */

    pthread_rwlock_t audit_lock; /* ����csp_audit_policy_visit_flag��csp_audit_policy */
	char csp_audit_policy_visit_flag; /* ��Ʋ��Եķ��ʱ�־,0=û�в���,1=�в���,2=�в��������¸Ķ��� */
	CSP_AUDIT_POLICY csp_audit_policy[CSP_POLICY_NUM]; /* ��Ʋ��� */
}CACHE_POLICY_CONF;

extern CACHE_POLICY_CONF * cachePolicy;

void bit_set(int i,u_int8_t a[]) ;
void bit_clr(int i,u_int8_t a[]) ;
int  bit_test(int i,u_int8_t a[]);

void *create_audit_shm(key_t key, size_t size, int oflag);
void *get_audit_shm(key_t key, int oflag);
void *create_audit_cache_policy_shm();
void *get_audit_cache_policy_shm();

int alarm_policy_set(MYSQL *my);
int alarm_policy_add(MYSQL *my, int id);
int alarm_policy_del(int id);

int csp_audit_policy_set(MYSQL *my);
int csp_audit_policy_add(MYSQL *my, int id);
int csp_audit_policy_del(int id);
int csp_audit_policy_clr();

#endif /* _CSP_POLICY_H */
