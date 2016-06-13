/*
2014-11-20 11:49    auditcheckfp_callback ���߳���
��������audit_desfp_lock �߳���
2014-12-02    10:00 auditchckfp_callback �̺߳����޸ģ��ļ����������꼴�رգ�����Ҫɨ��������ʹ�����
ȥ������audit_desfp_lock�߳���
2014-12-15     11:00 get_audit_type������ʶ��Ӧ��ȫ�ӳ������ж��������жϷ���
*/

#include "../include/inp.h"
//#include "audit_policy.h"
#include "csp_policy.h"
#include "../include/app_static.h" //��Ҫ֪��·��
#include "audit_api.h"
#include <dirent.h>
#include "audit_database.h"
#include "audit_time_api.h"
#include <sched.h>
#include "csp_redis.h"
#include  "redis_new_api.h"
#define __NEW_20150721 1
#define __NEW_TEST 0

#if __NEW_20150721
u_int32_t audit_sqlserver(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char * data);
#endif

#define UDP_PRO  17
#define TCP_PRO  6
#define ICMP_PRO 1
#define IPV4_PRO 4
#define IPV6_PRO 6
#define CFG_F_PATH "/usr/inp/cfg/inp.config"

int audit_policy_shmem_ret = 0;
Bool sipm=0;
Bool tmm = 0;
Bool typem = 0;
char tablesTime[AUDIT_TIME_LEN];

unsigned char * audit_shm[THREADS_ORACLE_NUM];//4 
unsigned int remote_computer_ip[10]; 
int computer_conts;

u_int8_t  SSN_HASH_BUCKET_SIZE_BITS = 0;
u_int32_t SSN_HASH_BUCKET_SIZE = 0;
u_int32_t SSN_HASH_BUCKET_SIZE_MASK = 0;

struct copy_packet_counter *copy_count[THREADS_ORACLE_NUM];
struct audit_packet_slot *audit_slot[THREADS_ORACLE_NUM];
//struct policy_share_mem * policy_mem;
struct system_parameter *sysParameter;
CACHE_POLICY_CONF * cachePolicy;
struct audit_monitor * appIdConfig;
//int appIdConfigLineNum;

struct  audit_info_st  audit_info;
pthread_t auditcheckfp_thread;
pthread_t audit_dbm_t[THREADS_ORACLE_NUM+THREADS_NUM];
//pthread_mutex_t csp_id_mutex;
pthread_mutex_t telnet_id_mutex;
pthread_mutex_t ftp_id_mutex;
pthread_rwlock_t tablesTime_lock;
Taudit_file_info file_info[AUDIT_INDEX_NUM];

pthread_rwlockattr_t rwlock; /* ���/����������ݵ������� */

redisContext *redis_conn[THREADS_ORACLE_NUM];
//pthread_t auditbbsfp_thread;
//int bbs_fp[ MAX_FILE_FD];
DIR *checkdir=NULL;

static int ethernet_len = 14;
FILE * audit_error_log = NULL;
//DBMHOSTS ips[AUDIT_HOSTS_MAX];
//int DbmHostNum ;
//net

//u_int32_t ecp_addr[8] = {
 //   0x0AA0106F,/*111*/
//    0x0AA01078,/*120*/
//    0x0AA01079,/*121*/
//    0x0AA0107A,/*122*/
 //   0x0AA01082,/*130*/
//    0x0AA01084,/*132*/
//    0x0AA0108C,/*140*/
//    0x0AA0108D /*141*/
//};
//u_int32_t main_cache_addr[2] = {
//    0x0AA0105A,/*90*/
//    0x0AA01012 /*12*/
//};

int get_audit_monitor_cfg_line_num(char * fname){
	FILE * fp;
	char  mid;
	int num=0;
	fp = fopen(fname,"r");
	if(!fp){
		return -1;
	}
	while(!feof(fp)) 
	{ 
		mid=fgetc(fp); 
		if(mid=='\n'){
			num++; 
		}
	}
	fclose(fp);
	return num;
}
/*
Bool isEcpToCache(struct audit_pack_info_head *p_info_hd) {
    int i,j;
    for(j = 0;j < 8;j++) {
        if(p_info_hd->cli_ip == ecp_addr[j]) {
            if(p_info_hd->ser_ip == main_cache_addr[0]) {
                return 1;
            } else if(p_info_hd->ser_ip == main_cache_addr[1]) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0;
}
*/
/* �����ж�  �Ժ�ֱ���ж�isnew */

int streamStat(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char *data) {
    if(p_info->hash_cliip == 0 && p_info->hash_serip == 0
       && p_info->hash_clipt == 0 && p_info->hash_serpt == 0) {
        //printf("1111111111111111111111111111111\n");
     //   p_info->audit_type = get_audit_type(data, p_info_hd);
        p_info->isnew  = 1;
		if(p_info->audit_type == 0) {
            return -1; //next packet
        } else if(p_info->audit_type == -1) {
            p_info->hash_cliip = p_info_hd->cli_ip;
            p_info->hash_serip = p_info_hd->ser_ip;
            p_info->hash_clipt = p_info_hd->cli_port;
            p_info->hash_serpt= p_info_hd->ser_port;
            p_info->hash_proto = p_info_hd->proto;
            return -2; //noneed
        } else {
            p_info->hash_cliip = p_info_hd->cli_ip;
            p_info->hash_serip = p_info_hd->ser_ip;
            p_info->hash_clipt = p_info_hd->cli_port;
            p_info->hash_serpt= p_info_hd->ser_port;
            p_info->hash_proto = p_info_hd->proto;
            return p_info->audit_type;//need
        }
    } 
	else if(p_info->hash_cliip != p_info_hd->cli_ip
              ||p_info->hash_serip != p_info_hd->ser_ip
              || p_info->hash_clipt!= p_info_hd->cli_port
              || p_info->hash_serpt!= p_info_hd->ser_port
              ||p_info->hash_proto!=p_info_hd->proto) {
        //printf("2222222222222222222222222222222\n");
        //printf("hash:%lu \n",p_info_hd->hash);
        //printf("p_info    : cliip= %u,serip = %u,clipt =%d, serpt = %d,proto = %d\n",p_info->hash_cliip,p_info->hash_serip,p_info->hash_clipt,p_info->hash_serpt,p_info->hash_proto);
        //printf("p_info_hd : cliip= %u,serip = %u,clipt =%d, serpt = %d,proto = %d\n",p_info_hd->cli_ip,p_info_hd->ser_ip,p_info_hd->cli_port,p_info_hd->ser_port,p_info_hd->proto);
		memset(p_info,0,sizeof(*p_info));
		p_info->isnew = 1;
        //memset(p_info,0,sizeof(struct audit_pack_info));
      //  p_info->audit_type = get_audit_type(data,p_info_hd);
        if(p_info->audit_type == 0) {
            return -1; //next packet
        } else if(p_info->audit_type == -1) {
            p_info->hash_cliip = p_info_hd->cli_ip;
            p_info->hash_serip = p_info_hd->ser_ip;
            p_info->hash_clipt = p_info_hd->cli_port;
            p_info->hash_serpt= p_info_hd->ser_port;
            p_info->hash_proto = p_info_hd->proto;
            return -2; //noneed
        } else {
            p_info->hash_cliip = p_info_hd->cli_ip;
            p_info->hash_serip = p_info_hd->ser_ip;
            p_info->hash_clipt = p_info_hd->cli_port;
            p_info->hash_serpt= p_info_hd->ser_port;
            p_info->hash_proto = p_info_hd->proto;
            return p_info->audit_type;//need
        }
    } 
	else {
    
        //printf("333333333333333333333333333333333333333\n");
     //   if(p_info->audit_policy_stat !=0) return p_info->audit_type;
		if(p_info_hd->app_id == 3|| p_info_hd->app_id == 5){
			if(p_info->audit_policy_stat !=0) return p_info->audit_type;
		}

        if(p_info->audit_type == 0){
     //       p_info->audit_type = get_audit_type(data,p_info_hd);
            if(p_info->audit_type == 0){
                return -1;
            } else if(p_info->audit_type == -1) {
                p_info->hash_cliip = p_info_hd->cli_ip;
                p_info->hash_serip = p_info_hd->ser_ip;
                p_info->hash_clipt = p_info_hd->cli_port;
                p_info->hash_serpt= p_info_hd->ser_port;
                p_info->hash_proto = p_info_hd->proto;
                return -2;
            } else {
                p_info->hash_cliip = p_info_hd->cli_ip;
                p_info->hash_serip = p_info_hd->ser_ip;
                p_info->hash_clipt = p_info_hd->cli_port;
                p_info->hash_serpt= p_info_hd->ser_port;
                p_info->hash_proto = p_info_hd->proto;
                return p_info->audit_type;
            }
        } else {
            return p_info->audit_type;
        }
    }
}

/* appid�������Ʋ���ƥ�� */
int audit_cache_policy_app_match(int appid,CSP_AUDIT_POLICY* cpconf) {
    if(bit_test(appid,cpconf->app_id) >0){
        return 1;
    }
    return 0;
}

