#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/time.h>

#include "audit_database_sql.h"

time_t _get_file_time(const char *filename)
{
        struct stat buf;
        if(stat(filename, &buf)<0)
        {
                return 0;
        }
        return buf.st_mtime;
}
void mv_file(char srcpath[],char despath[],char name[]){

	char filename[100]={0};
	char newfilename[100]={0};

	sprintf(filename,"%s%s",srcpath,name);
	sprintf(newfilename,"%s%s",despath,name);		
	
//	printf("mv %s %s \n",filename,newfilename);
	rename(filename,newfilename);

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

int main(int argc, char **argv){

	NC_daemon_audit();
	DIR *sqlDir;
	struct dirent *sqlDp;
	time_t file_modify_time,timenow;
	char fname[100];
	int len =0;

	while(1)
	{
		sqlDir = opendir(SQL_TMP); //打开目标文件夹
		if (NULL == sqlDir)
		{
			printf("opendir error !\n");
			return -1;
		}
		time(&timenow);
		while ((sqlDp = readdir(sqlDir)) != NULL) 
		{

			if(strcmp(sqlDp->d_name,".") == 0 || strcmp(sqlDp->d_name,"..") == 0){
				continue;
			}
			sprintf(fname,"%s%s",SQL_TMP,sqlDp->d_name);
			len = strlen(fname);
			fname[len] = '\0';
			file_modify_time = _get_file_time(fname);
			if(timenow - file_modify_time > 60){
				//mv file to  SQL_PATH
				mv_file(SQL_TMP,SQL_PATH,sqlDp->d_name);
			}
			
		}
		closedir(sqlDir);
		sqlDir = NULL;
		sleep(10);
	}


	return 0;
}


