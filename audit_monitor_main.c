#include <unistd.h>
#include <signal.h>
#include "csp_deal.h"
#include "csp_redis.h"
#include "audit_database_sql.h"
#include "audit_release.h"
#include "qsort.h"
#include <zlib.h>
#include "chunk.h"
#include "gzip.h"
#include "csp_policy.h"



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
	
}
