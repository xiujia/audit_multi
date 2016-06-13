#include "audit_lock.h"



int file_set_lock(int fd, int cmd){
	struct flock lock;
	lock.l_whence=SEEK_SET;
	lock.l_start=0;
	lock.l_len=0;
	lock.l_pid = getpid();
	if(cmd == F_SETLK)
		lock.l_type=F_WRLCK;
//	printf("type:%d\n",lock.l_type);
	if(fcntl(fd,cmd,&lock)==-1)
		return -1;
	else return 1;
}

int file_get_lock(int fd){
	struct flock lock;
        lock.l_type = F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=0;
        lock.l_len=0;
	lock.l_pid=0;
	if(fcntl(fd,F_GETLK,&lock)==-1){	
		//printf("errno=%d\n",errno);
		//perror("fcntl");
		return -1;
	}
	else if(lock.l_type==F_WRLCK){
		return 1;
	}
	else return 0;	
	
}
int file_setlease(int fd, int cmd){
	if(fcntl(fd,cmd,F_WRLCK)==-1)
		return -1;
	else return 1;
}

