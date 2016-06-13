#ifndef _CSP_DEAL_H
#define _CSP_DEAL_H

#include "audit_api.h"
#include "audit_database.h"
#include "csp_redis.h"

#include  <dirent.h>



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

typedef int (*CSPFUN)(char *);

typedef struct _csp_file_head{
	char id[CSP_ID_LEN];
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
	unsigned short dir;
}CSP_FILE_HEAD;

typedef struct _csp_file_data{
	int appId;
	char userName[CSP_USER_NAME];
	char requestUrl[CSP_URL_LEN];
//	char referrUrl[CSP_URL_LEN];
	char department[CSP_DEPART_LEN];
	char webContent[CSP_WEBCONTENT_LEN];
//	char event[CSP_EVENT_LEN];
	char session[CSP_SESSION_LEN];
//	char tform[CSP_TFORM_LEN];
	char request_sql[2048];
	char line_num[20];
}CSP_FILE_DATA;

typedef struct _csp_file_http{
	char transEncode[CSP_HTTP_TRANS_ENCODE_LEN];
	char contenEncode [CSP_HTTP_CONTENT_ENCODE_LEN];
	char contentLength[CSP_HTTP_CONTENT_LENTGH_LEN];
	char charset[10];
}CSP_FILE_HTTP;
typedef struct _csp_alarm{
	unsigned int id;
	unsigned int type;
	unsigned int  level[3];
}CSP_ALARM;

typedef struct _csp_file_info{
	char *request_data;
	char *response_data;
	redisContext *conn;
	unsigned int request_size;
	unsigned int response_size;
	int type;
	int isalarm;
	CSP_FILE_HEAD cspHead;
	CSP_FILE_DATA cspData;
	CSP_FILE_HTTP cspHttpHead;
	CSP_ALARM alarm;
	unsigned short security_level;
	unsigned short saveflag;
	time_t request_time;
	time_t response_time;
	time_t interval_time;
}CSP_FILE_INFO;

extern int policy_match(CSP_FILE_INFO *csp, CACHE_POLICY_CONF *policy_conf);

#endif
