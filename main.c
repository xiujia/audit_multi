#include "pthread_pool.h"
#include "audit_ensemble.h"
#include "csp_policy.h"


char pattern[2048];
struct multi_parm *mp;
AUDIT_MULTI_THREAD_PARM *parm;
CThread_worker **wk;
pthread_t main_thread_id;

#define DATA_PATH	"/dev/shm/"

#define DES_PATH	"/dev/shm/tmp/"

int is_file_ready(char *filename,int times)
{
	  time_t now;
	  struct stat statbuf;   
    if (stat(filename, &statbuf) == -1) 
    {   
      // printf("Get stat on %s Error：%s\n",  filename, strerror(errno));   
        return -1;   
    }   
    time(&now);
    if(now-statbuf.st_mtime>times)
    return 1;
    else 
    if(statbuf.st_mtime-now>times)
    return 1;
    else    
    return 0;   

	
}
void ltrim ( char *s )
{
   char *p;
   p = s;
   while ( *p ==0x20|| *p == '\t' ) {*p++;}
   strcpy (s,p);
}
void rtrim (char *s)
{
  int i;
  i = strlen(s)-1;
  while((s[i]==0x20||s[i]=='\t'||s[i]=='\n')&& i>= 0 ) {i--;}
  s[i+1] = '\0';
}
void trim ( char *s )
{
 ltrim ( s );
 rtrim ( s );
}

int read_pattern()
{
	FILE *f1;
	char buff[2048];
	
	f1=fopen("/usr/inp/cfg/http_pattern","r");
	if(f1==NULL)
	{
	return -1;
	}
	if(NULL!=fgets(pattern, sizeof(pattern), f1))  
	 {      
        trim(pattern);
        //printf("Pattern***%s***\n",pattern); 
        fclose(f1);     
        return 1;     
    }      
    else     
    {     
       
        fclose(f1);     
        return -1;     
    }     
	
	
}

int filter_http_file(char *filename)
{
	FILE *fstream=NULL;       
  char buff[2048];
  char command[2048];     
  memset(buff,0,sizeof(buff));   
  sprintf(command,"grep -l -E -f /usr/inp/cfg/http_pattern %s",filename);  
  //printf("%s\n",command);
  if(NULL==(fstream=popen(command,"r")))       
    {      
        fprintf(stderr,"execute command failed: %s",strerror(errno));       
        return -1;       
    }      
    if(NULL!=fgets(buff, sizeof(buff), fstream))      
    {      
        pclose(fstream);     
        return 1;     
    }      
    else     
    {     
        pclose(fstream);     
        return -1;     
    }     
}



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



void SigFun(int sig){
	pthread_t a;;
	switch(sig){
		case SIGSEGV:
			printf("ensemble SIGSEGV signale %d.\n",sig);
			break;
		case SIGBUS:
			printf("ensemble SIGBUS signale %d.\n",sig);
			break;
		case SIGINT:
			printf("ensemble SIGINT signle %d.",sig);
		default:
			break;
	}
	if((a = getpid()) == main_thread_id){
		printf("pid = %u\n",a);
		exit(0);
	}

}
int max;
int allfile;
int main(int argc,char **argv) {
	if(argc==4)
		NC_deamon_audit();
    DIR *dp;
	int r;
    struct dirent *dirp;
    char oldpath[256];
    char newpath[256];
	char tmp_path[256];
	
	char oldname[256], newname[256],filename[256];
    int len;
    char responseName[256];
	char requestName[256];
	unsigned int files=0;
	allfile = 0;
		signal(SIGINT,SigFun);
		signal(SIGTERM,SigFun);
		signal(SIGSEGV,SigFun);

	sprintf(oldpath,"%s%s/",DATA_PATH,argv[2]);
	sprintf(newpath,"%s%s/",DES_PATH,argv[2]);
//	sprintf(newpath,"%s0/",DATA_PATH);
	mp = NULL;
	parm = NULL;
	pool = NULL;
	max = atoi(argv[1]);

	main_thread_id = getpid();
	
	
	r = pool_init(max);

	if(r != 0 ) {
		return -1;
	}
	
			dp = opendir(newpath);
			if(!dp) return -1;
			while((dirp=readdir(dp))) {
				if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
					continue;
				}
				
				sprintf(filename,"%s%s",newpath,dirp->d_name);
				len = strlen(filename);
				if (filename[len-1]=='t') {
					pool_add_worker(myprocess, filename,argv[2]);
				}
				else if(filename[len-1]=='e'){
				 sprintf(responseName,"%s",filename);
				 sprintf(requestName,"%s",filename);
				 len = strlen(requestName);
	       		 strcpy(&(requestName[len-strlen("response")]), "request");
	        	//printf("%s\n",filename_request);
		        	if(access(requestName, F_OK)==-1)
		        	{
		        		unlink(responseName);
		        	}  
				}

			}
			
			closedir(dp);
			dp = NULL;


	
    while (1) {
        dp = opendir(oldpath);
        if(!dp) { continue; }
        while((dirp=readdir(dp))) {
            if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
                continue;
            }
            
			sprintf(filename,"%s%s",oldpath,dirp->d_name);
			len = strlen(filename);

	//		sprintf(newname,"%s%s",newpath,dirp->d_name);
	//		rename(filename,newname);
	//		continue;
            //若是request, 把requst和response一起移动
            if (filename[len-1]=='t'&&is_file_ready(filename,atoi(argv[3]))) {
		//		while(pool->cur_queue_size > MAX_WORK_QUEUE){
		//			printf("pool->cur_queue_size over 1024*1024.\n");
		//			sleep(1);
		//		}
				sprintf(requestName, "%s",dirp->d_name);
				len = strlen(requestName);
				sprintf(responseName,"%s",requestName);
				strcpy(&responseName[len-strlen("request")], "response");


				
				sprintf(oldname, "%s%s", oldpath, responseName);
				sprintf(newname, "%s%s", newpath, responseName);
                rename(oldname, newname);

				sprintf(oldname, "%s%s", oldpath, dirp->d_name);
				sprintf(newname, "%s%s", newpath, dirp->d_name);
                rename(oldname, newname);

				
	//			printf("newname=%s\n",newname);
	//			files++;
                pool_add_worker(myprocess, newname,argv[2]);
            }
			else if(filename[len-1]=='e'&&is_file_ready(filename,atoi(argv[3]))){
				 sprintf(responseName,"%s",filename);
				 sprintf(requestName,"%s",filename);
				 len = strlen(requestName);
	       		 strcpy(&(requestName[len-strlen("response")]), "request");
	        	//printf("%s\n",filename_request);
	        	if(access(requestName, F_OK)==-1)
	        	{
	        		unlink(responseName);
	        	}  
			}
			
        }
	//	allfile+=files;
	//	printf("files:%u\n",files);
	//	printf("allfile:%u\n",allfile);
	//	files=0;
        closedir(dp);
		usleep(1000);
    }
	return 0;
}





