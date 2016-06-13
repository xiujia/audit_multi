
#include <dirent.h>
#include "pthread_pool.h"
#include "audit_ensemble.h"



static CThread_pool * pool=NULL;
#define MAX_WORK_QUEUE  1048576

struct worker WORK[MAX_WORK_QUEUE];
/*���̳߳��м�������*/  
int  
pool_add_worker (void *(*process) (char *,AUDIT_MULTI_THREAD_PARM * ), char *arg,char * dir)  
{  

/*
	if(!newworker){
		printf("newworker malloc failed.\n");
		return -1;
	}
	*/
 
 //   newworker->next = NULL;/*�����ÿ�*/  
  	
    pthread_mutex_lock (&(pool->queue_lock));  

	while(pool->cur_queue_size == MAX_WORK_QUEUE&& !pool->shutdown){
	//	printf("main thread waiting...\n");
		pthread_cond_wait (&(pool->queue_full_ready), &(pool->queue_lock));	
	}
    /*��������뵽�ȴ�������*/  
//    CThread_worker *member = pool->queue_head;  
//	CThread_worker *tail = pool->queue_tail;
    /*����һ��������*/  
    CThread_worker *newworker = pool->queue_head+pool->write;
	pool->write++;
	if(pool->write == MAX_WORK_QUEUE){
		pool->write=0;
	}
    newworker->process = process;  
    sprintf(newworker->fname,"%s",arg);


/*	if(tail){
		tail->next = newworker;
		pool->queue_tail = newworker;
	}
    else  
    {  
        pool->queue_head = newworker;
		pool->queue_tail = newworker;
    }  
  
    assert (pool->queue_head != NULL);   
	assert (pool->queue_tail != NULL);  
	*/
  
    pool->cur_queue_size++;  
//	printf("dir %s thread %u cur_queue_size:%d\n",dir,getpid(),pool->cur_queue_size);
    pthread_mutex_unlock (&(pool->queue_lock));  
    /*���ˣ��ȴ��������������ˣ�����һ���ȴ��̣߳� 
    ע����������̶߳���æµ�����û���κ�����*/  
    pthread_cond_signal (&(pool->queue_ready));  
    return 0;  
}  
  
  
  
/*�����̳߳أ��ȴ������е����񲻻��ٱ�ִ�У������������е��̻߳�һֱ 
����������������˳�*/  
int  
pool_destroy ()  
{  
    if (pool->shutdown)  
        return -1;/*��ֹ���ε���*/  
    pool->shutdown = 1;  
  
    /*�������еȴ��̣߳��̳߳�Ҫ������*/  
    pthread_cond_broadcast (&(pool->queue_ready));  
  
    /*�����ȴ��߳��˳�������ͳɽ�ʬ��*/  
    int i;  
    for (i = 0; i < pool->max_thread_num; i++)  
        pthread_join (pool->threadid[i], NULL);  
    free (pool->threadid);  
  
    /*���ٵȴ�����*/  
	/*
    CThread_worker *head = NULL;  
    while (pool->queue_head != NULL)  
    {  
        head = pool->queue_head;  
        pool->queue_head = pool->queue_head->next;  
        free (head);  
    } */ 
    /*���������ͻ�����Ҳ����������*/  
    pthread_mutex_destroy(&(pool->queue_lock));  
    pthread_cond_destroy(&(pool->queue_ready));  
    pthread_cond_destroy(&(pool->queue_full_ready));  
      
    free (pool);  
    /*���ٺ�ָ���ÿ��Ǹ���ϰ��*/  
    pool=NULL;  
    return 0;  
}  
  
  
  
