/*
	update 2014-12-10 ��������   AuditWrite()����

*/

#ifndef _AUDIT_API_H
#define _AUDIT_API_H




#include <stdlib.h>
#include <string.h>


#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <datrie/trie.h>
#include <stdio.h>
#include <dlfcn.h>
#include <time.h>
#include <net/ethernet.h>
//#include "typedef.h"
#include "../include/app_static.h"

#include "op_db.h"
#include "../include/inp.h"
//#include "qq.h"
#include "audit_release.h"
#include "sunday.h"
#include "audit_time_api.h"
#include "csp_redis.h"
#include "csp_policy.h" 


#include <sched.h>

#define SHIFT 3
#define MASK  0x7
#define MAX_FILE_FD 65536

typedef u_int8_t Bool;
#define THREADS_NUM		1
#define THREADS_ORACLE_NUM	5

#define INIT 0
#define NEED 1
#define NONEED 2
#define MAX_AUDIT_POLICY 1000	

struct ether_net{
	u_int8_t dmac[6];
	u_int8_t smac[6];
	u_int16_t type;
};

struct audit_pack_info
{
	int audit_type;
	u_int16_t app_id;
	Bool  audit_policy_stat:2,flag:3,basic:1,isnew:1,done:1;
	u_int8_t wflag;
	u_int16_t policy_id;
	int type_flag;
	u_int32_t file_num;
	u_int32_t in_seq;
	u_int32_t out_seq;
	u_int32_t hash_cliip;
	u_int32_t hash_serip;
	u_int16_t hash_clipt;
	u_int16_t hash_serpt;
	u_int16_t hash_proto;
	u_int16_t security_level;
//	u_int16_t in_len;
//	u_int16_t out_len;
//	void * st;
//	u_int32_t writtenbyes;
#if AUDIT_DEBUG
	u_int32_t cont;
	u_int16_t len;
#endif
//	u_int8_t dir;
	u_int64_t nanotime;
	off_t in_offset;
	off_t out_offset;
//	u_int64_t id;
};


struct audit_pack_info_head{
	Bool  isin;
//	Bool  isnew;
	int thrid;
	CACHE_POLICY_CONF *policy;
	redisContext * redisConn;
	u_int8_t proto; /*0x06 tcp 0x17 udp*/
	int audit_type;
	int obj_id;
	u_int8_t ack:1,rst:1,syn:1,fin:1,psh:1;
	u_int16_t app_id;
	u_int32_t hash;
	u_int32_t user_id;
	u_int32_t seq;
	u_int16_t payload_len;
	u_int32_t timenow;
	u_int64_t audit_time;
	u_int8_t  cli_mac[6]; //client mac
	u_int8_t ser_mac[6];//server mac
	u_int32_t int_ip;	//inside ip
	u_int32_t cli_ip;//client ip
	u_int32_t ser_ip;//server ip
	u_int16_t cli_port;//client port
	u_int16_t ser_port;//server port
};

struct audit_info_st{
	struct audit_pack_info *pif;
	struct audit_pack_info_head phd[THREADS_ORACLE_NUM];	
};

/*
struct stream_stat_list{
	u_int8_t *stat;
};
*/

//struct stream_stat_list streamList;




enum{
	AUDIT_WEBPOST = 1,
	AUDIT_WEBMAIL,
	AUDIT_POP3,
	AUDIT_SMTP,
	AUDIT_MSN,	/*original MSN message*/
//	AUDIT_YIM1,   /*original yahoo message*/
//	AUDIT_MSN2,	/*MSN message using port 80*/
	AUDIT_FETION,
//	AUDIT_YIM2,	/*yahoo message using http*/
	AUDIT_TELNET,
	AUDIT_FTP,
	AUDIT_BAIDU,
	AUDIT_GOOGLE,
	AUDIT_QQ,
	AUDIT_GTALK,
	AUDIT_WEIBO,/*13*/
	AUDIT_CACHE_CSP_HIS,
	AUDIT_CACHE_CSP_PORTAL,
	AUDIT_CACHESTUDIO,
	AUDIT_FTP_DATA //17
};