/* ip�������Ʋ���ƥ�� */
int audit_cache_policy_ip_match(u_int32_t ip,CSP_AUDIT_POLICY* cpconf) {
    u_int32_t iip0,iip1,iip2,iip3,ip_map_addr;
    unsigned char ipstr[20]={0};

    if(cpconf->obj_user_flag == 1 )//ȫ�����
        return 1;
    inet_ntop(AF_INET,(void*)&ip,ipstr,20);
    sscanf(ipstr, "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);
    ip_map_addr =iip2*256*256+ iip1*256 + iip0;
    //printf("ip_map_addr = %u\n",ip_map_addr);

    if(bit_test(ip_map_addr,cpconf->ip)>0){
        return 1;
    }
    return 0;
}

/* time�������Ʋ���ƥ�� */
int audit_cache_policy_time_match(u_int32_t time,CSP_AUDIT_POLICY* cpconf) {
    if(bit_test(time,cpconf->time)>0){
        return 1;
    }
    return 0;
}

/* url�������Ʋ���ƥ�� */
int audit_cache_policy_url_match(int urlid,CSP_AUDIT_POLICY * cpconf){
    if(bit_test(urlid,cpconf->requesturl_id)>0){
        return 1;
    }
    return 0;
}

/* cache���ݿ�ģ�superserver/portal/ftp/telnet�������Ʋ���ƥ�䣬ƥ�䷵��1������0 */
int audit_cache_policy_match(struct audit_pack_info *p_info,int appid, u_int32_t ip, u_int32_t time, char * url, CACHE_POLICY_CONF* cpconf, redisContext *conn) {
    int i,app_flag,ip_flag,time_flag,url_flag;
    int id = 0;
    char urlid[10]={0};
	int flag=0;

#if __NEW_20150721
    if (1000 == appid) {
        for(i = 0;i < CSP_POLICY_NUM;i++) {
            if(cpconf->csp_audit_policy[i].valid_flag == 0){
                continue;
            }
            if (4 != cpconf->csp_audit_policy[i].monitor_obj_id) {
                continue;
            }
            ip_flag = audit_cache_policy_ip_match(ip, &cpconf->csp_audit_policy[i]);
            time_flag = audit_cache_policy_time_match(time, &cpconf->csp_audit_policy[i]);
            if(ip_flag & time_flag){
                return 1;
            }
        }
        return 0;
    }
#endif

    //printf("appid:%d,%u,%u,%s\n",appid,ip,time,url);
    if(appid ==2|| appid == 3||appid == 4) {
        for(i = 0;i < CSP_POLICY_NUM;i++) {
            if(cpconf->csp_audit_policy[i].valid_flag == 0){
                continue;
            }
            app_flag = audit_cache_policy_app_match(appid,&cpconf->csp_audit_policy[i]);
            ip_flag = audit_cache_policy_ip_match(ip,&cpconf->csp_audit_policy[i]);
            time_flag = audit_cache_policy_time_match(time,&cpconf->csp_audit_policy[i]);
            //printf("policy %d: %d %d %d\n",cpconf->csp_audit_policy[i].id,app_flag,ip_flag,time_flag);
            if(ip_flag == 0){
				#if GA_TEST
                continue;
				#else
				return 0;
				#endif
            }
			#if GA_TEST
            if(app_flag & ip_flag & time_flag){
				flag = 1;
				if(p_info->security_level==0||p_info->security_level > cpconf->csp_audit_policy[i].security_level){
					p_info->security_level = cpconf->csp_audit_policy[i].security_level;
				}
            }
			#else
            if(app_flag & ip_flag & time_flag){
                return 1;
            }
			#endif
			
        }
        //printf("policy not match.\n");
        #if GA_TEST
			return flag;
		#endif
        return 0;
    } else {
        /*
        if(!conn) return 0;
        memset(urlid,0,10);
        CspRedisOperation(REDIS_DB_URL_ID,OPERATION_GET,url,urlid,conn);
        if(strlen(urlid) !=0) {
            id = atoi(urlid);
        } else {
            id = 0;
        } */
        //if(!conn) return 0;
        for(i = 0;i < CSP_POLICY_NUM;i++) {
            if(cpconf->csp_audit_policy[i].valid_flag == 0) {
                continue;
            }

            app_flag = audit_cache_policy_app_match(appid,&cpconf->csp_audit_policy[i]);
            ip_flag = audit_cache_policy_ip_match(ip,&cpconf->csp_audit_policy[i]);
            time_flag = audit_cache_policy_time_match(time,&cpconf->csp_audit_policy[i]);
            if(cpconf->csp_audit_policy[i].obj_url_flag == 1) {
                memset(urlid,0,10);
                CspRedisOperation(REDIS_DB_URL_ID,OPERATION_GET,url,urlid,conn);
                if(strlen(urlid) !=0) {
                    id = atoi(urlid);
                } else {
                    id = 0;
                }
                url_flag = audit_cache_policy_url_match(id,&cpconf->csp_audit_policy[i]);
            } else {
                url_flag = 1;
            }
            if(ip_flag == 0) {
                return 0;
            }
            //printf("policy %d: %d %d %d %d\n",cpconf->csp_audit_policy[i].id,app_flag,ip_flag,time_flag,url_flag);
            if(app_flag&ip_flag&time_flag&url_flag) {
                return 1;
            }
        }
        //printf("policy not match.\n");
        return 0;
    }
}

int audit_dbmonitor_policy_ip_match(u_int32_t ip,u_int32_t time,CACHE_POLICY_CONF* cpconf){
    int i,ip_flag =0 ,time_flag=0;
    int id = 0;


	for(i = 0;i < CSP_POLICY_NUM;i++) {
		if(cpconf->csp_audit_policy[i].valid_flag == 0){
			continue;
		}
		ip_flag = audit_cache_policy_ip_match(ip,&cpconf->csp_audit_policy[i]);
//		time_flag = audit_cache_policy_time_match(time,&cpconf->csp_audit_policy[i]);
		//printf("policy %d: %d %d %d\n",cpconf->csp_audit_policy[i].id,app_flag,ip_flag,time_flag);

		if(ip_flag){
			return NEED;
		}
	}
	return NONEED;
}
int audit_dbmonitor_policy_app_match(int appid,CACHE_POLICY_CONF* cpconf){
    int i,app_flag =0 ,time_flag=0,ip_flag=0;
    int id = 0;


	for(i = 0;i < CSP_POLICY_NUM;i++) {
		if(cpconf->csp_audit_policy[i].valid_flag == 0){
			continue;
		}
		
	//	ip_flag = audit_cache_policy_ip_match(ip,&cpconf->csp_audit_policy[i]);
		app_flag = audit_cache_policy_app_match(appid,&cpconf->csp_audit_policy[i]);
	//	time_flag = audit_cache_policy_time_match(time,&cpconf->csp_audit_policy[i]);
		//printf("policy %d: %d %d %d\n",cpconf->csp_audit_policy[i].id,app_flag,ip_flag,time_flag);

		if(app_flag){
			return NEED;
		}
	}
	return NONEED;
}

int audit_dbmonitor_policy_match( struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,CACHE_POLICY_CONF* cpconf){
   int i,app_flag =0 ,time_flag=0,ip_flag=0,obj_flag=0;
    int id = 0;


	for(i = 0;i < CSP_POLICY_NUM;i++) {
		if(cpconf->csp_audit_policy[i].valid_flag == 0){
			continue;
		}
		if(p_info_hd->obj_id == cpconf->csp_audit_policy[i].monitor_obj_id){
			obj_flag = 1;
		}
		else{
			obj_flag = 0;
		}
		ip_flag = audit_cache_policy_ip_match(p_info_hd->cli_ip,&cpconf->csp_audit_policy[i]);
		app_flag = audit_cache_policy_app_match(p_info_hd->app_id,&cpconf->csp_audit_policy[i]);
		time_flag = audit_cache_policy_time_match(p_info_hd->timenow,&cpconf->csp_audit_policy[i]);
		//printf("policy %d: %d %d %d\n",cpconf->csp_audit_policy[i].id,app_flag,ip_flag,time_flag);
		if(p_info_hd->obj_id == 3||p_info_hd->obj_id == 4){
			if( obj_flag & ip_flag & time_flag ){
				p_info->policy_id = i;
				return NEED;
			}
		}
		else{
			if( obj_flag & ip_flag & time_flag &app_flag){
				p_info->policy_id = i;
				return NEED;
			}
		}

	}
	return NONEED;
}

int audit_oracle_type_match(int type_flag,u_int16_t pid,CACHE_POLICY_CONF* cpconf){
		int i,app_flag =0 ,time_flag=0,ip_flag=0,obj_flag=0;
	  	int id = 0;
	


		   app_flag = audit_cache_policy_app_match(type_flag,&cpconf->csp_audit_policy[pid]);
		   //printf("policy %d: %d %d %d\n",cpconf->csp_audit_policy[i].id,app_flag,ip_flag,time_flag);
		   if(app_flag == 1)
		   	return 1;
	   
	   return 0;

}

/*
Bool audit_policy_ip_match(u_int32_t ip,u_int32_t type,int index) {
    u_int32_t ip_bitmap;

    ip_bitmap = audit_policy_ip_map(ntohl(ip),2);
    //printf("policy_ip = %s,ip_bitmap = %d\n",inet_ntoa(*(struct in_addr *)&ip),ip_bitmap);
    if(type == 0){
        if(bit_test(ip_bitmap,audit_policy_share_mem->audit_policy[index].policy_str.sip) > 0){
        return 1;
        } else return 0;
    }
    else if(type == 1){
        if(bit_test(ip_bitmap,audit_policy_share_mem->audit_policy[index].policy_str.dip) > 0){
            return 1;
        } else return 0;
    }
    return 0;
}

Bool audit_policy_audittype_match(u_int32_t audit_type,u_int32_t index) {
    if  (bit_test(audit_type,audit_policy_share_mem->audit_policy[index].policy_str.type)){
        return 1;
    }
    return 0;
}

Bool audit_policy_time_match(u_int32_t time,u_int32_t index) {
    if  (bit_test(time,audit_policy_share_mem->audit_policy[index].policy_str.time)){
        return 1;
    }
    return 0;
}
*/

/*
Bool audit_policy_white_match(u_int32_t ip) {
    u_int32_t ip_bitmap;
    ip_bitmap = audit_policy_ip_map(ntohl(ip),3);
    if(bit_test(ip_bitmap,policy_mem->white_map.ip) > 0){
        return 1;
    }
    return 0;
}
*/

/*
Bool audit_policy_match(struct audit_pack_info_head *p_info_hd,struct audit_pack_info * p_info){
    int i;
    Bool iswhite, dipmatch = 0,sipmatch = 0,timematch = 0,audittypematch = 0;
    //�ж�sip�Ƿ��ڰ������� �������ֱ�ӷ���NONEED
    //iswhite = audit_policy_white_match(p_info_hd->int_ip);
    //if(iswhite) return NONEED;
    //printf("is not white\n ");
    //printf("there is %d audit_policy\n",audit_policy_share_mem->policy_count);
    for(i=0;i<audit_policy_share_mem->policy_count;i++){
    //printf("type = %d\n",p_info_hd->audit_type);
    sipmatch = audit_policy_ip_match(p_info_hd->int_ip,0,i);
    //dipmatch = audit_policy_ip_match(p_info_hd->desip,1,i);
    timematch = audit_policy_time_match(p_info_hd->timenow,i);
    audittypematch = audit_policy_audittype_match(p_info->audit_type,i);
    //sleep(3);
#if AUDIT_DEBUG
    printf("sipmatch = %02x,timematch = %02x,audittypematch = %02x\n",sipmatch,timematch,audittypematch);
#endif
    //sleep(4);
    sipm = sipmatch;
    tmm = timematch;
    typem = audittypematch;
    if(sipmatch == 1 &&  timematch ==  1    && audittypematch == 1){
#if AUDIT_DEBUG
    printf("sipmatch = %02x,timematch = %02x,audittypematch = %02x\n",sipmatch,timematch,audittypematch);
#endif
    //printf("hash=%d  is need audit ,ip:%s,audit_type:%x,app_id:%u,userid:%d\n",p_info_hd->hash,inet_ntoa(*(struct in_addr *)&p_info_hd->srcip),p_info_hd->audit_type,p_info_hd->app_id,p_info_hd->user_id);
    //exit(0);
    return NEED;
    }
    }
    if(p_info->audit_type == 0){ //���Ӧ������Ϊ0 �򷵻س�ʼ״̬
        return 0;
    }
    return NONEED;
}
*/

void * get_audit_shm_main(key_t key) {
    int shm_size;
    int ret;
    unsigned char * sh;
    shm_size=sizeof(struct copy_packet_counter);
    shm_size+=sizeof(struct audit_packet_slot)*AUDIT_PACKET_CACHE;
    //ret = shmget(AUDIT_PACKET_KEY, shm_size, IPC_CREAT);
    //shmctl(ret, IPC_RMID, 0 );
    ret = shmget(key, shm_size, 0666|IPC_EXCL);
    if(ret == -1 ){
        return NULL;
    }
    sh = (unsigned char * )shmat( ret, 0, 0 );
    return sh;
}

/*void  init_Stream_str( ) {
    int i;
    for(i=0;i<SSN_HASH_BUCKET_SIZE;i++){
        bit_clr(i,streamList.stat);
    }
}
*/

/*
struct policy_share_mem *get_policy_share_cache( void ) {
    int fd_new=0;
    struct policy_share_mem *share_cache;
    //����˴�����-1����鿴 /proc/sys/kernel/shmmax��ֵ������������
    if( (fd_new=shmget(SHARE_POLICY_KEY,sizeof(struct policy_share_mem),SVSHM_MODE | IPC_CREAT))==-1) {
        fprintf(stderr,"shmget struct policy_share_mem error 1\n");
        return NULL;
    }
    share_cache=(struct policy_share_mem*)shmat(fd_new,NULL,0);
    return share_cache;
}
*/

void *CspGetShareMem(int key, size_t shm_size) {

    int ret;
    char * sh;
    ret = shmget(key, shm_size, 0666|IPC_EXCL);
    if(ret == -1 ){
        perror("shmget");
        return NULL;
    }
    sh = (void* )shmat( ret, 0, 0 );
    return sh;
}

static inline void trim(char *s) {
    int i;
    for(i=0;i<strlen(s);i++) {
        if((s[i]==0x20)||(s[i]=='\n')) {
            s[i]='\0';
            return;
        }
    }
}
/*
//file  : /usr/inp/cfg/dbm_cache_hosts.cfg
int  audit_get_dbm_host(DBMHOSTS* ip) {
    FILE * hostFp;
    int len;
    char * pos;
    DbmHostNum = 0;

    hostFp = fopen("/usr/inp/cfg/dbm_cache_hosts.cfg","r");
    if(!hostFp)return -1;
    while(1) {
        if(DbmHostNum == AUDIT_HOSTS_MAX) break;
        if(fgets(ip[DbmHostNum].host, 100, hostFp) != NULL) {
            if((pos = strstr(ip[DbmHostNum].host, "\n")))
                *pos = '\0';
            //printf("%s\n",ip[DbmHostNum].host);
            DbmHostNum++;
            continue;
        }
        break;
    }
    fclose(hostFp);
    return DbmHostNum;
}
*/
/* ��ӡCSP��ҳ���������Ϣ������debug */
void CspDebug(struct audit_pack_info_head *pk_info_hd,struct audit_pack_info *pk_info ,unsigned char *data) {
    char sip[100],dip[100];
    memset(sip,0,sizeof(sip));
    memset(dip,0,sizeof(dip));

    sprintf(sip,"%d.%d.%d.%d",(int)UCHARPTR(&pk_info_hd->cli_ip)[0],(int)UCHARPTR(&pk_info_hd->cli_ip)[1],(int)UCHARPTR(&pk_info_hd->cli_ip)[2],(int)UCHARPTR(&pk_info_hd->cli_ip)[3]);
    sprintf(dip,"%d.%d.%d.%d",(int)UCHARPTR(&pk_info_hd->ser_ip)[0],(int)UCHARPTR(&pk_info_hd->ser_ip)[1],(int)UCHARPTR(&pk_info_hd->ser_ip)[2],(int)UCHARPTR(&pk_info_hd->ser_ip)[3]);
    //sprintf(sip,"%s",inet_ntoa(*(struct in_addr *)&pk_info_hd->cli_ip));
    //sprintf(dip,"%s",inet_ntoa(*(struct in_addr *)&pk_info_hd->ser_ip));

    if(pk_info_hd->isin && memcmp(data,"HTTP/1.",7)==0) {                                                                                                                                        //pk_info_hd->cli_ip,pk_info_hd->cli_port,pk_info_hd->ser_ip,pk_info_hd->ser_port
        printf("get_audit_type  match Http  policy_stat = %d, hash = %lu,audit_type = %d cli = %s:%u ser = %s:%u \n",pk_info->audit_policy_stat  ,pk_info_hd->hash,pk_info->audit_type,sip,pk_info_hd->cli_port,dip,pk_info_hd->ser_port);
    } else if(pk_info_hd->isin==0 && memcmp(data,"POST ",5)==0) {
        printf("get_audit_type  match Post  policy_stat = %d, hash = %lu,audit_type = %d cli = %s:%u ser = %s:%u \n",pk_info->audit_policy_stat  ,pk_info_hd->hash,pk_info->audit_type,sip,pk_info_hd->cli_port,dip,pk_info_hd->ser_port);
    } else if(pk_info_hd->isin==0 && memcmp(data,"GET ",4)==0) {
        printf("policy  match Get  policy_stat = %d, hash = %lu,audit_type = %d cli = %s:%u ser = %s:%u \n",pk_info->audit_policy_stat  ,pk_info_hd->hash,pk_info->audit_type,sip,pk_info_hd->cli_port,dip,pk_info_hd->ser_port);
    }
}


void audit_init() {
    printf("initing...\n");
    FILE * config_file;
    u_int32_t shm_size;
    char cmd[100];
    char temp_str[101]={0};
    //key_t key[]={AUDIT_PACKET_KEY};
    key_t key[]={
	    AUDIT_PACKET_KEY1,
	    AUDIT_PACKET_KEY2,
	    AUDIT_PACKET_KEY3,
	    AUDIT_PACKET_KEY4,
	    AUDIT_PACKET_KEY5,
	    AUDIT_PACKET_KEY6,
	    AUDIT_PACKET_KEY7,
	    AUDIT_PACKET_KEY8,
	    AUDIT_PACKET_KEY9,
	    AUDIT_PACKET_KEY10
    };
//	appIdConfigLineNum = 0;
//	appIdConfigLineNum = get_audit_monitor_cfg_line_num("/usr/inp/bin/cfg/audit_monitor_cfg")
    int i = 0;
    //key[0]=AUDIT_PACKET_KEY1;

    if(pthread_rwlockattr_init(&rwlock) != 0) {
        exit(0);
    }
    if(pthread_rwlockattr_setpshared(&rwlock, PTHREAD_PROCESS_SHARED) != 0) {
        exit(0);
    }
  //  if(pthread_mutex_init(&csp_id_mutex, NULL) != 0) {
   //  exit(0);
   // }
    if(pthread_mutex_init(&telnet_id_mutex, NULL) != 0) {
        exit(0);
    }
    if(pthread_mutex_init(&ftp_id_mutex, NULL) != 0) {
        exit(0);
    }
    if(pthread_rwlock_init(&tablesTime_lock, NULL) != 0) {
        exit(0);
    }

    Taudit_file_info file_info[AUDIT_INDEX_NUM];
    memset(file_info, 0, sizeof(file_info));
    int index;
    for(index = 0;index < AUDIT_INDEX_NUM;index++) {
        if(pthread_rwlock_init(&file_info[index].ftp_file_lock,NULL)!=0){
            exit(0);
        }
    }

    pthread_mutex_lock(&ftp_id_mutex);
    ftp_id = 0;
    pthread_mutex_unlock(&ftp_id_mutex);

    pthread_mutex_lock(&telnet_id_mutex);
    telnet_id = 0;
    pthread_mutex_unlock(&telnet_id_mutex);

//    pthread_mutex_lock(&csp_id_mutex);
//    csp_id = GetId(AUDIT_CSP_ID_PATH);
    //printf("csp_id = %d\n",csp_id);
//    pthread_mutex_unlock(&csp_id_mutex);
/*
    memset(ips,0,AUDIT_HOSTS_MAX*sizeof(DBMHOSTS));
    if(audit_get_dbm_host(ips)<=0) {
        printf("No host config.\n");
        exit(0);
    }
*/
    config_file = fopen(CFG_F_PATH,"r");
    if(!config_file) {
        printf("/usr/inp/cfg/inp.config read failed.\n");
        exit(0);
    }
    /*
    shm_size=sizeof(struct system_parameter);
    sysParameter = (struct system_parameter *)CspGetShareMem(SYSTEM_PARAMETER,shm_size);
    printf("config mac :%hhx%hhx%hhx%hhx%hhx%hhx\n",sysParameter->uplink_mac1[0],sysParameter->uplink_mac1[1],sysParameter->uplink_mac1[2],sysParameter->uplink_mac1[3],sysParameter->uplink_mac1[4],sysParameter->uplink_mac1[5]);
    */
    if(fgets(temp_str,100,config_file)!=NULL) {
        trim(temp_str);
        //SSN_HASH_BUCKET_SIZE_BITS=atol(temp_str);
        sscanf(temp_str,"%d",&SSN_HASH_BUCKET_SIZE_BITS);
        SSN_HASH_BUCKET_SIZE=1<<SSN_HASH_BUCKET_SIZE_BITS;
        SSN_HASH_BUCKET_SIZE_MASK=SSN_HASH_BUCKET_SIZE-1;
        printf("ssn_hash_bucket_size = %lu\n",SSN_HASH_BUCKET_SIZE);
    }
    fclose(config_file);

    system("mkdir -p /data/audit/tmp/bbs/");
    system("mkdir -p /data/audit/tmp/eml/");
    system("mkdir -p /data/audit/tmp/webmail/");
    //system("mkdir -p /data/audit/tmp/csp/");

    system("rm -rf /data/audit/tmp/bbs/*");
    system("rm -rf  /data/audit/tmp/eml/*");
    system("rm -rf  /data/audit/tmp/webmail/*");

    system("mkdir -p /data/audit/bbs/");
    system("mkdir -p /data/audit/eml/");
    system("mkdir -p /data/audit/webmail/");

    system("rm -rf /data/audit/bbs/*");
    system("rm -rf  /data/audit/eml/*");
    system("rm -rf  /data/audit/webmail/*");

    system("mkdir -p /data/audit/http/");
    system("mkdir -p /data/audit/telnet/");
    system("mkdir -p /data/audit/studio/");

    system("mkdir -p /media/data/sql/");
    system("mkdir -p /media/data/sql_tmp/");
    system("mkdir -p /media/data/sql_backup/");

    system("rm -rf  /media/data/sql/*");
    system("rm -rf  /media/data/sql_tmp/*");
    system("rm -rf  /media/data/sql_backup/*");

#if __NEW_20150721
	system("rm -f /dev/shm/sqlserver_tmp/*");
	system("mkdir -p /dev/shm/sqlserver_tmp");
#endif

    system("rm -rf  /dev/shm/studio_tsi_tmp/");
    system("mkdir -p /dev/shm/studio_tsi_tmp/");

    system("rm -rf  /dev/shm/telnet_tmp/");
    system("mkdir -p /dev/shm/telnet_tmp/");

    system("rm -rf /dev/shm/ftp_tmp/");
    system("mkdir -p /dev/shm/ftp_tmp/");

    int dirs;
    for(dirs=0;dirs<AUDIT_CSP_THREAD_NUM;dirs++) {
        memset(cmd,0,sizeof(cmd));
        sprintf(cmd,"mkdir -p /media/data/csp/%d/",dirs);
        system(cmd);

        memset(cmd,0,sizeof(cmd));
        sprintf(cmd,"mkdir -p  /media/data/csp_tmp/%d/",dirs);
        system(cmd);

        memset(cmd,0,sizeof(cmd));
        sprintf(cmd,"rm -rf  /media/data/csp_tmp/%d/*",dirs);
        system(cmd);


        memset(cmd,0,sizeof(cmd));
        sprintf(cmd,"rm -rf /media/data/csp/%d/*",dirs);
        system(cmd);
    }

    cachePolicy = (CACHE_POLICY_CONF * )get_audit_cache_policy_shm();
    if(!cachePolicy) {
        printf("cachePolicy shm get failed.\n");
        exit(0);
    }
	appIdConfig = (struct audit_monitor *)get_audit_shm(AUDIT_MONITOR ,0666);
	
	if(!appIdConfig){
        printf("audit_monitor shm get failed.\n");
        exit(0);
	}
//    memset(cachePolicy,0,sizeof(*cachePolicy));
    if(pthread_rwlock_init(&cachePolicy->audit_lock,&rwlock) != 0) {
        exit(0);
    }

    if(pthread_rwlock_init(&cachePolicy->alarm_lock,&rwlock)!=0) {
        exit(0);
    }
    //audit_policy_share_mem = create_audit_policy_shmem(&audit_policy_shmem_ret);//��������ڴ�
    //policy_mem = get_policy_share_cache();//�������

    for(i=0; i<THREADS_NUM+THREADS_ORACLE_NUM; i++) {
        audit_shm[i] = get_audit_shm_main(key[i]);//�������
        if(audit_shm[i] == NULL) exit(0);
        copy_count[i] =(struct copy_packet_counter *)audit_shm[i];
        audit_slot[i] =(struct audit_packet_slot *)(audit_shm[i] + sizeof(struct copy_packet_counter));
        printf("writed packet :%lu,read packet :%lu \n",copy_count[i]->write,copy_count[i]->read);
        copy_count[i]->read = copy_count[i]->write;
    }

    audit_info.pif =(struct audit_pack_info*) malloc(SSN_HASH_BUCKET_SIZE*sizeof(struct audit_pack_info));
    memset(&audit_info.phd,0,(THREADS_NUM+THREADS_ORACLE_NUM)*sizeof(struct audit_pack_info_head));
    memset(audit_info.pif,0,SSN_HASH_BUCKET_SIZE*sizeof(struct audit_pack_info));

    //init_Stream_str();
}

/*
Bool ifnewStream(struct audit_pack_info_head * hd){
if(bit_test(hd->hash,streamList.stat)){//hit
//if(hd->payload_len == 0)
return 0;//is old
}
else{//not hit
if(hd->payload_len != 0){
bit_set(hd->hash,streamList.stat);
return 1;//isnew
}
return 0;
}

}
*/
Bool isBaidu(const char * data) {
    char * start,*end;
    char host[200];

    if(memcmp(data,"GET  ",4)==0) {
        //printf("in isbidu GET  packet\n");
        //sleep(1);
        if((start = sunday_search(data,"Host: "))) {
            start += 6;
            end = sunday_search(start ,"\r\n");
            if(!end) return 0;
            memset(host,0,sizeof(host));
            memcpy(host,start,end-start);

            //sleep(1);
            if(sunday_search(host,"www.baidu.com")
                ||sunday_search(host,"news.baidu.com")
                ||sunday_search(host,"music.baidu.com")
                ||sunday_search(host,"map.baidu.com")
                ||sunday_search(host,"v.baidu.com")
                ||sunday_search(host,"image.baidu.com")) {
#if AUDIT_DEBUG
                printf("host:%s\n",host);
                printf("ISBAIDU............................\n");
#endif
                //sleep(4);
                return 1;
            }
        }
    }
    return 0;
}


Bool isGoogle(const char  * data) {
    char * start,*end;
    char host[200];
    if(memcmp(data,"GET ",4)==0){
    if((start = sunday_search(data,"Host:"))) {
        start += 6;
        end = sunday_search(start ,"\r\n");

        if(!end) return 0;
        memcpy(host,start,end-start);
        if(sunday_search(host,"google.com")){
#if AUDIT_DEBUG
            printf("IS GOOGLE----------------------\n");
#endif
            return 1;
        }
    }
    }
    return 0;
}

Bool isMail(const char * data) {
    char * start,*end;
    char host[200];
    if(memcmp(data, "GET ", 4) ==0
       || memcmp(data, "POST ", 5) == 0
       || memcmp(data, "OPTIONS ", 8) == 0) {
        if((start = sunday_search(data, "Host:"))) {
         start += 6;
         end = sunday_search(start ,"\r\n");
         if(!end) return 0;
         memcpy(host,start,end-start);
         if(sunday_search(host,"mail") ||sunday_search(host,"www.sdan.com.cn")){
             return 1;
         }
        }
    }
    return 0;
}

Bool isMsn(const char * data) {
    //char * start,*end;
    //char host[200];
    if(memcmp(data,"POST ",5)==0){
    if (sunday_search(data + 5, "USR") != NULL ||
    sunday_search(data + 5, "CHG") != NULL ||
    sunday_search(data + 5, "NLN") != NULL ||
    sunday_search(data + 5, "PNG") != NULL ||
    sunday_search(data + 5, "ANS") != NULL ||
    sunday_search(data + 5, "CAL") != NULL ||
    sunday_search(data + 5, "VER") != NULL ||
    sunday_search(data + 5, "XFR") != NULL )
    return 1;
    }
    return 0;
}

/* ��CSP��ҳ��������1������0 */
/*
Bool isCsp(const char *data, int hostNum) {
    char * start,*end;
    char host[200];
    int len;
    int i;

    if(memcmp(data,"GET ",4)==0 || memcmp(data,"POST ",5)==0 ) {
        if((start = sunday_search(data,"Host:"))) {
            start += 6;
            end = sunday_search(start, "\r\n");
            if(!end) return 0;
            len = end -start;
            if(len > 200) return 0;
            memset(host, 0, sizeof(host));
            memcpy(host, start, len);
            //printf("host : %s\n",host);
            for(i = 0;i < hostNum;i++) {
#if CSP_DEBUG
                //printf("host : %s\n",host);
                //printf("ip.host:%s\n",ips[i].host);
#endif
                if(strcmp(host,ips[i].host) == 0) {
#if AUDIT_DEBUG
                    printf("IS CSP----------------------\n");
#endif
                    if(strstr(ips[i].host, ":57772") || strstr(ips[i].host, ":8972")){
                        return 2;
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}
*/
u_int16_t get_app_id(struct audit_pack_info_head *hd,struct audit_monitor * conf){
	int i;
	for(i=0;i<100;i++){
		if(conf[i].dip == 0){
			break;
		}
		if(conf[i].dip == ntohl(hd->ser_ip) && conf[i].dport == hd->ser_port){
			return conf[i].app_id;
		}
	}
	return 0;
}
int  get_obj_id(int app_id){
	switch(app_id){
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 40:
		case 100:
		case 110:
		case 200:
		case 300:
		case 301:
			return 1;
		case 30:
		case 31:
			return 3;
		case 10:
			return 4;
		default :
			return 0;
	}
	return 0;
}


int get_audit_type_new(struct audit_pack_info_head *hd){
	switch(hd->app_id){
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
			return 2;
		case 40:
			return 4;
		case 100:
		case 110:
			return 1;
		case 200:
			return 3;
		case 300:
		case 301:
			return 5;
		default:
			return 100;			
	}
}

/* Ӧ�ò�����ʶ���ж�'data'������ͨ�����ַ�ʽ����cache���ݿ�İ� */
int get_audit_type(unsigned  char *data, struct audit_pack_info_head *hd) {
    if(hd->ser_port == 1972) {
        hd->app_id = APP_ENTERPRISE_CACHESTUDIO;
    } else if(hd->ser_port == 80 ||hd->ser_port == 57772 ||hd->ser_port == 8972 ) {
        hd->app_id =APP_WEB_HTTP;
    } else if(hd->ser_port == 23) {
        hd->app_id = APP_ENTERPRISE_TELNET;
    } else if(hd->ser_port == 21) {
        hd->app_id = APP_ENTERPRISE_FTP;
    } else if(hd->ser_port == 20) {
        hd->app_id = 126;
#if __NEW_20150721
    } else if (hd->ser_port == 1433) {
        hd->app_id = 1000;
#endif
    }
	else if(hd->ser_port == 1521){
		hd->app_id = 1001;
	}
	else {
        return -1;
    }

    switch(hd->app_id) {
        case APP_WEB_SINAWB:
            return AUDIT_WEIBO;
        case APP_WEB_HTTP:
            if(hd->payload_len == 0) return 0;
            if(hd->isin) return 0;
      //      if(isCsp(data, DbmHostNum) == 1) {return AUDIT_CACHE_CSP_HIS;}
       //     else if(isCsp(data,DbmHostNum) == 2){return AUDIT_CACHE_CSP_PORTAL;}
            else return 0;
        case APP_ENTERPRISE_CACHESTUDIO:
            return AUDIT_CACHESTUDIO;
        case APP_ENTERPRISE_TELNET:
            return 200;
        case APP_ENTERPRISE_FTP:
            return 300;
        case 126:
            return 301;
#if __NEW_20150721
        case 1000:
            return 1000;
#endif
		case 1001:
			return 1001;
        default:
            break;
    }
    return -1; //noneed
}

/* ��Ʋ���ƥ�� */
Bool set_policy_stat( unsigned char * data, struct audit_pack_info_head *pk_info_hd, struct audit_pack_info * pk_info) {
    int i;
    int appid;
    int flag;
    //printf("pk_info->audit_policy_stat = %02x",pk_info->audit_policy_stat);
    //printf("flag = %d , basic =  %d ,isnew =  %d,done = %d,audit_policy_stat = %d\n ",pk_info->flag,pk_info->basic,pk_info->isnew,pk_info->done,pk_info->audit_policy_stat);
#if AUDIT_DEBUG
    printf("%d\n%d\n",pk_info->hash_ip,pk_info_hd->int_ip);
#endif


    if(pk_info->audit_policy_stat == 0) {//δ���Ӳ��� ����  �ǵ�һ����
      
            /* ��Ʋ���ƥ�� */
			switch(pk_info->audit_type){
				case 1:
						for(i = 0; i < CSP_POLICY_NUM; i++) {
		                    if(pk_info_hd->policy->csp_audit_policy[i].valid_flag == 0) {
		                        continue;
		                    }
		                    pthread_rwlock_rdlock(&(pk_info_hd->policy->audit_lock));
		                    flag = audit_cache_policy_app_match(pk_info->audit_type, &pk_info_hd->policy->csp_audit_policy[i]);
		                    pthread_rwlock_unlock(&(pk_info_hd->policy->audit_lock));
		                    if(flag == 1) {
		                        break;
		                    }
	               		}
	                if(flag == 0) {
	                    pk_info->audit_policy_stat = NONEED;
	                } else{
	                    pk_info->audit_policy_stat = NEED;
	                }
					break;
				case 2:
				case 3:
				case 4:
				case 5:
						pthread_rwlock_rdlock(&(pk_info_hd->policy->audit_lock));
		                flag = audit_cache_policy_match(pk_info,pk_info->audit_type, pk_info_hd->cli_ip, pk_info_hd->timenow, NULL, pk_info_hd->policy, pk_info_hd->redisConn);
		                pthread_rwlock_unlock(&(pk_info_hd->policy->audit_lock));
		                if(flag == 0) {
		                    pk_info->audit_policy_stat = NONEED;
		                } else {
		                    pk_info->audit_policy_stat = NEED;
		                }
							break;
				default:
					break;			
			}
            //printf("hash:%u,audit_type : %d,policy_stat:%d,dport=%d,len =%d\n",pk_info_hd->hash,pk_info->audit_type,pk_info->audit_policy_stat,pk_info_hd->ser_port,pk_info_hd->payload_len);
        }
        return pk_info->audit_policy_stat;

}

int is_oracle_odbc(unsigned char * data,struct audit_pack_info_head *pk_info_hd,struct audit_pack_info * pk_info){

	//�ж�������Ƿ����� oracle odbc����
	if(!pk_info_hd->isin){
		if(data[4]==0x06){
			switch(data[10]){
				case 0x03:
				if(data[11]==0x2b || data[11]==0x47||data[11]==0x4a){
					return 31 ;//odbc
				}
				else if(data[11]==0x5e || data[11]==0x05 || data[11]==0x76){
					return 32;//sqlplus
				}
				else return 31;
			
				case 0x11:
					return 32;
				default:
					return 31;
			}
		}
		return ;
	}
	return ;
}

Bool set_policy_stat_2( unsigned char * data, struct audit_pack_info_head *pk_info_hd, struct audit_pack_info * pk_info){
	
	if(pk_info->audit_policy_stat == 0) {
		pk_info->audit_policy_stat = audit_dbmonitor_policy_match(pk_info_hd,pk_info,pk_info_hd->policy);
	}
	return pk_info->audit_policy_stat;

	int oracle_odbc_flag=0;



	if(pk_info->type_flag == 0){
		oracle_odbc_flag = is_oracle_odbc(data,pk_info_hd,pk_info);
		//oracle_odbc_flag -1:�ظ���  0:������������ 1:odbc����  2:��odbc����
		switch
			case 1:
				pk_info->type_flag = 31;
				break;
			case 2:
				pk_info->type_flag = 32;
				break;
			default:
				pk_info->type_flag = 0;
				break;
		}
	}
	
	switch(pk_info->type_flag){
		case 0:
			return INIT;
		case 31:
		case 32:
			if(pk_info->audit_policy_stat == 0) {
				pk_info->audit_policy_stat = audit_dbmonitor_policy_match(pk_info_hd,pk_info,pk_info_hd->policy);
			}
			break;
		default:
			break;
	}

	
	
	return pk_info->audit_policy_stat;
}



