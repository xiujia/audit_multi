#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stdlib.h>
#include <stdio.h>
#include "my_list.h"

#define MALLOC_BLOCK 1024*1024

typedef  int datatype;



typedef struct mem_node
{
	struct list_head node;
	datatype  mem;
}MEM_LIST;

typedef struct 
{
	pthread_mutex_t mem_pool_lock;
	pthread_cond_t  mem_pool_cond;

	MEM_LIST * mem_head;
	unsigned long pool_size;
	unsigned long used_size;

	struct list_head free_head;
	struct list_head used_head;
	
}MEM_POOL;

MEM_POOL * mem_pool;


#endif
