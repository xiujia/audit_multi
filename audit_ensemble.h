#ifndef _AUDIT_ENSEMBLE_H
#define _AUDIT_ENSEMBLE_H

#include "audit_release.h"
#include "audit_api.h"
#include "csp_deal.h"
#include "audit_database.h"
#include "csp_redis.h"

#include  <dirent.h>
#define MAX_THREAD_NUM    20 

/****************************HIS WEB PORTAL***************************************/
#define CSP_ID_LEN  		21
#define CSP_USERID_LEN 		11
#define CSP_TIMES_LEN 		40
#define CSP_IP_LEN 			16
#define CSP_MAC_LEN 		20
#define CSP_PATH_LEN 		1024
#define CSP_PORT_LEN 		10

#define CSP_HEAD_LEN 		200
#define CSP_USER_NAME 		50
#define CSP_URL_LEN  		1024
#define CSP_DEPART_LEN 		30
#define CSP_WEBCONTENT_LEN	10240
#define CSP_EVENT_LEN 		200
#define CSP_TFORM_LEN		50
#define CSP_SESSION_LEN		200
#define CSP_KEY_LEN 		100
#define CSP_VALUE_LEN		2000
#define CSP_KEYVALUE_LEN	2000
#define CSP_PAYLOADLEN_LEN	5
#define CSP_OPERAT_LEN		30
#define CSP_BODY_LEN		10240
#define CSP_QUERY_LEN		20480
#define CSP_FILENAME_LEN	32
#define CSP_HTTPHEAD_LEN	1024*5
#define CSP_FILE_MAX		1024*5
#define CSP_FILE_MAX_LEN		1024*1024*10

#define CSP_REQUEST_TYPE_POST		1
#define CSP_REQUEST_TYPE_GET		2

#define CSP_HTTP_TRANS_ENCODE_LEN		10
#define CSP_HTTP_CONTENT_ENCODE_LEN		10
#define CSP_HTTP_CONTENT_LENTGH_LEN 	10

#define CSP_KEYWD_SESSION 				"CSPSESSIONID-SP-80-UP-="
#define CSP_KEYWD_SESSION_PORTAL		"CSPSESSIONID-SP-57772-UP-csp-sys-="
#define CSP_KEYWD_SESSION_DOC_PORTAL	"CSPSESSIONID-SP-57772-UP-csp-documatic-="
#define CSP_KEYWD_REFERER				"Referer: "
#define CSP_KEYWD_USERNAME				"USERNAME="
#define CSP_KEYWD_USERNAME_PORTAL		"CacheUserName="
#define CSP_KEYWD_USERNAME_INCOOKIE		"Username="
#define CSP_KEYWD_DEPARTMENT			"DEPARTMENTAlias="
#define CSP_KEYWD_END_1					"&"
#define CSP_KEYWD_END_2					";"
#define CSP_KEYWD_END_3					"\r\n"
#define CSP_KEYWD_END_4					"\r\n\r\n"
#define CSP_KEYWD_END_5					"="

#define CSP_HTTP_KW_CONTENT_LENGTH				"Content-Length"
#define CSP_HTTP_KW_CONTENT_ENCODING				"Content-Encoding"
#define CSP_HTTP_KW_TRANS_ENCODING				"Transfer-Encoding"
#define CSP_HTTP_ENCODING_STATUS_CHUNK			1
#define CSP_HTTP_ENCODING_STATUS_GZIP			2
#define CSP_HTTP_ENCODING_STATUS_CHUNK_GZIP		3

#define CSP_CONFGI_HOST_PATH				"/usr/inp/cfg/dbm_cache_hosts.cfg"

#define CONGOUS_URL_PASSPORT 			"/cognos/cgi-bin/cognos.cgi?m_passportID"

#define CSP_KEY_TYPE_INSERT		1
#define CSP_KEY_TYPE_DELETE		2
#define CSP_KEY_TYPE_UPDATE		3
#define CSP_KEY_TYPE_TFORM		4
#define CSP_KEY_TYPE_WEVENT		5

/*****************************************************************************/

//AUDIT_ENSEMBLE_REL rel;
#if GA_TEST
#define MONGO_ENSEMBLE_REQUEST_VALUES	"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%d|colfam1:sub_app_id=%d|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%hu|colfam1:dst_port=%hu|colfam1:user_id=%hu|colfam1:saveflag=%d|colfam1:interval_time=%d|colfam1:security_level=%hu|colfam1:operation_command=%u|colfam\n"
#define MONGO_ENSEMBLE_REQUEST_PARAMS	rel->times,rel->table,rel->type,rel->sub_app_id,userip,desip,rel->srcmac,rel->desmac,rel->cliport,rel->serport,rel->userid,rel->saveflag,rel->interval_time,rel->security_level,rel->operation_len
#else
#define MONGO_ENSEMBLE_REQUEST_VALUES	"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%d|colfam1:sub_app_id=%d|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%hu|colfam1:dst_port=%hu|colfam1:user_id=%hu|colfam1:saveflag=%d|colfam1:interval_time=%d|colfam1:operation_command=%u|colfam\n"
#define MONGO_ENSEMBLE_REQUEST_PARAMS	rel->times,rel->table,rel->type,rel->sub_app_id,userip,desip,rel->srcmac,rel->desmac,rel->cliport,rel->serport,rel->userid,rel->saveflag,rel->interval_time,rel->operation_len
#endif
#define MONGO_ENSEMBLE_RESPONSE_VALUES	"rowkey=%lu|colfam1:line_num=%d|colfam1:response_content=%u|colfam\n"
#define MONGO_ENSEMBLE_RESPONSE_PARAMS	rel->times,rel->line_num,rel->response_len

