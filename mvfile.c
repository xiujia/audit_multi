#include "pthread_pool.h"
#include "audit_ensemble.h"
#include "csp_policy.h"


struct multi_parm *mp;
AUDIT_MULTI_THREAD_PARM *parm;
CThread_worker **wk;
pthread_t main_thread_id;

#define DATA_PATH	"/dev/shm/"

#define DES_PATH	"/dev/shm/tmp/"

int is_file_ready(char *filename)
{
	  time_t now;
	  struct stat statbuf;   
    if (stat(filename, &statbuf) == -1) 
    {   
      // printf("Get stat on %s Error：%s\n",  filename, strerror(errno));   
        return -1;   
    }   
    time(&now);
    if(now-statbuf.st_mtime>20)
    return 1;
    else 
    if(statbuf.st_mtime-now>20)
    return 1;
    else    
    return 0;   

	
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
	if(argc==3)
		NC_deamon_audit();
    DIR *dp;
	int r;
    struct dirent *dirp;
    char oldpath[256];
    char newpath[256];
	
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
//	sprintf(newpath,"%s%s/",DES_PATH,argv[2]);
	sprintf(newpath,"%s0/",DATA_PATH);
	mp = NULL;
	parm = NULL;
	pool = NULL;
	max = atoi(argv[1]);

	main_thread_id = getpid();
	
	
	r = pool_init(max);

	if(r != 0 ) {
		return -1;
	}

	
	
    while (1) {
        dp = opendir(oldpath);
        if(!dp) { continue; }
        while((dirp=readdir(dp))) {
            if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
                continue;
            }
            
			sprintf(filename,"%s%s",oldpath,dirp->d_name);
			len = strlen(filename);

			sprintf(newname,"%s%s",newpath,dirp->d_name);
			rename(filename,newname);
			continue;
            //若是request, 把requst和response一起移动
            if (filename[len-1]=='t'&&is_file_ready(filename)) {
				
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
			else if(filename[len-1]=='e'&&is_file_ready(filename)){
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
		sleep(10);
    }
	return 0;
}





