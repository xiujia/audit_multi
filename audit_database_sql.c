#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#include "audit_database_sql.h"
#include "audit_release.h"
#define PORT 80
#define NUM 200
#define ALARM_REQUEST	"GET /alarmrefresh.do HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/40.0.2214.111 Safari/537.36\r\nAccept-Encoding: gzip, deflate, sdch\r\nAccept-Language: zh-CN,zh;q=0.8\r\n\r\n";
//#define ALARM_REQUEST	"GET /alarmrefresh.do HTTP/1.1\r\n"

typedef  struct  _link{
	char fileName[NUM];
	struct _link  *next;
}Link,Node;

Node * createNode( char *elem)
{
	Node * node =(Node *)malloc(sizeof(Node));
	if(!node) return NULL;
	strcpy(node->fileName, elem);
	node->next = NULL;
	return node;
}

 void addLink(Link *head,Link *node){
	node->next = head->next;
	head->next = node;

}
 void delLink(Link *head){
	Node * node;
	int i = 0;
	node = head->next;
	while(node){
		head->next = node->next;
		node->next = NULL;
		free(node);
		node = head->next;
//		printf("i = %d\n",++i);
	}
}

/*线程函数  创建socket 链接服务器发送数据*/
void *fun(void *p)
{
	int fd ,ret, wbyte;
	struct sockaddr_in serverinfo;
	char server_ip[] = "127.0.0.1";
	 //= "GET /alarmrefresh.do HTTP/1.1/r/nHost: 127.0.0.1\r\nUser-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\nConnection: close\r\n\r\n";
	char * tmp;

	tmp = (char *)p;



	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{		
		perror("socket");
		pthread_exit(NULL);
	}

	serverinfo.sin_family = AF_INET;
	serverinfo.sin_port = htons(PORT);
	inet_pton(AF_INET, server_ip,&serverinfo.sin_addr.s_addr);
	ret = connect(fd, (struct sockaddr*)&serverinfo, sizeof(serverinfo));
	if(ret < 0)
	{
		perror("connect()");
		pthread_exit(NULL);
	}
//	printf("befroe write tmp:%s",tmp);
//	printf("byte = %d\n",strlen(tmp));
	wbyte = write(fd, tmp, strlen(tmp));
//	printf("wbyte = %d\n",wbyte);
//	printf("tmp:%s",tmp);
//	printf("sizeof:%d\n",sizeof(tmp));
	if(wbyte < 0)
	{
		perror("write()");
		exit(1);
	}else
	{
		while(wbyte < strlen(tmp))
		{
			wbyte += write(fd, tmp+wbyte, strlen(tmp)-wbyte);
		}
	}
	
//	printf("alarm .\n");
//	sleep(4);
	close(fd);
	pthread_exit(NULL);
}

int cmp(const void *a, const void *b)  
{  
    return strcmp(a,b);;  
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


int main(int argc, char *argv[])
{
	NC_daemon_audit();
	DIR *sqlDir;
	struct dirent *sqlDp;
	char dName[NUM], intoSql[NUM];
	pthread_t tid;
	int flag = 0; //标识是否存在alarm文件
	//Node * node;
	//Node * head;
	//char **p;
	//unsinged long szFilename[1000]
	//char * flnm[1000];
	//char tmp[100];
	char flnm[1000][100];
	int i, j;
	char  * request = ALARM_REQUEST;
	
	while(1)
	{
		sqlDir = opendir(SQL_PATH); //打开目标文件夹
		if (NULL == sqlDir)
		{
			printf("opendir error !\n");
			return -1;
		}
		memset(*flnm,0 ,100*1000);
		i = 0; //记录文件个数
		//head = createNode("head");//空的头节点
		while ((sqlDp = readdir(sqlDir)) != NULL) 
		{

			if(strcmp(sqlDp->d_name,".") == 0 || strcmp(sqlDp->d_name,"..") == 0){
				continue;
			}
			
			if (memcmp(sqlDp->d_name, CSP_SQL_ALARM_FILE, 14) == 0 || memcmp(sqlDp->d_name, TERMINAL_SQL_ALARM_FILE, 19) == 0 || memcmp(sqlDp->d_name, STUDIO_SQL_ALARM_FILE, 17) == 0)
			{

				//node = createNode(sqlDp->d_name);  //创建链节
				//addLink(head,node);  //添加链节
			//	memset(&flnm[i],0,100);
				strcpy(flnm[i],sqlDp->d_name);
				flnm[i][strlen(flnm[i])]='\0';
				i++;
				
				flag = 1;

			}else
				if (memcmp(sqlDp->d_name, CSP_SQL_FILE, 8) == 0 || memcmp(sqlDp->d_name, TERMINAL_SQL_FILE, 13) == 0 || memcmp(sqlDp->d_name, STUDIO_SQL_FILE, 11) == 0)
				{
				

					//node = createNode(sqlDp->d_name);
					//addLink(head,node);
				//	memset(&flnm[i],0,100);
					strcpy(flnm[i],sqlDp->d_name);
					flnm[i][strlen(flnm[i])]='\0';
					
					i++;

				}else
				{
					continue; // 不符合要求的文件跳过
				}
				if(i==1000)
				{
					break;
				}

		}
		if(i==0)
		{
			closedir(sqlDir);
			sqlDir = NULL;
			continue;
		}
		if(i>1)
			qsort(flnm,i,50,cmp);  //排序函数
		
		//p=(char**)malloc(i*sizeof(char*));  //动态分配创建字符数组
		
		
		//flnm[j] = tmp;
		/*
		if(p == NULL){
			delLink(head);
			continue;
		}
		
		for(j=0;j<i;j++)
		{
			p[j]=(char*)malloc(NUM*sizeof(char));
			memset(p[i],0,NUM*sizeof(char));
		}
		
		node = head->next;
		j = 0;
		while(node)
		{	
			memcpy(p[j], node->fileName, strlen(node->fileName));
			node = node ->next; 
			j++;
		}
*/
		


		for( j=0; j<i; j++ )
		{
			//printf("filename:%s\n",flnm[j]);
			bzero(intoSql, sizeof(intoSql));
			sprintf(intoSql, "mysql < %s%s 1>/dev/null",SQL_PATH, flnm[j]);
			system(intoSql);
//			puts(intoSql);
				
			bzero(dName, sizeof(dName));
			#if CSP_SQL_RM_FILE
				sprintf( dName,"rm -rf "SQL_PATH"%s" , flnm[j] );
			#else 
				sprintf( dName,"mv -f "SQL_PATH"%s "SQL_TMP"%s" , flnm[j], flnm[j]);
			#endif
			system(dName);
			//free(p[j]);
		}
		

		closedir(sqlDir);
		sqlDir = NULL;
		if( flag == 1)
		{
			pthread_create(&tid, NULL, fun, (void *)request);
			flag = 0;
		}
		//delLink(head);
		//free(p);
		
		usleep(1000);
	}
}








