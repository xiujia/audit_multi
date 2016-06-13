#include <stdio.h>
#include <stdlib.h>


struct list_head{
	struct list_head *prev,*next;
};


#define list_entry(ptr,type,member)\
                container_of(ptr,type,member)

 

#define offsetof(TYPE,MEMBER) ((size_t)&((TYPE *)0)->MEMBER)

#define container_of(ptr,type,member) ( {\
        const typeof( ((type*)0)->member ) *__mptr=(ptr);\
        (type*)( (char*)__mptr - offsetof(type,member) );} )