/* ������̫���㣬����㣬������Э��ͷ���֡��������Э��ͷ��һЩ��Ϣ���Լ�Ӧ�ò����ݡ�
   'slot'����������档
   'pk_info_hd'��'pk_info'����Э��ͷ��Ϣ��
   'data'����Ӧ�ò����ݡ� */
static char set_pack_info( struct audit_packet_slot *slot,struct audit_pack_info_head *pk_info_hd,struct audit_pack_info *pk_info ,unsigned char *data){
    u_int32_t time;
    struct ether_header *ethernet;
    struct iphdr * ip;
    struct tcphdr * tcp;
    struct udphdr * udp;
    //struct icmphdr * icmp;
    unsigned char * payload =NULL;
    int iplen=0,udplen = 0;
    int len = 0;
    u_int64_t long_max = (u_int64_t)-1;
    char * pk;
    pk = slot->packet_content;

    /* ��̫���� */
    ethernet = (struct ether_header  * )pk;
    u_int16_t eth_type = ntohs(ethernet->ether_type);
    if(eth_type !=0x0800 && eth_type !=0x8100) return 1;
    u_int16_t eth_len = 0;
    if (eth_type == 0x0800)
        eth_len = 14;
    else if (eth_type == 0x8100)
        eth_len = 18;

    /* IP��ʹ����(TCP/UDP) */
    ip = (struct iphdr *)(pk+eth_len);
    time = get_min();
    if(ip->protocol == UDP_PRO) {
        /* ������UDP */
        return  ip->protocol;

#if AUDIT_DEBUG
        printf("UDP-----------------------------UDP\n");
#endif
        /* UDPЭ��� */
        //return ip->protocol;
        udp =(struct udphdr *)((char *)ip+ip->ihl*4);
        udplen  = ntohs(udp->len);
        payload = ((unsigned char *)udp) + sizeof(struct udphdr);
        //iplen = ntohs(ip->tot_len);
        pk_info_hd->proto = UDP_PRO;
        len = udplen-sizeof(struct udphdr);
        if(len < 0 ||len > 1460 ) return 0;
        if(len >= 0) {
            /* ����Ӧ�ò����� */
            if(len > 0) memcpy(data, payload, len);

            /* TCPЭ��ͷ����UDPû�е� */
            //pk_info_hd->srcip = ip->saddr; //����˳��
            //pk_info_hd->desip = ip->daddr;//����˳��
            pk_info_hd->seq = 0;
            pk_info_hd->ack = 0;
            pk_info_hd->rst = 0;
            pk_info_hd->syn = 0;
            pk_info_hd->fin = 0;
            pk_info_hd->psh = 0;
            pk_info_hd->app_id = slot->app_id;
			
#if AUDIT_DEBUG
            printf("udp appid = %d\n",slot->app_id);
            printf("udp.sp=%u,udp.dp=%u\n",ntohs(udp->source),ntohs(udp->dest));
#endif

            /* ���ַ��򣬱���client/server��Э����Ϣ */
            if(ip->saddr == htonl(slot->sip)) {
                pk_info_hd->isin = 0;//out
                //pk_info_hd->sport = ntohs(udp->source);
                //pk_info_hd->dport = ntohs(udp->dest);
                pk_info_hd->cli_ip = ip->saddr;
                pk_info_hd->ser_ip =  ip->daddr;
                pk_info_hd->cli_port = ntohs(udp->source);
                pk_info_hd->ser_port = ntohs(udp->dest);
                memcpy(pk_info_hd->cli_mac , ethernet->ether_shost,sizeof(ethernet->ether_shost));
                memcpy(pk_info_hd->ser_mac ,ethernet->ether_dhost,sizeof(ethernet->ether_dhost));
            } else {
                pk_info_hd->isin = 1;//inle
                pk_info_hd->cli_ip = ip->daddr;
                pk_info_hd->ser_ip =  ip->saddr;
                pk_info_hd->cli_port = ntohs(udp->dest);
                pk_info_hd->ser_port = ntohs(udp->source);
                memcpy(pk_info_hd->cli_mac , ethernet->ether_dhost,sizeof(ethernet->ether_dhost));
                memcpy(pk_info_hd->ser_mac ,ethernet->ether_shost,sizeof(ethernet->ether_shost));
            }
			pk_info_hd->app_id = get_app_id(pk_info_hd,appIdConfig);
			pk_info->audit_type = get_audit_type_new(pk_info_hd);
//printf("server_port : %hs,client_port : %hs\n",pk_info_hd->ser_port,pk_info_hd->cli_port);
        }
    } else  if(ip->protocol ==  TCP_PRO) {
#if AUDIT_DEBUG
        printf("TCP-----------------------------TCP\n");
#endif
        /* TCPЭ��� */
        tcp =(struct tcphdr *)((char *)ip+ip->ihl*4);
        payload = (unsigned char *)tcp + tcp->doff*4;
        iplen = ntohs(ip->tot_len);
        pk_info_hd->proto = TCP_PRO;
        len = iplen-ip->ihl*4-tcp->doff*4;
        if(len < 0 ||len > 1460) return 0;
        if(len >= 0) {
            /* ����Ӧ�ò����� */
            if(len > 0) memcpy(data,payload,len);

            /* TCPЭ��ͷ��������TCP���� */
            //pk_info_hd->srcip = ip->saddr;
            //pk_info_hd->desip = ip->daddr;
            pk_info_hd->seq = ntohl(tcp->seq);
            pk_info_hd->ack = tcp->ack;
            pk_info_hd->rst = tcp->rst;
            pk_info_hd->syn = tcp->syn;
            pk_info_hd->fin = tcp->fin;
            pk_info_hd->psh = tcp->psh;
            pk_info_hd->app_id = slot->app_id;
            //pk_info_hd->sport = ntohs(tcp->source);
            //pk_info_hd->dport = ntohs(tcp->dest);
#if AUDIT_DEBUG
            printf("appid ====%d\n",slot->app_id);
            printf("tcp.sp=%u,tcp.dp=%u\n",ntohs(tcp->source),ntohs(tcp->dest));
#endif

            //printf("config mac :%02x%02x%02x%02x%02x%02x\n",ethernet->ether_dhost[0],ethernet->ether_dhost[1],ethernet->ether_dhost[2],ethernet->ether_dhost[3],ethernet->ether_dhost[4],ethernet->ether_dhost[5]);
            //if(1){
            //if(memcmp(ethernet->ether_dhost,sysParameter->uplink_mac1,6)==0){
            //if(ntohs(tcp->dest) == 57772||ntohs(tcp->dest) == 80 ||ntohs(tcp->dest)  == 23 ||ntohs(tcp->dest)  == 1972){

            /* ���ַ��򣬱���client/server��Э����Ϣ */
            if(ip->saddr == htonl(slot->sip)) {
                pk_info_hd->isin = 0;//out
                pk_info_hd->cli_ip = ip->saddr;
                pk_info_hd->ser_ip =  ip->daddr;
                pk_info_hd->cli_port = ntohs(tcp->source);
                pk_info_hd->ser_port = ntohs(tcp->dest);
                memcpy(pk_info_hd->cli_mac , ethernet->ether_shost,sizeof(ethernet->ether_shost));
                memcpy(pk_info_hd->ser_mac ,ethernet->ether_dhost,sizeof(ethernet->ether_dhost));
#if 0
                char smac[20]={0};
                char dmac[20]={0};
                sprintf(smac, "smac:%02x-%02x-%02x-%02x-%02x-%02x",pk_info_hd->cli_mac[0],pk_info_hd->cli_mac[1],pk_info_hd->cli_mac[2],pk_info_hd->cli_mac[3],pk_info_hd->cli_mac[4],pk_info_hd->cli_mac[5]);
                sprintf(dmac, "dmac:%02x-%02x-%02x-%02x-%02x-%02x",pk_info_hd->ser_mac[0],pk_info_hd->ser_mac[1],pk_info_hd->ser_mac[2],pk_info_hd->ser_mac[3],pk_info_hd->ser_mac[4],pk_info_hd->ser_mac[5]);
                printf("sport = %u,smac = %s,sip = %lu\n",pk_info_hd->cli_port,smac,pk_info_hd->cli_ip);
                printf("dport = %u,dmac = %s,dip = %lu\n",pk_info_hd->ser_port,dmac,pk_info_hd->ser_ip);
#endif
            } else {
                pk_info_hd->isin = 1;//inle
                pk_info_hd->cli_ip = ip->daddr;
                pk_info_hd->ser_ip =  ip->saddr;
                pk_info_hd->cli_port = ntohs(tcp->dest);
                pk_info_hd->ser_port = ntohs(tcp->source);
                memcpy(pk_info_hd->cli_mac , ethernet->ether_dhost,sizeof(ethernet->ether_dhost));
                memcpy(pk_info_hd->ser_mac ,ethernet->ether_shost,sizeof(ethernet->ether_shost));
            }
				
				pk_info_hd->app_id = get_app_id(pk_info_hd,appIdConfig);
				pk_info_hd->obj_id = get_obj_id(pk_info_hd->app_id);
				pk_info->audit_type = get_audit_type_new(pk_info_hd);
	//			printf("app_id :%d\n",pk_info_hd->app_id);
//printf("server_port : %u,client_port : %u\n",pk_info_hd->ser_port,pk_info_hd->cli_port);
        }
    }
	else {return 0;}

#if CSP_DEBUG
    printf("client ip = %lu : %u,server ip = %lu : %u,len = %d\n",pk_info_hd->cli_ip,pk_info_hd->cli_port,pk_info_hd->ser_ip,pk_info_hd->ser_port,len);
#endif
    //printf("appid = %d\n",slot->app_id);
    pk_info_hd ->int_ip = htonl(slot->sip);
    //printf("int_ip%lu\n",pk_info_hd ->int_ip);
    pk_info_hd->hash = slot->hash;
    pk_info_hd->user_id = slot->user_id;
    //pk_info_hd->srcip = ip->saddr; //����˳��
    //pk_info_hd->desip = ip->daddr;//����˳��
    pk_info_hd->payload_len  = len;
#if AUDIT_DEBUG
    printf("userid = %d\n",pk_info_hd->user_id );
#endif
#if AUDIT_DEBUG
    printf("len = %d\n",pk_info_hd->payload_len);
    printf("srcip = %s\n",inet_ntoa(*(struct in_addr *)&pk_info_hd->srcip));
    printf("desip = %s\n",inet_ntoa(*(struct in_addr *)&pk_info_hd->desip));
#endif
    pk_info_hd->timenow = time;
#if REL_HBASE
    pk_info_hd->audit_time = get_usec_time();
   // pk_info_hd->audit_time = (long_max/2) - pk_info_hd->audit_time;
#endif
    pk_info_hd->policy = cachePolicy;
    //pk_info_hd->audit_type = 0;
#if AUDIT_DEBUG
    printf("audit_policy_stat = %d\n",pk_info->audit_policy_stat);
#endif
#if CSP_DEBUG
    CspDebug( pk_info_hd,pk_info,data);
    /*if(pk_info_hd->isin && memcmp(data,"HTTP/1.",7)==0) {                                                                                                                                        //pk_info_hd->cli_ip,pk_info_hd->cli_port,pk_info_hd->ser_ip,pk_info_hd->ser_port
        printf("before get_audit_type  match Http  policy_stat = %d, hash = %lu,audit_type = %d cli = %s:%u ser = %s:%u \n",pk_info->audit_policy_stat  ,pk_info_hd->hash,pk_info->audit_type,inet_ntoa(*(struct in_addr *)& pk_info_hd->cli_ip),pk_info_hd->cli_port,inet_ntoa(*(struct in_addr *)& pk_info_hd->ser_ip),pk_info_hd->ser_port);
    } else if(pk_info_hd->isin==0 && memcmp(data,"POST ",5)==0) {
        printf("before get_audit_type  match Post  policy_stat = %d, hash = %lu,audit_type = %d sip = %s:%u dip = %s:%u \n",pk_info->audit_policy_stat  ,pk_info_hd->hash,pk_info->audit_type,inet_ntoa(*(struct in_addr *)& pk_info_hd->cli_ip),pk_info_hd->cli_port,inet_ntoa(*(struct in_addr *)& pk_info_hd->ser_ip),pk_info_hd->ser_port);
    } else if(pk_info_hd->isin==0 && memcmp(data,"GET ",4)==0) {
        printf("before policy  match Get  policy_stat = %d, hash = %lu,audit_type = %d sip = %s:%u dip = %s:%u \n",pk_info->audit_policy_stat  ,pk_info_hd->hash,pk_info->audit_type,inet_ntoa(*(struct in_addr *)& pk_info_hd->cli_ip),pk_info_hd->cli_port,inet_ntoa(*(struct in_addr *)& pk_info_hd->ser_ip),pk_info_hd->ser_port);
    } */
#endif
    /*printf("isin = %d",pk_info_hd->isin);
    if(memcmp(data,"HTTP/1.",7) == 0 || memcmp(data,"POST ",5)==0 || memcmp(data,"GET ",4)==0){
        printf("%s\n",data);
    }*/

    /*if(pk_info->audit_type == 0){
        pk_info->audit_type = get_audit_type((const char *)data,pk_info_hd);
    }*/

    //printf("pk_info_hd->audit_type = %d\n",pk_info_hd->audit_type);

    return ip->protocol;
}

