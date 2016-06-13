#include <sys/shm.h>

#define HASH_TABLE_SIZE 10240
#define FREE_POOL_SIZE 5120
#define ORACLE_SHMEM_KEY_1   1018   /* �����ڴ��ֵ */
#define ORACLE_SHMEM_MODE  0666    /* �����ڴ�Ȩ��ֵ */

// 97byte
//sql���, ����Ϣ, ��ֵ��ʽ����3���������п��ܱȽϴ�, ����redis��
//������Ϣ���ڴ�
//�ϻ���: ÿ����1���ļ�, ���ȴ����ϻ��Ľ��
//hash��: ����һ��select���̵�����
struct SHM_NODE{
    unsigned int hash;
    unsigned int src_ip;
    unsigned int src_port;
    char src_mac[16];  /* src_mac��dst_mac�Ժ���Ҫ�ĸ�ʽ�Ļ�, Ҫע���ֹԽ��, ������12�ֽ� */
    unsigned int dst_ip;
    unsigned int dst_port;
    char dst_mac[16];
    unsigned long capture_time;
    unsigned long interval_time;
    unsigned int line_num;
    unsigned char dir_id;
    long selectPktCaptureTime; /* select�ļ�ʱ�� �� ���һ�������ļ���ʱ�� ֮����select���ʱ�� */
    long lastResutlPktCaptureTime;
    time_t savetime;

    /* ����free����hash��ĳ�ͻ�� */
    /* hash��ͻ�����᳤, hashȡֵ�õĻ�, ������1��, �õ������� */
    struct SHM_NODE *next;

    /* old�Ǽ�¼���е���, ��ô����ܳ�, ��λ1����㲢ɾ����, ��Ҫ��ǰ��, ���ǵ�����, ��Ҫ��ͷ��ʼ����, ���׺�ʱ�� */
    struct SHM_NODE *oldnext;
    struct SHM_NODE *oldprev;
};

typedef struct {
    struct SHM_NODE freePool[FREE_POOL_SIZE];
    struct SHM_NODE *free;
    struct SHM_NODE *old;
    struct SHM_NODE *hashTable[HASH_TABLE_SIZE];
    int count;
}ORACLE_SHM_MEM;


#if 0
typedef struct {
    ORACLE_SHM_MEM shm[5];
}ORACLE_BUFFER;

/* ���������ڴ棬�ɹ����ع����ڴ�ͷ��ַ��ʧ�ܷ���NULL */
void *doCreateOracleShm(key_t key, size_t size, int oflag) {
	int ret = shmget(key, size, oflag);
	if(-1 == ret){
	    return NULL;
	}
	return shmat( ret, 0, 0 );
}

/* �򿪹����ڴ棬�ɹ����ع����ڴ�ͷ��ַ��ʧ�ܷ���NULL */
void *doGetOracleShm(key_t key, int oflag) {
	int ret = shmget(key, 0, oflag);
	if(-1 == ret){
	    return NULL;
	}
	return shmat( ret, 0, 0 );
}

void *getOracleShm(key_t key, int dir_id) {
    int i, j, size, ii;
	ORACLE_BUFFER *buf = doGetOracleShm(key, ORACLE_SHMEM_MODE);
    if (!buf) {
        fprintf(stderr, "%d:%dno shared memory\n", __LINE__, dir_id);
        exit(-1);
    }
    printf("getOracleShm:%d:%llu\n", buf, dir_id);
    ORACLE_SHM_MEM *shm;

    shm = &(buf->shm[dir_id-1]);
    memset(shm, 0, sizeof(ORACLE_SHM_MEM));
    for (j=0; j<FREE_POOL_SIZE-1; j++) {
        shm->freePool[j].next = &(shm->freePool[j+1]);
    }
    shm->free.next = &(shm->freePool[0]);
}

/* �½�CSP���Թ����ڴ� */
void *createOracleShm(key_t key, int dir_id) {
    ORACLE_BUFFER *buf;
    ORACLE_SHM_MEM *shm;
    int i, j, size, ii;

    size = sizeof(ORACLE_BUFFER);
    buf = doCreateOracleShm(key, size, (ORACLE_SHMEM_MODE | IPC_CREAT | IPC_EXCL));
    if (!buf) {
        fprintf(stderr, "%d:already exists\n", __LINE__);
        return NULL;
    }

    shm = &(buf->shm[dir_id-1]);
    memset(&(buf->shm[dir_id-1]), 0, sizeof(buf->shm[dir_id-1]));
    for (j=0; j<FREE_POOL_SIZE-1; j++) {
        shm->freePool[j].next = &(shm->freePool[j+1]);
    }
    shm->free.next = &(shm->freePool[0]);

    return buf;
}
#endif

