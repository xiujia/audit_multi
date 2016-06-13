
#include <stdlib.h>
#include <stdio.h>
#include "csp_redis.h"
redisContext * conn;


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

int main(int argc,char **argv){
	NC_daemon_audit();
	int db[10]={0};
	if (argc > 10) return -1;
	int i = 0;
	conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
	if(argc >1){
		for(i = 0;i<argc;){
			db[i]= atoi(argv[++i]);
			CspRedisClear(db[i],conn);
		}
	}
	else{
		db[0] = 2;
		db[1] = 3;
		CspRedisClear(db[0],conn);
		CspRedisClear(db[1],conn);
	}
	
	redisFree(conn);
	return 0;
}
