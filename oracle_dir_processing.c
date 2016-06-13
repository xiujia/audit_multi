 /*
2015-12-15:
1. ��ɾ���ļ��Ķ����Ƶ�������������ļ����漰ɾ���ļ�������

2015-12-17:
1. ��main()��ɾ���ļ���system()��Ϊunlink()
*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <sched.h>

#include "/usr/src/inp/audit_new/oracle1521_30_processing.c"
#include "audit_release.h"

#define MAX_FILE_NUM 200000

char fileNames[MAX_FILE_NUM][128]; /*  */
char *pfnames[MAX_FILE_NUM];

int daemon_init(void) {
    pid_t pid;
    if((pid = fork()) < 0)
        return(-1);
    else if(pid != 0)
        exit(0); /* parent exit */

    /* child continues */
    setsid(); /* become session leader */
    chdir("/"); /* change working directory */
    umask(0); /* clear file mode creation mask */
    close(0); /* close stdin */
    //close(1);
    //close(2);
    return(0);
}

int isFileReady(char *filename) {
    time_t now;
    struct stat statbuf;
    if (stat(filename, &statbuf) == -1) {
        return -1;
    }
    time(&now);
    if(now-statbuf.st_mtime > 5) return 1;
    else if(statbuf.st_mtime-now > 5) return 1;
    return 0;
}

int cmp(const void *p, const void *q) {
    return memcmp(*(char **)p, *(char **)q, 16);
}

//ֻ��3���ܰ󶨣�1U����4����0123��2U��8����01234567
int BindCpu(int toBind){
	cpu_set_t mask;
	CPU_ZERO(&mask);    //�ÿ�
	#if U2_DEV
        toBind %= 8;
		if(toBind == 3){
			toBind++;
		}
    #else
	    if(toBind == 3){
	        toBind--;
	    }
	#endif

   	CPU_SET(toBind,&mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//�����߳�CPU�׺���
    {
//       printf("warning: could not set CPU affinity!\n");
	   return -1;
    }
//	printf("set CPU affinity success!\n");
	return 0;
}


/* <1>ÿsleep(1)����1��Ŀ¼
   <2>����..��.�Թ�
   <3>����δ����ʱ�޵��ļ�, ����
   <4>������response��request���ļ���, ����
   <5>��������ʱ�޵�request, ���������ļ�������, ɾ�����ļ�, ���򱣴��ļ���
   <6>��������ʱ�޵�response, ���������ļ�������, ɾ�����ļ�, ��������(��ֹ��α���request�ļ�)
   <7>ѭ��1��, �򱣴��ļ��������ﵽ����, ��رն�Ŀ¼, sleep(1)����ֹ��Ŀ¼ʱ��һֱѭ����Ŀ¼��ռ��cpu
   <8>�ļ�������
   <9>���������ļ���,  ���ε��ý������� */
int main(int argc, char **argv) {
    int i, count, dir_id;
    char dirname[128], curfile[256];
    char tempstr[1024], command[1024], curOppositeFile[1024];
    DIR *dirp;
    struct dirent *direntp;

#if 1
    /* ����Ŀ¼1-5 */
    if (argc != 3) {
        printf("process [1-5] [0/1]\n[1-5]specify path\n[0/1]:1=daemon\n");
        exit(-1);
    }
    dir_id = atoi(argv[1]);
    if (dir_id<1 || dir_id>5) {
        printf("process [1-5] [0/1]\n[1-5]specify path\n[0/1]:1=daemon\n");
        exit(-1);
    }
    sprintf(dirname, "/dev/shm/oracle/%s", argv[1]);
    BindCpu(dir_id);

    i = atoi(argv[2]);
    if (1 == i) daemon_init();
#else
    strcpy(dirname, "/home/oracle_debug/test");
#endif
    ORACLE_SHM_MEM * shm;
    shm = malloc(sizeof(ORACLE_SHM_MEM));
    if (!shm) {
        printf("cannot get shared memory\n");
        exit(-1);
    }
    memset(shm, 0, sizeof(ORACLE_SHM_MEM));
    int j;
    for (j=0; j<FREE_POOL_SIZE-1; j++) {
        shm->freePool[j].next = &(shm->freePool[j+1]);
    }
    shm->free = &(shm->freePool[0]);

    while(1) {
        for (i=0; i<MAX_FILE_NUM; i++) {
            pfnames[i] = fileNames[i];
        }
        count = 0;
        dirp = opendir(dirname);
        if (!dirp) {
            printf("opendir() failed.\n");
            exit(-1);
        }
        while ((direntp = readdir(dirp)) != NULL) {
            if (direntp->d_name[0] == '.') continue;

            sprintf(curfile, "%s/%s", dirname, direntp->d_name);
            if (!isFileReady(curfile)) continue;

            memset(tempstr, 0, sizeof(tempstr));
            if (strstr(direntp->d_name, "request") != NULL) {
                strncpy(tempstr, curfile, strlen(curfile)-7);
                sprintf(curOppositeFile, "%sresponse", tempstr);
                if (access(curOppositeFile, F_OK) == -1) {
                    unlink(curfile);
                }
                sprintf(pfnames[count++], "%s", direntp->d_name);
                if (count >= MAX_FILE_NUM-1) break;
            } else if (strstr(direntp->d_name, "response") != NULL) {
                strncpy(tempstr, curfile, strlen(curfile)-8);
                sprintf(curOppositeFile, "%srequest", tempstr);
                if(access(curOppositeFile, F_OK) == -1) {
                    unlink(curfile);
                }
            }
        }
        closedir(dirp);
        qsort(pfnames, count, sizeof(char*), cmp);

        for (i=0; i<count; i++) {
            sprintf(command, "%s/%s", dirname, pfnames[i]);
            mainoracle(command, shm);
        }

        sleep(1);
    }

    return 0;
}

