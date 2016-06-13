#include "audit_time_api.h"

#define ONEHOUR 1440
RUNTIME loopTime;
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
	int month_now = 0;
	int year_next = 0,year_last=0,year_now = 0;
	char cmd[200] ;
	unsigned long times;
	while(1){
		sleep(60);
		times = get_min();
//		printf("%lu\n",times);
		if( times%ONEHOUR == 1439 ) 
			system("/usr/inp/bin/audit_redis_clear  ");
	}
}