/* �����ַ���'name'�ӵ�3���ַ���ʼ�Ĳ��֣����������� */
u_int64_t  AuditGetId(char *name) {
    char *pos = NULL;
    u_int32_t offset;
    return strtoul(name+2, NULL, 10);
}

/* �ƶ��ļ������ļ������䡣
   ���ļ���ʼ·����'srcpath''dir'/
   Ŀ��·����'despath''dir'/
   �ļ���Ϊ��"Q_"/"P_"/""Ϊǰ׺�����'name' */
void AuditFileRename(char srcpath[], char despath[], u_int64_t name, int flag, int dir) {
    char rdyFileName[AUDIT_PATH_LEN]={0};
    char filename[AUDIT_PATH_LEN]={0};
    char newfilename[AUDIT_PATH_LEN]={0};
    char * pos;
    u_int16_t offset;

    if(flag == HTTP_REQUEST) {
        sprintf(filename, "%s%d/Q_%lu", srcpath, dir, name);
        sprintf(newfilename, "%s%d/Q_%lu", despath, dir, name);
    } else if(flag == HTTP_RESPONSE) {
        sprintf(filename, "%s%d/P_%lu", srcpath, dir, name);
        sprintf(newfilename, "%s%d/P_%lu", despath, dir, name);
    } else {
        sprintf(filename, "%s%d/%lu", srcpath, dir, name);
        sprintf(newfilename, "%s%d/%lu", despath, dir, name);
    }
    //printf("mv %s %s \n", filename, newfilename);
    rename(filename, newfilename);
}

