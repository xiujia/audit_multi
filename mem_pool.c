#include "mem_pool.h"
#include "my_list.h"


MEM_POOL * mem_pool;


void free_list_init(){
	int i=0;	
	INIT_LIST_HEAD(&mem_pool->free_head);
	for(i=mem_pool->pool_size;i>0;i--){
		list_add(mem_pool->mem_head[i].node,&mem_pool->free_head);
	}
}
void used_list_init(){
	INIT_LIST_HEAD(&mem_pool->used_head);
}
int is_empty(struct list_head *head){
	return (head->prev == head&&head->next == head); 
}

int mem_pool_init(){
	mem_pool = (MEM_POOL *)malloc(sizeof(*mem_pool));
	if(!mem_pool){
		printf("malloc mem_pool failed.\n");
		return -1; 
	}
	memset(mem_pool,0,sizeof(*mem_pool));
	
	mem_pool->mem_head = (MEM_LIST *)malloc(MALLOC_BLOCK*sizeof(MEM_LIST));
	if(!mem_pool->mem_head){
		printf("malloc mem_pool->mem_head failed.\n");
		return -1; 
	}
	memset(mem_pool->mem_head,0,MALLOC_BLOCK*sizeof(MEM_LIST));

	mem_pool->pool_size = MALLOC_BLOCK;
	mem_pool->used_size = 0;
	
	free_list_init();
	used_list_init();


    pthread_mutex_init (&(mem_pool->mem_pool_lock), NULL);  
    pthread_cond_init (&(mem_pool->mem_pool_cond), NULL);  	
	
}

int mem_pool_destroy(){
	free(mem_pool->mem_head);
	mem_pool->mem_head=NULL;
	pthread_mutex_destroy(&(mem_pool->mem_pool_lock));  
    pthread_cond_destroy(&(mem_pool->mem_pool_cond));  
	free(mem_pool);
	mem_pool=NULL;
}


datatype * my_malloc(){
	pthread_mutex_lock(&(mem_pool->mem_pool_lock));
	struct list_head * free_head = &mem_pool->free_head;
	struct list_head * used_head = &mem_pool->used_head;
	struct list_head * list_node;
	if(is_empty(free_head)){
		return NULL;
	//	pthread_cond_wait(&(mem_pool->mem_pool_cond), &(mem_pool->mem_pool_lock));	
	}
	list_node = free_head->next;
	list_del_entry(list_node); //从 free连上取第一个 脱链
	list_add(list_node,used_head);//加到 used 链上
	mem_pool->used_size++;
	pthread_mutex_unlock(&(mem_pool->mem_pool_lock));

	struct mem_node * m = list_entry(list_node,struct mem_node,node);
	retrun &(m->mem);
}

void * my_free(datatype * data){
	struct list_head * list_node;
	struct list_head * free_head;

	struct mem_node * m = list_entry(data,struct mem_node,mem);
	pthread_mutex_lock(&(mem_pool->mem_pool_lock));
	list_node = m->node;
	
	free_head = &mem_pool->free_head;
	used_head = &mem_pool->used_head;

	list_del_entry(list_node);//从used 上脱链
	
	list_add(list_node,free_head);//加到 free 链上
	mem_pool->used_size--;
	pthread_mutex_unlock(&(mem_pool->mem_pool_lock));
//	pthread_cond_signal(&(mem_pool->mem_pool_cond));
}

