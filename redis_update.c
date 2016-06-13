#include "csp_db.h"
#include "csp_redis.h"

#define SEARCH_ACCOUNT_SQL	"SELECT  user_name,user_id FROM cfg_monitor_web_account"
#define SEARCH_URL_SQL			"SELECT request_url,id FROM cfg_monitor_web_requesturl "

MYSQL_RES * res;	
MYSQL_ROW row;
redisContext * conn;


int update_redis(MYSQL *msql,int db,char *query,redisContext *conn){
	char cmd[COMMANDLEN]={0};
	if(mysql_real_query(msql,query,strlen(query) )!=0 ){
				if(mysql_errno(msql)){			
					fprintf(stderr,"error %d : %s\n", mysql_errno(msql),mysql_error(msql));		
				}		
				return -1;
	}
	if((res = mysql_store_result(msql))==NULL){
		return -1;
	}
	while((row = mysql_fetch_row(res)) != NULL) {
		CspRedisOperation(db,OPERATION_SET,row[0],row[1],conn);
	}
	return 0;
}

int NC_daemon_audit(void)
{  
	pid_t pid;
	int ret;
	if ((pid = fork()) < 0){
		return -1;
	}
	else if (pid != 0){
		exit(0); /* parent goes bye-bye */
	}
	if((ret=setsid()) < 0) /* become session leader */
	{
		printf("unable to setsid.\n");
	}
	 setpgrp();
	 return 0;
}


int main(int argc , char * argv[]){
	NC_daemon_audit();
	MYSQL monitorMysql;
	int db;
	if(argc < 2){
		return -1;
	}
	db = atoi(argv[1]);
	if(db <0 ) return -1;
	conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
	CspRedisClear(db,conn);
	if(connect_mysql(&monitorMysql) < 0){
		printf("monitor mysql connnect failed.\n");
		return -1;
	}
	if(db == REDIS_DB_ACCOUNT_ID)
		update_redis(&monitorMysql,db,SEARCH_ACCOUNT_SQL,conn);
	else if(db == REDIS_DB_URL_ID)
		update_redis(&monitorMysql,db,SEARCH_URL_SQL,conn);
	
	close_mysql(&monitorMysql);
	
	return 0;

}