/* ���·��'path'n(n=0...AUDIT_CSP_THREAD_NUM-1)�µ��ļ�����Щ�ļ�������Ӧ-
   (REQUEST��REPONSE)������һ���ļ��Ĵ���ʱ�䳬��'times'�룬�ͰѸ��ļ�������
   ��Ӧ�ļ��ƶ���'despath'·���� */
void AuditCheckDirFiles(char path[], char despath[], u_int16_t times, u_int32_t list[]) {
    struct dirent *dp;
    time_t timenow,filetime;
    char filepath[AUDIT_PATH_LEN];
    char filename[AUDIT_PATH_LEN];
    u_int64_t id,hash;
    char * pos;
    u_int16_t offset = 0;
    int i = 0;
    int dir = 0;

    for(dir = 0; dir < AUDIT_CSP_THREAD_NUM; dir++) {
        memset(filepath, 0, AUDIT_PATH_LEN);
        sprintf(filepath, "%s%d/", path, dir);
        //printf("%s\n"filepath);
        checkdir = opendir(filepath);
        if(checkdir == NULL) return;
        while((dp=readdir(checkdir)) != NULL) {
            if(strcmp("..",dp->d_name)==0||strcmp(".",dp->d_name)==0) {
                continue;
            } else {
                if(dp->d_name[0] == 'Q') {
                    i = HTTP_REQUEST;
                }
                else if(dp->d_name[0] == 'P') {
                    i = HTTP_RESPONSE;
                }
                memset(filename, 0, sizeof(filename));
                sprintf(filename, "%s%d/%s", path, dir, dp->d_name);
                filetime = _get_file_time(filename);
                time(&timenow);
                if(filetime == -1) {
                    continue;
                    perror("filetime:");
                } else if(timenow -  filetime  > times) {
                    id = AuditGetId(dp->d_name);
                    //hash = list[id%AUDIT_INDEX_NUM];
                    //list[id%AUDIT_INDEX_NUM] = 0;
                    AuditFileRename(path, despath, id, HTTP_REQUEST, dir);
                    AuditFileRename(path, despath, id, HTTP_RESPONSE, dir);
                }
            }
        }
        closedir(checkdir);
        checkdir = NULL;
    }
}

