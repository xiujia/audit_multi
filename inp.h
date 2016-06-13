#ifndef _INP_H
#define _INP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>

#define FS_100

#define __USE_GNU

#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sched.h>

#define RUNNING_PATH "/usr/inp"
#define DPDK_PATH "/DPDK"

#define MGT_IP_CFG "/dhcc/forceview/cfg/sys.cfg"


#ifdef FS_10000
#define SSN_HASH_BUCKET_SIZE 		0x400000*4  //16777216
#define SSN_HASH_BUCKET_SIZE_BITS   24
#endif

#ifdef FS_1000
#define SSN_HASH_BUCKET_SIZE 		0x400000
#define SSN_HASH_BUCKET_SIZE_BITS   22
#endif

#ifdef FS_100
#define SSN_HASH_BUCKET_SIZE 		0x200000
#define SSN_HASH_BUCKET_SIZE_BITS   21
#endif

#define SSN_HASH_BUCKET_DEPTH		0x4
#define SSN_HASH_BUCKET_SIZE_MASK 	(SSN_HASH_BUCKET_SIZE-1)
#define SSN_HASH_BUCKET_BITS        2
#define SSN_HASH_BUCKET_MASK        (SSN_HASH_BUCKET_DEPTH-1)

#define SYSTEM_PARAMETER 100
#define COUNTER_KEY		1000
#define SSN_KEY				1001
#define COPY_PACKET_KEY	1002
#define SPEED_METER_KEY	1003
#define IP_AUTH_KEY 1004
#define CONSTRUCT_PACKET_KEY 1005
#define AUDIT_PACKET_KEY 1006
#define AUDIT_POLICY_SHMEM   1007

/*Policy number*/
#define LEVEL_1_POLICY_NUMBER 2048
#define LEVEL_2_POLICY_NUMBER 204800
#define PORT_QOS_NUMBER	8

/*Max Copy packets number*/
#define MAX_COPY_PACKETS_depth 10
/*copy packets cache */
#define COPY_PACKETS_CACHE 10000
#define MAX_PACKET_LEN 1600
#define CONSTRUCT_PACKET_CACHE 100
#define AUDIT_PACKET_CACHE 10000

#define MAX_INTERFACE 8
#define MAX_RING	8

struct system_parameter
{
	u_int32_t mgt_ip;
	u_int32_t link1_int_ip;
	u_int32_t link1_ext_ip;
	u_int8_t  route_flag;
	u_int8_t	nat_flag;
	u_int8_t	user_auth_flag;
	u_int8_t	web_filter_flag;
	u_int8_t	web_log_flag;
	u_int8_t  default_white_black; /*0 default all black,1 default all whilte*/
	u_int8_t  ssn_aging_timer;
	u_int8_t  user_aging_timer;
	u_int8_t  audit_flag;
};


struct packet_counter
{
	u_int64_t all_bytes;
	u_int64_t all_packets;
	u_int64_t arp_bytes;
	u_int64_t arp_packets;
	u_int64_t ip_bytes;
	u_int64_t ip_packets;
	u_int64_t icmp_bytes;
	u_int64_t icmp_packets;
	u_int64_t igmp_bytes;
	u_int64_t igmp_packets;
	u_int64_t old_all_bytes;
	u_int64_t old_all_packets;
	u_int64_t old_arp_bytes;
	u_int64_t old_arp_packets;
	u_int64_t old_ip_bytes;
	u_int64_t old_ip_packets;
	u_int64_t old_icmp_bytes;
	u_int64_t old_icmp_packets;
	u_int64_t old_igmp_bytes;
	u_int64_t old_igmp_packets;
	};