/*
typedef struct
{
	PRUint32 ip;
	PRInt32 user_id;
	PRUint64 id:59, pflag:2, basic:1, isnew:1, k:1;		
	PRUint32 in_seq;
	PRUint32 out_seq;
	PRUint16 pid;
	PRUint16 length:11, type:4, done:1; 
	PRUint32 cont;
	PRUint8 signal:4, flag:4;
	PRUint8 urg:1,ack:1,psh:1,rst:1,syn:1,fin:1,rsv:2;
	char *indata;
	char *outdata;
}Taudit_pro_cell;

typedef struct
{
	Taudit_pro_cell audit_pro_cells[SSN_HASH_BUCKET_SIZE/2];
}Tshare_audit_pro;
*/
#define SHARE_CACHE_AUDIT_PRO "/dev/shm/fp_share_audit_pro"

//Tshare_audit_pro *share_audit_pro;

#define	AUDIT_URL_LEN					5000
#define	AUDIT_TIME_LEN					40
#define	AUDIT_USERNAME_LEN			40
#define	AUDIT_SUBJECT_LEN				3000
#define	AUDIT_TEXT_LEN					100000
#define   AUDIT_MAC_LEN                                20

#define STACK_MAX_NUM					50
#define AUDIT_TELCMD_LEN				100

#define AUDIT_BOUNDARY_LEN	100
#define AUDIT_FP_INDEX				65535
#define	AUDIT_ACCOUNT_LEN				40
#define	AUDIT_FILENAME_LEN			300						
#define	AUDIT_ACCOUNT_NUM				5
#define	AUDIT_ID						0xffffffffffffffLL
#define	AUDIT_PACK_LEN			1500
#define	AUDIT_INDEX_NUM						65536
#define AUDIT_PATH_LEN			300
#define AUDIT_PATH					"/data/mail_attach/"
#define AUDIT_TEMP_POST_PATH			"/data/audit/post_temp/"
#define AUDIT_TEMP_MAIL_PATH			"/data/audit/mail_temp/"
#define AUDIT_MAIL_CONTENT_PATH		"/data/audit/eml/"
#define AUDIT_WEBMAIL_CONTENT_PATH	"/data/audit/webmail/"
#define AUDIT_BBS_CONTENT_PATH		"/data/audit/bbs/"
#define AUDIT_CSP_CONTENT_PATH		"/data/audit/csp/"
#define AUDIT_TMP_BBS_PATH			"/data/audit/tmp/bbs/"
#define AUDIT_TMP_MAIL_PATH			"/data/audit/tmp/eml/"
#define AUDIT_TMP_CSP_PATH			"/data/audit/tmp/csp/"
#define AUDIT_TMP_WEBMAIL_PATH		"/data/audit/tmp/webmail/"



#define AUDIT_LOG_PATH			"/data/audit/audit_log/"
#define AUDIT_CSP_ID_PATH		"/usr/inp/cfg/cspid.cfg"
#define AUDIT_COMMAND_LEN		100500
#define AUDIT_CMD_LEN			500
#define AUDIT_IPADDR_LEN		16
#define AUDIT_INIT_SEARCHID		"select max(search_id) from audit_search"
#define AUDIT_INIT_TELNETID		"select max(telnet_id) from audit_telnet"
#define AUDIT_INIT_FTPID			"select max(ftp_id) from audit_ftp"
#define AUDIT_INIT_MAILID			"select max(mail_id) from audit_mail"
#define AUDIT_INIT_IMID				"select max(im_id) from audit_im"
#define AUDIT_INIT_IMIDQQ			"select max(im_id) from audit_im_qq"

#define AUDIT_INIT_POSTID			"select max(post_id) from audit_post"
#define AUDIT_INIT_MSGID			"select max(msg_id) from audit_im_msg"
#define AUDIT_INIT_FILEID			"select max(file_id) from audit_im_file"
#define AUDIT_INIT_ATTACHID		"select max(attach_id) from audit_mail_attach"
#define AUDIT_INIT_QQIM			"select max(im_id) from audit_im_qq"
#define AUDIT_INDEX_URL		1000
#define AUDIT_TIMEOUT			120
#define AUDIT_HOSTS_MAX		100
#define AUDIT_HOST_LEN			16