/* Ӧ�ò����ݴ����������ͷ������������Ĵ������� */
u_int32_t audit_handle(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char *payload) {
    //int index;
    int ret=0;
    char sermac[AUDIT_MAC_LEN] = {0};
    char climac[AUDIT_MAC_LEN] = {0};

    //index = p_info_hd->hash%AUDIT_INDEX_NUM;
    //printf("index = %d\n",index);
#if 0
    printf("in audit_handle  audittype = %d\n",p_info->audit_type);
#endif
#if CSP_DEBUG
    CspDebug( p_info_hd,p_info,payload);
#endif


//printf("app_id = %d\n",p_info_hd->app_id);
    switch(p_info_hd->app_id){
        case 300:
			if(p_info_hd->thrid == 0)
            	ret = audit_ftp(p_info_hd,p_info, payload);
            break;
        case 301:
			if(p_info_hd->thrid == 0)
            	ret = audit_ftp_file(p_info_hd,p_info, payload);
            break;
        case 200:
			if(p_info_hd->thrid == 0){
				printf("before telnet\n");
				ret = audit_telnet(p_info_hd,p_info, payload);
				printf("after telnet\n");		
			}
            break;
        case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
        //    ret = audit_studio(p_info_hd,p_info, payload);
            break;
		case 40:
        case 100:
        case 110:
          //  ret = CspPackRefrom(p_info_hd,p_info, payload);
            break;
		case 30:
		case 31:
			if(p_info_hd->thrid > 0)
				ret = audit_oracle(p_info_hd, p_info, payload);
        default:
            return 0;
    }
    return ret;
}