void *  
thread_routine (void *arg)  
{  
	int thid;
	thid =((AUDIT_MULTI_THREAD_PARM *)arg)->mp->thread_id;
 //   printf ("init thread %u,thread_id %d \n", pthread_self (),thid);  
 //	sleep(1);
    while (1)  
    {  
  //      uleep(100);
        pthread_mutex_lock (&(pool->queue_lock));  
//		printf ("before thread %u cur_queue_size %d  \n", pthread_self (),pool->cur_queue_size); 
        /*����ȴ�����Ϊ0���Ҳ������̳߳أ���������״̬; ע�� 
        pthread_cond_wait��һ��ԭ�Ӳ������ȴ�ǰ����������Ѻ�����*/  
        while (pool->cur_queue_size == 0 && !pool->shutdown)  
        {  
       //     printf ("thread 0x%x is waiting\n", pthread_self ());  
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));  
        }  
  
 //		 printf ("after thread %u cur_queue_size %d	\n", pthread_self (),pool->cur_queue_size); 
        /*�̳߳�Ҫ������*/  
        if (pool->shutdown)  
        {  
            /*����break,continue,return����ת��䣬ǧ��Ҫ�����Ƚ���*/  
            pthread_mutex_unlock (&(pool->queue_lock));  
      //      printf ("thread 0x%x will exit\n", pthread_self ());  
            pthread_exit (NULL);  
        }  
  
   //     printf ("thread 0x%x is starting to work\n", pthread_self ());  
  
        /*assert�ǵ��Եĺð���*/  
        assert (pool->cur_queue_size != 0);  
    //    assert (pool->queue_head != NULL);  
          
        /*�ȴ����г��ȼ�ȥ1����ȡ�������е�ͷԪ��*/  
        pool->cur_queue_size--;  
		CThread_worker *worker;
        worker = pool->queue_head + pool->read;
		pool->read++;
		if(pool->read == MAX_WORK_QUEUE){
			pool->read = 0;
		}
/*
		pool->queue_head = worker->next;
		if(pool->cur_queue_size == 0){
			pool->queue_tail = NULL;
		}
		*/
        pthread_mutex_unlock (&(pool->queue_lock));  
 		pthread_cond_signal (&(pool->queue_full_ready));  
        /*���ûص�������ִ������*/  
        (*(worker->process)) (worker->fname,(AUDIT_MULTI_THREAD_PARM*)arg);  
  //      free (worker);  
   //     worker = NULL;  
    }  
    /*��һ��Ӧ���ǲ��ɴ��*/  
    pthread_exit (NULL);  
}  

int parm_init(int max_thread_num){
	int i=0;

	mp = (struct multi_parm *)malloc(max_thread_num*sizeof(struct multi_parm));
	if(!mp){
		return -5;
	}
	memset(mp,0,sizeof(*mp));
	parm  = (AUDIT_MULTI_THREAD_PARM *)malloc(max_thread_num*sizeof(AUDIT_MULTI_THREAD_PARM));
	if(!parm){
		printf("malloc AUDIT_MULTI_THREAD_PARM failed.\n");
		return -1;
	}
	memset(parm,0,sizeof(*parm));

	parm[0].policy  = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm(); 
	if(!parm[0].policy){
		printf("thread %hu get_audit_policy memory failed.\n",i);
		return -3;
	}
  
    for (i = 0; i < max_thread_num; i++)  
    {   
		if(i>0){
			parm[i].policy = parm[0].policy;
		}
		mp[i].conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
		if(mp[i].conn->err){
			redisFree(mp[i].conn);
			mp[i].conn = NULL;
			printf("thread %hu can not connect  to redis server.\n",i);
			return -4;					
		}
		mp[i].thread_id = i;

		mp[i].rel = (AUDIT_ENSEMBLE_REL*)malloc(sizeof(AUDIT_ENSEMBLE_REL));
		if(!mp[i].rel){
			printf("thread %hu malloc rel failed.\n",i);
			return -4;
		}
		memset(mp[i].rel,0,sizeof(AUDIT_ENSEMBLE_REL));
		
		mp[i].rel->operation = (char *)malloc(REQ_RES_LEN);
		if(!mp[i].rel->operation){
			printf("thread %hu malloc rel->operation failed.\n",i);
			return -6;
		}
		memset(mp[i].rel->operation,0,REQ_RES_LEN);
		
		mp[i].rel->response = (char *)malloc(REQ_RES_LEN);
		if(!mp[i].rel->response){
			printf("thread %hu malloc rel->response failed.\n",i);
			return -7;
		}
		memset(mp[i].rel->response,0,REQ_RES_LEN);
		parm[i].mp = &mp[i];
    }  
	return 0;
}
void parm_destroy(int max_thread_num){
	int i=0;

	for(i = 0;i < max_thread_num;i++){
		parm[i].mp = NULL;
		if(mp[i].conn){
			redisFree(mp[i].conn);
			mp[i].conn=NULL;
		}
		if(mp[i].rel->operation){
			free(mp[i].rel->operation);
			mp[i].rel->operation=NULL;
		}
		if(mp[i].rel->response){
			free(mp[i].rel->response);
			mp[i].rel->response=NULL;
		}
		if(mp[i].rel){
			free(mp[i].rel);
			mp[i].rel=NULL;
		}
		if(i>0){
			parm[i].policy = NULL;
		}
	}
	if(parm[0].policy){
		shmdt((const void *)parm[0].policy);
		parm[0].policy = NULL;
	}
	if(mp){
		free(mp);
		mp=NULL;
	}
	if(parm){
		free(parm);
		parm=NULL;
	}
}