#define get_u8(X,O)  (*(unsigned char *)(X + O))
#define get_u16(X,O)  (*(unsigned short *)(X + O))
#define get_u32(X,O)  (*(unsigned int *)(X + O))
#define max(a,b)       (a) >( b) ?( a):( b)
#define UCHARPTR(ptr) ((unsigned char*)(void*)ptr)

struct _http_info{
	u_int32_t offset;
	u_int64_t id;
};

typedef struct{
	time_t time;
	int fp;
	u_int64_t hash;
}Taudit_desfp;

typedef struct
{	
	u_int32_t offset;
	u_int64_t id;
	
}Taudit_mail_info;
	

typedef struct{
	u_int32_t offset;
	u_int64_t id;
	
}Taudit_bbs_info;


typedef struct{
	u_int16_t file_nums;
	u_int32_t fileid;
	u_int32_t msg_nums;
	u_int64_t  id;
	u_int64_t  msgid;
	char temp_prev[AUDIT_PACK_LEN];
	char temp_next[AUDIT_PACK_LEN];
}Taudit_im_info;

typedef struct {
	char user[AUDIT_ACCOUNT_LEN];
	char pass[AUDIT_ACCOUNT_LEN];
	Bool log_state:2,end_flag,mode_flag:2,trans_flag:2 ;
	u_int64_t id;
	u_int16_t cmd_id;
	int res_len;
	int port ;
	char cmd[AUDIT_ACCOUNT_LEN];
	char flnm[AUDIT_CMD_LEN];

}Taudit_ftp_info;

/*
typedef struct {
//pthread_rwlock_t ftp_file_lock;
	char user[AUDIT_ACCOUNT_LEN];
	int port;
	char file_cmd[AUDIT_ACCOUNT_LEN];
	char srcip[32];
	char dstip[32];
	char srcmac[64];
	char dstmac[64];
	int file_set;
	int res_len;
}Taudit_file_info;
*/
/*
typedef struct {
	char user[AUDIT_ACCOUNT_LEN];
	int port;
	char file_cmd[AUDIT_CMD_LEN];
	char srcip[32];
	char dstip[32];
	char srcmac[64];
	char dstmac[64];
	int file_set;
	int res_len;
}Tfile_info;
*/
typedef struct {
	char *user;
	int port;
	char *file_cmd;
	/*
	char *srcip;
	char *dstip;
	char *srcmac;
	char *dstmac;
	*/
	int file_set;
	int res_len;
}Tfile_info;

typedef struct {
	Tfile_info f_info;
	pthread_rwlock_t ftp_file_lock;
}Taudit_file_info;
/*
typedef struct {
	char user[AUDIT_ACCOUNT_LEN];
	char pass[AUDIT_ACCOUNT_LEN];
	u_int8_t pos;
	u_int8_t index;
	Bool user_state:2,log_state:2,pass_state:2,cmd_state:2;
	char cmd[AUDIT_CMD_LEN];
	u_int64_t id;
	u_int16_t cmd_id;
	char res_content[1000];
	Bool cmd_flag:2, mode_flag:2, end_flag:2 ;
	int res_len;
}Taudit_telnet_info;
*/

typedef struct {
	char user[AUDIT_ACCOUNT_LEN];
	char pass[AUDIT_ACCOUNT_LEN];
	u_int8_t pos;
	u_int8_t index;
	u_int8_t cmd_pos;
	u_int8_t stack_num;
	
	Bool user_state:2,log_state:2,pass_state:2,cmd_state:2;
	//Bool cmd_flag:2, mode_flag:2, end_flag:2 ;
	Bool cmd_flag:2, mode_flag:2, end_flag:2,gui_flag:2 ;
	Bool log_set;
	char cmd[AUDIT_TELCMD_LEN];
	u_int64_t id;
	u_int16_t cmd_id;
	time_t start ,end;
	char res_content[1000];
	char cmd_stack[STACK_MAX_NUM][AUDIT_TELCMD_LEN];//�����         
																	
	int res_len;
}Taudit_telnet_info;

typedef struct{
        u_int64_t  id;
        char content[AUDIT_PACK_LEN];
}Taudit_search_info;

typedef struct {
	struct _http_info http;
}Taudit_csp_info;

typedef struct{
	char host[40];
}DBMHOSTS;