/* �߳̾������, ���߳�����ÿ10s����һ���ļ���'AUDIT_TMP_CSP_PATH'n(n=0...9)
   �µ��ļ�����ֹ�ļ���ѹ�����崦����ʽ��'AuditCheckDirFiles()'������ע�� */
void *auditcheckfp_callback(void *_param) {
    while(1){
        sleep(10);
        //printf("checkfp thread..\n");
        //continue;
        //AuditCheckDirFiles(AUDIT_TMP_BBS_PATH,AUDIT_BBS_CONTENT_PATH,AUDIT_TIMEOUT,bbs_id_hash);
        //AuditCheckDirFiles(AUDIT_TMP_MAIL_PATH,AUDIT_MAIL_CONTENT_PATH,AUDIT_TIMEOUT,mail_id_hash);
        //AuditCheckDirFiles(AUDIT_TMP_WEBMAIL_PATH,AUDIT_WEBMAIL_CONTENT_PATH,AUDIT_TIMEOUT,mail_id_hash);
        AuditCheckDirFiles(AUDIT_TMP_CSP_PATH, AUDIT_CSP_CONTENT_PATH, 60, csp_id_hash);
    }
}

/* ���߳�IDΪ'pthId'���̣߳��󶨵�'pthNum'ָ����CPU */
void audit_set_cpu_affinity_thrd(pthread_t pthId, int pthNum ) {
    cpu_set_t cpuMask;
    CPU_ZERO(&cpuMask);
    CPU_SET(pthNum,&cpuMask);
    if(pthread_setaffinity_np(pthId, sizeof(cpuMask), &cpuMask) < 0) {
        fprintf(stderr, "set thread %d to cpucore %d affinity failed\n",pthNum,pthNum+6);
    }
}

/* �߳̾������, ���߳����ڴ���Ӧ�ò����ݡ�
   'param'���ڴ����̱߳�� */
void *audit_dbm_packet_reform_thrd_fun(void *param) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //�����˳��߳�
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //��������ȡ��

    int pthNum; //�̱߳�ʶ
    //I32 testPrivateVal [2]= {0}; //����˽������
    char proto; //����Э�鷵��ֵ
    int policy_stat=0;
    char p;
    char policy_audit_flag;
    unsigned char payload[MAX_PACKET_LEN];
    struct audit_pack_info_head  * pkt_info_head;
    struct audit_pack_info * pkt_info;
    struct audit_packet_slot * p_slot;

    //���ݲ����ж����߳������Ŀ��ڴ�
    pthNum =*( (int*)param);
    //if(pthNum > 0) return ;
    printf("pthNum = %d. ",pthNum);
    //�󶨵�ǰ�̵߳�cpu��
    //audit_set_cpu_affinity_thrd(pthread_self(),pthNum);