int  
pool_init (int max_thread_num)  
{  
	unsigned short i = 0;
//	int r,flag = 0;
	if(max_thread_num > 20){
		printf("max 20 threads!\n");
		return -1;
	}
    pool = (CThread_pool *) malloc (sizeof (CThread_pool));  

	if(!pool){
		return -1;
	}
    pthread_mutex_init (&(pool->queue_lock), NULL);  
    pthread_cond_init (&(pool->queue_ready), NULL);  
    pthread_cond_init (&(pool->queue_full_ready), NULL);  
 
  
    pool->max_thread_num = max_thread_num;  
    pool->cur_queue_size = 0;  
  
    pool->shutdown = 0;  
  
    pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t));  
	if(!pool->threadid){
		free(pool);
		pool = NULL;
		return -1;
	}

	pool->queue_head = WORK;
	pool->write = 0;
	pool->read = 0;
	
	if(parm_init(max_thread_num)!=0){
		parm_destroy(max_thread_num);
		pool_destroy();
		return -2;
	}
	
    for (i = 0; i < max_thread_num; i++) {
		 pthread_create (&(pool->threadid[i]), NULL, thread_routine,(void * )&parm[i]); 
	}
	return 0;
} 
  
//    �����ǲ��Դ���  
/*
void *  
myprocess (char *arg) 
{  
	double  b=1,i=1,k=1;
   // printf ("threadid is 0x%x,file %s processing\n", pthread_self(),arg);  
//	unlink(arg);
   for(b=1;b<1000000;b++){
	k = 1;
	k++;
	k--;
     for(i=1;i<100;i++){
        k = k*i;
//      printf("k=%lf\n",k);
     }

  }	
 //   printf ("threadid is 0%d,file %s processing , k = %lf \n", pthread_self(),arg,k);  
   printf("threadid is %d ,k=%d\n",pthread_self(),k);
       
	
return NULL;  
}  


*/





#if 0
int  
main (int argc, char **argv)  
{  
    pool_init (100);/*�̳߳������������߳�*/  
      
    /*���������Ͷ��10������*/  
//    int *workingnum = (int *) malloc (sizeof (int) * 10);  
    int i;  
    for (i = 0; i < 10; i++)  
    {  
        workingnum[i] = i;  
        pool_add_worker (myprocess, &workingnum[i]);  
    }  
    /*�ȴ������������*/  
    sleep (5);  
    /*�����̳߳�*/  
    pool_destroy ();  
  
//    free (workingnum);  
    return 0;  
} 
#endif
