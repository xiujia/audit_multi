#ifndef _AUDIT_DATABASE_H
#define _AUDIT_DATABASE_H
#include "audit_api.h"


#define IN
#define OUT

/* ********************************CSP*******************************************/


#define AUDIT_CSP_HOST						"172.21.21.78"
#define AUDIT_CSP_HOST_PORTAL		 		"172.21.21.78:57772"
#define AUDIT_CSP_REQUEST_LOGON_HIS			"/dthealth/web/csp/dhc.logon.csp"
#define AUDIT_CSP_REQUEST_LOGON_PORTAL		"/csp/sys/UtilHome.csp"
#define AUDIT_CSP_GLOBLE_VIEW_CSP			"/csp/sys/exp/UtilExpGlobalView.csp"
#define AUDIT_CSP_SQL_VIEW_CSP				"/csp/sys/exp/UtilSqlHome.csp"
#define AUDIT_CSP_REQUEST_CLS				"/dthealth/web/csp/%25CSP.Broker.cls"
#define AUDIT_CSP_REQUEST_CSP				"/dthealth/web/csp/websys.csp"
#define AUDIT_CSP_THREAD_NUM				10

#define HTTP_REQUEST						1
#define HTTP_RESPONSE						2

#define AUDIT_CSP_TYPE_HIS					1
#define AUDIT_CSP_TYPE_PORTAL				4




#define AUDIT_CSP_TYPE_HIS_LOGON 			3
#define AUDIT_CSP_TYPE_PORTAL_LOGON 		2
#define AUDIT_CSP_TYPE_GLOBLE_VIEW_CSP 		5
#define AUDIT_CSP_TYPE_SQL_VIEW_CSP			6
#define AUDIT_CSP_TYPE_CLS					7
#define AUDIT_CSP_TYPE_CSP					8
#define AUDIT_CSP_TYPE_OTHERS				100



extern pthread_rwlock_t tablesTime_lock;
extern char tablesTime[AUDIT_TIME_LEN];


#endif