//    redis_conn[pthNum] = redisConnect(REDISSERVERHOST,REDISSERVERPORT);

    while(1){
        usleep(10);
#if 1
        /* ֻ�̱߳��=0���̸߳�����Ʋ��Ե�״̬�޸�
           �����߳�ֻ�е���Ʋ���״̬=1ʱ������
           û����Ʋ���ʱ������������ֻ�е���1����1��������Ʋ��ԣ��Ż�������*/
        if(pthNum == 0){
            pthread_rwlock_rdlock(&cachePolicy->audit_lock);
            if(cachePolicy->csp_audit_policy_visit_flag == 0) {
                pthread_rwlock_unlock(&cachePolicy->audit_lock);
                continue;
            }
            pthread_rwlock_unlock(&cachePolicy->audit_lock);

            pthread_rwlock_rdlock(&cachePolicy->audit_lock);
            if(cachePolicy->csp_audit_policy_visit_flag == 2) {
                pthread_rwlock_unlock(&cachePolicy->audit_lock);
                pthread_rwlock_wrlock(&cachePolicy->audit_lock);
                if(cachePolicy->csp_audit_policy_visit_flag == 2){
                    cachePolicy->csp_audit_policy_visit_flag = 1;
                    //[pthNum] ->read = copy_count[pthNum] ->write;
                    memset(audit_info.pif, 0, SSN_HASH_BUCKET_SIZE*sizeof(struct audit_pack_info));
                }
            }
            pthread_rwlock_unlock(&cachePolicy->audit_lock);
        } else {
            pthread_rwlock_rdlock(&cachePolicy->audit_lock);
            if(cachePolicy->csp_audit_policy_visit_flag != 1) {
                pthread_rwlock_unlock(&cachePolicy->audit_lock);
                continue;
            }
            pthread_rwlock_unlock(&cachePolicy->audit_lock);
        }

        /* �ϴ��� - �����̶߳������޸���Ʋ��Ե�״̬������������ȴ�ʱ�����
           �Ӷ����°��Ļ�ѹ */
      /*pthread_rwlock_rdlock(&cachePolicy->audit_lock);
        if(cachePolicy->csp_audit_policy_visit_flag == 0){
            pthread_rwlock_unlock(&cachePolicy->audit_lock);
            continue;
        }
        pthread_rwlock_unlock(&cachePolicy->audit_lock);

        pthread_rwlock_rdlock(&cachePolicy->audit_lock);
        if(cachePolicy->csp_audit_policy_visit_flag == 2){
            pthread_rwlock_unlock(&cachePolicy->audit_lock);

            pthread_rwlock_wrlock(&cachePolicy->audit_lock);
            if(cachePolicy->csp_audit_policy_visit_flag == 2){
                cachePolicy->csp_audit_policy_visit_flag = 1;
                copy_count[pthNum] ->read = copy_count[pthNum] ->write;
                memset(audit_info.pif,0,SSN_HASH_BUCKET_SIZE*sizeof(struct audit_pack_info));
            }
        }
        pthread_rwlock_unlock(&cachePolicy->audit_lock); */
#endif
        /* �жϰ���������״̬����2������������յĺ����� */
        if(copy_count[pthNum]->write == copy_count[pthNum] ->read ){
            continue;
        }
        if(copy_count[pthNum]->write - copy_count[pthNum]->read >= AUDIT_PACKET_CACHE){
            printf("cache full.\n");
        }

        /* ��ֹ�ڳ������й����жϿ�redis������ */
		/*
        if(redis_conn[pthNum] == NULL) {
            redis_conn[pthNum] = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
        }
        if(redis_conn[pthNum]->err) {
            redisFree(redis_conn[pthNum]);
            redis_conn[pthNum] = NULL;
            redis_conn[pthNum] = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
            if(redis_conn[pthNum]->err){
                 redisFree(redis_conn[pthNum]);
                 redis_conn[pthNum] = NULL;
                 printf("can not connect  to redis server.\n");
                 continue;
            }
        }
*/
        /* ��õ�ǰ�̵߳İ���������һ������������TCP/IPЭ��ͷ
           ע�� - ץ���ĵط����Ѿ��Ѱ�����ֳ���ÿ���߳�һ�飬����ֱ�ӷ��ʱ��߳�
                  ��Ӧ���ǿ黺���OK�ˡ�*/
        p_slot = audit_slot[pthNum] + copy_count[pthNum]->read%AUDIT_PACKET_CACHE;
        pkt_info_head =&audit_info.phd[pthNum];
        pkt_info = &audit_info.pif[p_slot->hash];
		
		//������ֵ
	/*	if(p_slot->direction){
			memset(pkt_info,0,sizeof(*pkt_info));
			pkt_info->isnew = 1;
		}
		*/
		pkt_info_head->thrid = pthNum;

        memset(payload, 0, sizeof(payload));
        p = set_pack_info(p_slot, pkt_info_head, pkt_info, payload);
		
     //   pkt_info_head->redisConn = redis_conn[pthNum];

        /* ֻ����TCP��UDP�İ� */
        if( p != 0x06 &&p!= 0x11){
            copy_count[pthNum]->read++;
            continue;
        }

        /* ��������״̬���������ͻ��� */
#if CSP_DEBUG
        printf("before streamStat-----------------------\n");
        CspDebug(pkt_info_head, pkt_info, payload);
#endif

        streamStat(pkt_info_head, pkt_info, payload);

#if CSP_DEBUG
        CspDebug( pkt_info_head,pkt_info,payload);
        printf("\nafter streamStat------------------------");
#endif

        //printf("hash:%u,audit_type : %d,policy_stat:%d,dport=%d,len =%d\n",pkt_info_head->hash,pkt_info->audit_type,pkt_info->audit_policy_stat,pkt_info_head->ser_port,pkt_info_head->payload_len);

		

		//if(pkt_info->audit_type == 3||pkt_info->audit_type == 5){
		if(pthNum == 0){
			set_policy_stat(payload,pkt_info_head,pkt_info);
			if(pkt_info->audit_policy_stat != NEED){
	            copy_count[pthNum]->read++;
	            continue;
     	  	}
		}else{
		//	set_policy_stat_2(payload,pkt_info_head,pkt_info);
			pkt_info->audit_policy_stat = NEED;
//			pkt_info->audit_policy_stat = audit_dbmonitor_policy_ip_match(pkt_info_head->cli_ip,pkt_info_head->timenow,pkt_info_head->policy);
			if(pkt_info->audit_policy_stat != NEED){
	            copy_count[pthNum]->read++;
	            continue;
     	  	}

		}
        /*if(pkt_info_head->ser_port == 21 || pkt_info_head->ser_port == 20){
            pkt_info->audit_policy_stat = NEED;
        } */

#if AUDIT_DEBUG
        printf("*********************************************************************\n");
        printf("copy_count ->read = %lu, copy_count->write = %lu\n",copy_count ->read,copy_count->write);
        printf("hash  = %d \n",p_slot->hash);
        printf("audit_type = %d\n",pkt_info->audit_type);
        printf("app_id = %u\n",pkt_info_head->app_id);
        printf("cliip = %s\n",inet_ntoa(*(struct in_addr *)&pkt_info_head->cli_ip));
        printf("time: %d\n",pkt_info_head->timenow );
        printf("plocy_stat = %d \n",pkt_info->audit_policy_stat);
        printf("isin = %d\n",pkt_info_head->isin);
#endif


#if AUDIT_DEBUG
        if(pkt_info_head->isin){
            printf("p_info->in_seq = %lu\n",pkt_info->in_seq);
        } else{
            printf("p_info->out_seq = %lu\n",pkt_info->out_seq);
        }
#endif

        /* ����Ӧ�ò����� */
        audit_handle(pkt_info_head,pkt_info,payload);
        copy_count[pthNum]->read++;
        //printf("*********************************************************************\n");
        //sleep(6);
    }
}

/* �źŴ������� */
void sig_fun(int sig) {
    char time[50]={0};
    //if(checkdir) closedir(checkdir);
    //pthread_mutex_lock(&csp_id_mutex);
  //  SetId(AUDIT_CSP_ID_PATH, csp_id);
    //pthread_mutex_unlock(&csp_id_mutex);

    pthread_cancel(auditcheckfp_thread);
    pthread_join(auditcheckfp_thread,NULL);
    shmdt((const void *)audit_shm[0]);
    shmdt((const void *)audit_shm[1]);
    shmdt((const void *)audit_shm[2]);
    shmdt((const void *)audit_shm[4]);
    shmdt((const void *)audit_shm[5]);
    shmdt((const void *)audit_shm[6]);
    shmdt((const void *)audit_shm[7]);
    shmdt((const void *)audit_shm[8]);
    shmdt((const void *)audit_shm[9]);
    shmdt((const void *)cachePolicy);
    //shmdt((const void *)policy_mem);
    //shmctl(audit_policy_shmem_ret,IPC_RMID,0);
    //printf("ret = %d\n",audit_policy_shmem_ret);

    /*if(audit_info.pif){
    free(audit_info.pif);
    audit_info.pif = NULL;
    }*/

    /* SIGINT�������ֶ��رս��̡�
       �����������ǽ��̳���Խ����ʵȵ��쳣�ź� */
    switch(sig){
        case SIGINT:
            exit(0);
        case SIGTERM:
        case SIGSEGV:
            get_audit_time(time);
            audit_error_log = fopen("/usr/inp/cfg/audit_error_log","a+");
            if(audit_error_log) {
                fprintf(audit_error_log,"%s audit_process get sig %d\n",time,sig);
                fflush(audit_error_log);
                fclose(audit_error_log);
            }
            exit(0);
        default:
            exit(0);
    }
}

int readIP(char *fname, unsigned int A[], int maxsize) {
    FILE *fp;
    unsigned long a1,a2,a3,a4, i;
    fp = fopen(fname, "a+");
    if (!fp) {
        printf("fopen failed in readIP()\n");
        return 0;
    }
    i = 0;
    while (fscanf(fp, "%u.%u.%u.%u", &a1, &a2, &a3, &a4) != EOF) {
        A[i++] = a1*256*256*256 + a2*256*256 + a3*256 + a4;
        printf("%u,%u,%u,%u, =%u\n", a1, a2, a3, a4, A[i-1]);
    }
    fclose(fp);
    return i;
}


/* ������ - audit_procesִ���ļ� */
int main(int argc, char **argv) {
#if CSP_RELEASE_DEBUG
    if(0)
#endif
    if(argc== 1)
   	 NC_daemon_audit();
	
    int param[10]={0,1,2,3,4,5,6,7,8,9};
    int i;
	computer_conts = 0;


    audit_init();
    printf("after initing ...\n");


	computer_conts = readIP("/usr/inp/cfg/telnet_remote_computer.cfg",remote_computer_ip,10);

/*
    if (pthread_create(&auditcheckfp_thread, NULL, &auditcheckfp_callback,NULL)!= 0){
        printf("error auditcheckfp_callback setup\n");
        kill(0,SIGTERM);
    }
    */
    for(i = 0; i < THREADS_ORACLE_NUM+THREADS_NUM; i++) {
        if (pthread_create(&audit_dbm_t[i], NULL, &audit_dbm_packet_reform_thrd_fun, (void *)(&param[i]))!= 0) {
            printf("error audit_dbm_packet_reform_thrd_fun setup\n");
            kill(0,SIGTERM);
        }
    }

    signal(SIGINT, sig_fun);
    signal(SIGTERM, sig_fun);
    signal(SIGSEGV, sig_fun);

    //system("/usr/inp/bin/audit_policy_reload &");
    //system("/usr/inp/bin/audit_csp_release");
    //system("/usr/inp/bin/audit_sql_insert");

    /* ÿ��1���ӣ��޸Ĺ�����ʱ���ַ��� */
    while(1){
        pthread_rwlock_wrlock(&tablesTime_lock);
        memset(tablesTime, 0, sizeof(tablesTime));
        get_audit_time_3(tablesTime);
        pthread_rwlock_unlock(&tablesTime_lock);
        sleep(60);
    }
}

