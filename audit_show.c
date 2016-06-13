#include "audit_api.h"
//#include "audit_policy.h"


static void * get_audit_shm_main(key_t key){
		int shm_size;
		int ret;
		unsigned char * sh;
		shm_size=sizeof(struct copy_packet_counter);
		shm_size+=sizeof(struct audit_packet_slot)*AUDIT_PACKET_CACHE;
//		ret = shmget(AUDIT_PACKET_KEY, shm_size, IPC_CREAT); //
//		shmctl(ret, IPC_RMID, 0 );
		ret = shmget(key, shm_size, 0666|IPC_EXCL); 
		if(ret == -1 ){
			return NULL;
		}
		sh = (unsigned char * )shmat( ret, 0, 0 );

		return sh;
}

int main(int argc, char ** argv){
	int t,i;
	key_t key[10]={1020,1021,1022,1023,1024,1025,1026,1027,1028,1029};
	t = atoi(argv[1]);
	i = atoi(argv[2]);
//	Taudit_policy_mem * policy;
	unsigned char * audit_shm;
//	policy = get_audit_policy_shmem();
	audit_shm = get_audit_shm_main(key[i]);
	struct copy_packet_counter * copy_count;
	copy_count =(struct copy_packet_counter *)audit_shm;
	while(1){
		printf("*****************************\n");
//		printf("policy_stat:%d\n",policy->policy_stat);
//		printf("policy_count:%lu\n",policy->policy_count);
		printf("write:%lu   read:%lu\n",copy_count->write,copy_count->read);
		printf("write - read = %d\n",copy_count->write-copy_count->read);
		printf("**************************** *\n");
		sleep(t);
	}
	
	


}