struct ssn 
{
	u_int8_t  link_id;
	u_int32_t sip; 
	u_int32_t dip;
	u_int16_t sport;
	u_int16_t dport;
	u_int8_t  proto;
	u_int16_t app_id;
	u_int64_t in_bytes;
	u_int64_t in_packets;
	u_int64_t out_bytes;
	u_int64_t out_packets;
	u_int64_t old_in_bytes;
	u_int64_t old_in_packets;
	u_int64_t old_out_bytes;
	u_int64_t old_out_packets;
	u_int32_t in_policy_1;
	u_int32_t in_policy_2;
	u_int32_t out_policy_1;
	u_int32_t out_policy_2;
	u_int8_t  copy_packets_counter;
	u_int8_t read_packets_counter;
	u_int8_t read_packets_content;
	u_int32_t user_id;
	u_int8_t audit_flag; /* 0 initial value,1 audit,2 do not copy */
	u_int8_t drop_flag; /* 1 for drop */
	u_int8_t user_flag; /*0 for nomarl user,1 for white user,2 for black user*/
	u_int8_t  age;
};

struct speed_meter {
       u_int64_t limit;  //bps
       int burst;
       int secs_of_day;
       u_int64_t sec_speed;
      };
struct Policy1 {
	u_int8_t src_ip[8192];  //64K bitmap
	u_int8_t dst_ip[8192];  //64K bitmap
	u_int8_t app[256]; //2k bitmap
	};
struct Policy2 {
	u_int8_t src_ip[8192*256];  //64K bitmap
	u_int8_t app[256]; //2k bitmap
	};

struct white_name_map {
	u_int8_t ip[8192*256]; //64K bitmap
};
struct black_name_map {
	u_int8_t ip[8192*256]; //64K bitmap
};
struct packet_slot {
	u_int32_t hash;
	u_int8_t count;
	char packet_content[MAX_PACKET_LEN];
};   
struct copy_packet_counter {
	u_int64_t read;
	u_int64_t write;
	};

struct audit_packet_slot {
	u_int16_t app_id;
	u_int32_t hash;
	u_int32_t sip; //网络序
	u_int32_t user_id;
	char packet_content[MAX_PACKET_LEN];
};

	
/*packet cache存放构造包，等待发送*/
struct packet_cache_slot {
	u_int16_t len;
	char packet_content[MAX_PACKET_LEN];
};
struct packet_cache {
	u_int64_t read;
	u_int64_t write;
	struct packet_cache_slot slot[CONSTRUCT_PACKET_CACHE];
};

struct packet_proto {
	u_int32_t l2_proto; /*0x0806 arp,0x0800 ip,0x8863 p-to-p*/
	u_int8_t  ip_v; /* ipv4 ipv6*/
	u_int32_t ip_proto; /* 0x0001 icmp,0x0002 igmp,0x0006 tcp,0x0011 udp */
	u_int32_t sip;
	u_int32_t dip;
	u_int16_t sport;
	u_int16_t dport;
	u_int8_t	is_get;
	u_int8_t  is_post;
	u_int8_t  is_syn;
	u_int8_t  is_fin;
};
	
struct ssn_policy {
	u_int8_t transmit_flag;
	u_int8_t nat_flag;
	u_int8_t copy_flag;
	u_int8_t count;
	u_int8_t user_flag;
	u_int8_t audit_flag;
	u_int32_t sip;
	u_int32_t user_id;
	u_int16_t app_id;
};



unsigned char * shm_p;
unsigned char * shm_p_temp;

unsigned char * system_parameter_shm_p;

unsigned char * interface_counter_shm_p;
unsigned char * ssn_shm_p;
unsigned char * copy_packet_shm_p;
unsigned char * audit_packet_shm_p;
unsigned char * construct_packet_shm_p;
unsigned char * speed_meter_shm_p;
unsigned char * ip_auth_shm_p;

struct packet_counter * counter_p;
struct ssn * ssn_p;
struct copy_packet_counter * copy_packet_p;
struct copy_packet_counter * audit_packet_p;
struct packet_slot * psp;
struct speed_meter * level_1_meter_p;
struct speed_meter * level_2_meter_p;
struct system_parameter *system_p;


void * ssn_aging(void * arg);
void * heart_beat(void * arg);

pthread_mutex_t qos_mutex;
#endif
