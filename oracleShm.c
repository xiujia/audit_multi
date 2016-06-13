#include <sys/shm.h>

#define HASH_TABLE_SIZE 10240
#define FREE_POOL_SIZE 5120
#define ORACLE_SHMEM_KEY_1   1018   /* 共享内存键值 */
#define ORACLE_SHMEM_MODE  0666    /* 共享内存权限值 */

// 97byte
//sql语句, 列信息, 列值格式串这3个数据量有可能比较大, 放在redis里
//其他信息放内存
//老化链: 每处理1个文件, 就先处理老化的结点
//hash表: 保存一个select过程的数据
struct SHM_NODE{
    unsigned int hash;
    unsigned int src_ip;
    unsigned int src_port;
    char src_mac[16];  /* src_mac和dst_mac以后若要改格式的话, 要注意防止越界, 现在是12字节 */
    unsigned int dst_ip;
    unsigned int dst_port;
    char dst_mac[16];
    unsigned long capture_time;
    unsigned long interval_time;
    unsigned int line_num;
    unsigned char dir_id;
    long selectPktCaptureTime; /* select文件时间 和 最后一个返回文件的时间 之差是select间隔时间 */
    long lastResutlPktCaptureTime;
    time_t savetime;

    /* 用于free链和hash表的冲突链 */
    /* hash冲突链不会长, hash取值好的话, 链最多就1个, 用单链即可 */
    struct SHM_NODE *next;

    /* old是记录所有的流, 那么链会很长, 定位1个结点并删除它, 需要其前驱, 若是单向链, 需要从头开始遍历, 容易耗时长 */
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

/* 创建共享内存，成功返回共享内存头地址，失败返回NULL */
void *doCreateOracleShm(key_t key, size_t size, int oflag) {
	int ret = shmget(key, size, oflag);
	if(-1 == ret){
	    return NULL;
	}
	return shmat( ret, 0, 0 );
}

/* 打开共享内存，成功返回共享内存头地址，失败返回NULL */
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

/* 新建CSP策略共享内存 */
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