u_int64_t mail_id;
u_int64_t attach_id;
u_int64_t bbs_id;
u_int64_t im_id;
u_int64_t msg_id;
u_int32_t file_id;
u_int32_t im_qq_id;
u_int64_t search_id;
u_int64_t ftp_id;
u_int64_t telnet_id;
u_int64_t csp_id;
extern pthread_mutex_t csp_id_mutex;
extern pthread_mutex_t telnet_id_mutex;
extern pthread_mutex_t ftp_id_mutex;



extern char tablesTime[AUDIT_TIME_LEN];


Taudit_file_info file_info[AUDIT_INDEX_NUM];
Taudit_ftp_info ftp_info[THREADS_NUM][AUDIT_INDEX_NUM];

Taudit_telnet_info telnet_info[THREADS_NUM][AUDIT_INDEX_NUM];
//Taudit_ftp_info ftp_info[THREADS_NUM][AUDIT_INDEX_NUM];
Taudit_mail_info mail_info[AUDIT_INDEX_NUM];
Taudit_bbs_info bbs_info[AUDIT_INDEX_NUM];
Taudit_im_info im_info[AUDIT_INDEX_NUM];
Taudit_search_info search_info[AUDIT_INDEX_NUM];
Taudit_desfp audit_desfp[MAX_FILE_FD];
Taudit_csp_info csp_info[AUDIT_INDEX_NUM];
u_int32_t mail_id_hash[AUDIT_INDEX_NUM];
u_int32_t bbs_id_hash[AUDIT_INDEX_NUM];
u_int32_t csp_id_hash[AUDIT_INDEX_NUM];



MYSQL audit_mysql;
FILE *audit_temp_fp,*attach_log_fp;
FILE * audit_error_log;
char audit_url[AUDIT_INDEX_URL][AUDIT_URL_LEN];

struct copy_packet_counter *copy_count[THREADS_ORACLE_NUM];
struct audit_packet_slot *audit_slot[THREADS_ORACLE_NUM];

extern int audit_policy_shmem_ret;
pthread_mutex_t audit_desfp_lock;
extern DBMHOSTS ips[AUDIT_HOSTS_MAX];
unsigned char *_base64_decode(const char *value, int *rlen,unsigned char *result);

int base64_decode(char decoded[],char src[]);

PRUint32 im_audit(PRUint32 index);
void audit_id_init();

PRUint32 web_post_audit(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info, unsigned char * data,int type);
u_int32_t audit_im(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t audit_bbs(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t mail_smtp(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t mail_pop3(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t mail_web(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t im_fetion(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t im_msn1(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t im_msn2(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t im_yahoo1(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t im_yahoo2(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t audit_ftp(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);

u_int32_t audit_search_google(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t audit_search_baidu(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * data);
u_int32_t  im_gtalk(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char *data);
u_int32_t  im_qq(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char *data);
u_int32_t audit_telnet(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * payload);
u_int32_t audit_ftp(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * payload);
u_int32_t  audit_ftp_file(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char* payload);
int CspPackRefrom(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char* payload);



time_t _get_file_time(const char *);




char * findstring(char *data,PRInt16 string);
char *findstring_back(char *data,char *string);
char *find_key(char string[],char *key,int length);
char *strlink(char dest[],char *src);
PRUint32 serch(char s);
int check_audit_strings(PRUint32 index, char *string);
void delete_temp_file(char *filename);
int gbk_to_utf8(char * gbk_str, unsigned long gbk_len,char * utf8_str,unsigned long  utf8_len);
int iso_to_utf8(char * iso_str, int iso_len,char * utf8_str,int utf8_len);
PRUint32 url_2_utf8(char utf8_code[],char url_code[], int length);
unsigned long get_file_size_audit(char *filename);
int get_db_line(MYSQL * mysql, char sql [ ], char line [ ]);
void escape(char *from);
int code_convert(char * from_charset,char * to_charset,char  **input, size_t *input_len,char  ** output, size_t * output_len);
 void bit_set(int i,u_int8_t a[]);
 void bit_clr(int i,u_int8_t a[]) ;
 int  bit_test(int i,u_int8_t a[]);
 int NC_daemon_audit(void);
int CheckReformedFile(char * filename);
int AuditWrite(int Fd,char *psData,int Len);








#endif 