#define REQ_RES_LEN  1024*1024*5
#define SEG_LEN		 65537
#define PATH_LEN	 1024



typedef struct {
	unsigned int plen;
	unsigned int session_num_1;
	unsigned short session_num_2;
	unsigned short unknown_flag;
	unsigned short opt_flg;
}AUDIT_ENSEMBLE_HEAD;

typedef struct{
	unsigned char len;
	unsigned char type;
	unsigned char data[0];
}AUDIT_ENSEMBLE_DATA_ST;

typedef struct{
	unsigned short len;
	unsigned char type;
	unsigned char data[0];
}AUDIT_ENSEMBLE_DATA2_ST;


	char userName[CSP_USER_NAME];
	char requestUrl[CSP_URL_LEN];
//	char referrUrl[CSP_URL_LEN];
	char department[CSP_DEPART_LEN];
	char webContent[CSP_WEBCONTENT_LEN];
//	char event[CSP_EVENT_LEN];
	char session[CSP_SESSION_LEN];
//	char tform[CSP_TFORM_LEN];
	char request_sql[2048];

typedef struct {
	char department[CSP_DEPART_LEN];
	char session[CSP_SESSION_LEN];
	char userName[CSP_USER_NAME];
	char requestUrl[CSP_URL_LEN];
	char webContent[CSP_WEBCONTENT_LEN];
	char transEncode[CSP_HTTP_TRANS_ENCODE_LEN];
	char contenEncode [CSP_HTTP_CONTENT_ENCODE_LEN];
	char contentLength[CSP_HTTP_CONTENT_LENTGH_LEN];
	char request_sql[2048];
	char charset[10];
	unsigned short type;
	unsigned short appId;
}AUDIT_ENSEMBLE_WEB_REL;


typedef struct{
	unsigned short dir;
	unsigned short request_type;
	unsigned short security_level;
 	long operation_len;
	long response_len;
	int  type;
	int sub_app_id;
	char * request_data;
	char * response_data;
	unsigned long request_size;
	unsigned long response_size;
	int saveflag;
	time_t request_time;
	time_t response_time;
	time_t interval_time;
	int line_num;
	redisContext * conn;
#if MULTI_THREADS
	unsigned short thid;
	char * operation;
	char * response;
	unsigned short id;
	unsigned short userid;
	unsigned short cliport;
	unsigned short serport;
	unsigned long times;
	unsigned int userip;
	unsigned int desip;
	unsigned int policytime;
	char table[10];
	char srcmac[CSP_MAC_LEN];
	char desmac[CSP_MAC_LEN];
	AUDIT_ENSEMBLE_WEB_REL web;
#else
	char userid[CSP_USERID_LEN];
	char times[CSP_TIMES_LEN];
	char userip[CSP_IP_LEN];
	char srcmac[CSP_MAC_LEN];
	char desip[CSP_IP_LEN];
	char desmac[CSP_MAC_LEN];
	char cliport[CSP_PORT_LEN];
	char serport[CSP_PORT_LEN];
	char policytime[CSP_TIMES_LEN];
	char table[AUDIT_TIME_LEN];
	char operation[REQ_RES_LEN];
	char response[REQ_RES_LEN];
#endif	
}AUDIT_ENSEMBLE_REL;






typedef struct{
	int segLen;
	int dataLen;
}LENS;

typedef struct {
	char * key1;
	char * key2;
}OPTKEYS;


char httpContent[MAX_THREAD_NUM][CSP_FILE_MAX_LEN];
char httpUnChunked[MAX_THREAD_NUM][CSP_FILE_MAX_LEN];
char httpUzip[MAX_THREAD_NUM][CSP_FILE_MAX_LEN];

char * httpptr[MAX_THREAD_NUM];
unsigned long httplen[MAX_THREAD_NUM];



//char request_cmd_debug[PATH_LEN];
//char response_cmd_debug[PATH_LEN];
unsigned char fileContent1[MAX_THREAD_NUM][REQ_RES_LEN];
unsigned char fileContent2[MAX_THREAD_NUM][REQ_RES_LEN];

unsigned char from_segment[MAX_THREAD_NUM][REQ_RES_LEN];
unsigned char to_segment[MAX_THREAD_NUM][REQ_RES_LEN];
unsigned char segment_str[MAX_THREAD_NUM][REQ_RES_LEN];

unsigned char ensemble_payload[MAX_THREAD_NUM][REQ_RES_LEN];
//unsigned char datamonth[AUDIT_TIME_LEN];
unsigned long first_len[MAX_THREAD_NUM];
//int  runFlag;



#endif
