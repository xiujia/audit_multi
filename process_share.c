
#include "audit_api.h"
#include <unistd.h>


int NC_deamon_audit(void) {
	pid_t pid;
	int ret;
	if ((pid = fork()) < 0) {
		return -1;
	} else if (pid != 0) {
		exit(0); /* parent goes bye-bye */
	}
	if ((ret = setsid()) < 0) /* become session leader */
	{
		printf("unable to setsid.\n");
	}
	setpgrp();
	return 0;
}
time_t _get_file_time(const char *filename)
{
        struct stat buf;
        if(stat(filename, &buf)<0)
        {
                return 0;
        }
        return buf.st_mtime;
}
int CheckReformedFile(char *filename){
	//return -1 file rm  return 0 good file
	time_t timenow,filetime;
	time(&timenow);
	filetime = _get_file_time(filename);
	if(filetime == -1 ) return 0;
	if(timenow - filetime > 600){
		unlink(filename);
		return -1;
	}
	else{
		return 0;
	}
	return 0;
}
