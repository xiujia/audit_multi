#ifndef _AUDIT_POLICY_H
#define _AUDIT_POLICY_H

//#include "/DPDK/examples/inp/inp.h"
#include "../include/inp.h"
#include "audit_api.h"
#include "../policy/policy.h"
#include "audit_api.h"


#define AUDIT_P_START  "[AUDIT_POLICY START]"
#define AUDIT_P_END	"[AUDIT_POLICY END]"
#define AUDIT_P_ID		"ID"
#define AUDIT_P_LINK	"LINK_ID"
#define AUDIT_P_INTIP	"INT_IP"
#define AUDIT_P_EXTIP	"EXT_IP"
#define AUDIT_P_TYPE	"AUDIT_TYPE"
#define AUDIT_P_TIME	"TIME_SPAN"
#define AUDIT_P_PATH	"/usr/inp/cfg/audit_policy_conf"
#define AUDIT_POLICY_FILE_READ_BLOCK  100
#define AUDIT_POLICY_LINE_NUM  1000
 #define MAX_WEEK_MIN_NUM 10080





typedef struct audit_policy_map{
	u_int8_t type[4];
	u_int8_t 	time[1261];
	u_int8_t sip[8192];
	u_int8_t dip[8192];
//	u_int8_t user_id[65536];
}Taudit_policy_map;

typedef struct audit_policy{
	u_int8_t   link_id:5,policy_type:3;//link:0,1,2,3,4;policy_type:Ä¬ÈÏÖµ
	u_int16_t policy_id;
	Taudit_policy_map policy_str;
}Taudit_policy;

typedef struct {
	Bool policy_stat;
	u_int16_t policy_count;
	Taudit_policy  audit_policy[MAX_AUDIT_POLICY];
}Taudit_policy_mem;


typedef struct {
	u_int16_t policy_id;
	u_int32_t type;
}Taudit_policy_file;
Taudit_policy_mem * audit_policy_share_mem;
Taudit_policy_mem * policy_shmem;
Taudit_policy_file audit_policy_file_tmp;

Taudit_policy_mem * create_audit_policy_shmem();

Taudit_policy_mem * get_audit_policy_shmem();
inline void audit_policy_init(Taudit_policy_mem * p);
inline u_int32_t audit_policy_ip_map(u_int32_t policy_ip,u_int16_t  n);
inline  void audit_policy_ip_deal(char *content,  int ip_type);
inline void audit_policy_type_deal(char * content);
inline void audit_policy_time_deal(char * content);
inline void audit_policy_file_content_deal(char *type, char *content);
Bool audit_policy_add();
Bool audit_policy_del();


/*dbm*/
void * create_policy_shm(key_t key,size_t shmsz);



//in policy.h

#endif
