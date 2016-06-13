#ifndef REDIS_NEW_API_H
#define REDIS_NEW_API_H

#include <hiredis/hiredis.h>


#define REDISSERVERHOST  	"127.0.0.1"
#define REDISSERVERPORT  	6379
#define REDIS_IP_MAC_TABLE  15   

#define REDIS_CALL_ERROR_CACHE(A)  \
	fprintf(stderr,"error %d:%s\n",(A)->err,(A)->errstr)



inline void redis_call_error_catch(redisContext * conn);
int redis_get_value_str(int databaseNO,char *key,char *value,redisContext * conn);
int redis_set_value_str(int databaseNO,char *key,char *value,redisContext * conn);
int get_mac_str(char * ip,char * mac,redisContext * conn);

int redis_clear_db(int db);


#endif
