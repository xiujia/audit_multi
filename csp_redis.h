#ifndef _CSP_REDIS_H
#define _CSP_REDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <hiredis/hiredis.h>
//#include "hiredis.h"

#define REDISSERVERHOST  	"127.0.0.1"
#define REDISSERVERPORT  	6379

#define COMMANDLEN		    	500
#define VALUELEN			500

#define OPERATION_SET		1
#define OPERATION_GET		2

#define REDIS_DB_0			0                   	 //ip-username-his
#define REDIS_DB_1			1			 //ip-username-portal
#define REDIS_DB_2			2			//session - username-his
#define REDIS_DB_3			3			//session - username-portal
#define REDIS_DB_ACCOUNT_ID	4
#define REDIS_DB_URL_ID		5
#define REDIS_DB_6			6			//DEPARTMENT IP AND SESSION
#define REDIS_TABLE_MAX_NUM	16
extern redisContext * conn;


int CspRedisOperation(int db,int type,char * key, char *value,redisContext * conn);
int CspRedisClear(int db,redisContext * conn);

#endif
