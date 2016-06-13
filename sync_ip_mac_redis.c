#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "redis_new_api.h"

/*  0x0a      xxxxx    0x0a*/

#define LINE_NUM 		1024
#define IP_ADDR_LEN 	20
#define MAC_ADDR_LEN 	30
#define PATH			"/media/data/tmp/1.tmp"

#define IP_MAC_KEWORD_OID	"ipNetToMediaPhysAddress."
#define IP_MAC_KEWORD_HEX	" = Hex-STRING: "

redisContext * globle_conn;



// ipNetToMediaPhysAddress.31.10.10.117.155 = Hex-STRING: 3C 97 0E C4 56 D7 
int get_ip_mac(char * line,char *ip,char *mac){
	char *start,*end;
	int len=0;
	
	len = strlen(IP_MAC_KEWORD_OID);
	if(strncmp(line,IP_MAC_KEWORD_OID,len)!=0){
		return -1;//no ip mac
	}

	if((start = strstr(line+len,"."))==NULL) return -1;

	start+=1;
	
	if((end = strstr(start,IP_MAC_KEWORD_HEX))==NULL) return -1;

	len = end - start;
	if(len <0 ||len > 15){
		return -1;
	}
	strncpy(ip,start,len);
	ip[len]='\0';

	start = end + strlen(IP_MAC_KEWORD_HEX);

	if((end = strstr(start," \n"))==NULL) return -1;

	len = end - start;

	if(len < 0||len > 17){
		return -1;
	}
	strncpy(mac,start,len);
	mac[len]='\0';
	return 0;

}


void mac_fmt(char * mac,int len){
	int i;
	for(i=0;i<len;i++){
		if(mac[i]==' '){
			mac[i]='-';
		}
	}
}

int sync_ip_mac_redis(redisContext * conn){
	char line[LINE_NUM]={0};
	char ip[IP_ADDR_LEN]={0};
	char mac[MAC_ADDR_LEN]={0};
	int line_count=0;
	FILE *fp;

	fp =  fopen(PATH,"r");
	if(!fp){
		return -1;
	}

	fgets(line,1024,fp);
	
	do{
		if(line_count > 0){
			//get key value
			
			if(get_ip_mac(line,ip,mac)==0){
				mac_fmt(mac,strlen(mac));
				redis_set_value_str(REDIS_IP_MAC_TABLE,ip,mac,conn);
			}
		}
		memset(line,0,LINE_NUM);
		line_count++;
	}while(fgets(line,1024,fp));
	fclose(fp);
	fp = NULL;
	return line_count-1;
}

void SigFun(int sig){
	redisFree(globle_conn);
	globle_conn=NULL;
	printf("%d signal .\n",sig);
	exit(0);
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

//argc == 2 带参数循环前台有打印    argc==1  默认60秒 循环 后台 无打印  argc==3 同2 但不循环
int main(int argc,char **argv){
	int sec = 60;

	switch(argc){
		case 1:
			NC_daemon_audit();
			break;
		case 2:
		case 3:
			sec = atoi(argv[1]);
			break;
	}

	redisContext * conn = NULL;
	globle_conn = NULL;
	int rel;
	
	
	
	conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
	if(conn->err){
		REDIS_CALL_ERROR_CACHE(conn);
		redisFree(conn);
		conn = NULL;
		return -1;
	}
	globle_conn = conn;

	signal(SIGINT,SigFun);
	signal(SIGTERM,SigFun);
    signal(SIGSEGV,SigFun);

	if(1){

		if((rel = sync_ip_mac_redis(conn))<0){
			fprintf(stderr,"Function:sync_ip_mac_redis returns [%d]\n",rel);
		}
		else{
			if(argc != 1)
				fprintf(stderr,"%d lines updated.\n",rel);
		}	
//		if(argc == 3)
	//		break;

	//	sleep(sec);
	}

	redisFree(conn);
	conn = NULL;
	globle_conn = NULL;

}
