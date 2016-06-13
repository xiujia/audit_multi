#include <sys/shm.h>

#define HASH_TABLE_SIZE 2048
#define FREE_POOL_SIZE 2048
typedef struct __SHM_NODE{
    int hash;
    unsigned int srcip;
    unsigned int srcport;
    unsigned char srcmac[24];
    unsigned int dstip;
    unsigned int dstport;
    unsigned char dstmac[24];
    unsigned long capture_time;
    unsigned long interval_time;
    unsigned short line_num;
    int dir_id;
    struct __SHM_NODE *hashnext;
    struct __SHM_NODE *hashprev;
    struct __SHM_NODE *oldnext;
    struct __SHM_NODE *oldprev;
}SHM_NODE;

typedef struct {
    SHM_NODE freePool[FREE_POOL_SIZE];
    SHM_NODE *free;
    SHM_NODE *old;
    SHM_NODE *hashTable[HASH_TABLE_SIZE];
    int count;
}SHM_MEM;

#define ORACLE_SHMEM_KEY   1018   /* 共享内存键值 */
#define ORACLE_SHMEM_MODE  0666    /* 共享内存权限值 */

