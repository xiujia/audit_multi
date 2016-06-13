#include "audit_time_api.h"


#define ONEHOUR 1440;
int edays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
int month_next[12]={2,3,4,5,6,7,8,9,10,11,12,1};
int month_last[12]={10,11,12,1,2,3,4,5,6,7,8,9};
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
	while(1){
		sleep(60);
		get_run_time(&loopTime);
		month_now = loopTime.month+1;
		year_last = year_next = year_now = loopTime.year;
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"/usr/inp/bin/create_table.sh %04d_%02d",year_now+1900,month_now);
	//	printf("%s\n",cmd);
		system(cmd); //创建表
		if(month_now == 12 ){
			year_next +=1;
		}
		else if(month_now < 4){
			year_last -= 1;
		}
/*		if( loopTime.redis_clear_time%ONEHOUR == 0 ) 
			system("/usr/inp/bin/audit_redis_clear  ");
*/
		if((loopTime.year-72)%4 == 0)
                   	edays[1] = 29;
		else{
			edays[1] = 28;
		}
		
		if(loopTime.day != edays[month_now-1] ){
			continue;
		}
		if(loopTime.hour !=23)
			continue;
		if(loopTime.min!= 59)
			continue;

		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"/usr/inp/bin/create_table.sh %04d_%02d",year_next+1900,month_next[month_now - 1]);
		system(cmd); //创建表
/*
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"/usr/inp/bin/backup_table.sh %04d_%02d",year_last+1900,month_last[month_now-1]);
		system(cmd);
*/		
		
	}
}
