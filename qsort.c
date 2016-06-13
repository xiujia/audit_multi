#include "qsort.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



 Node * createNode( elemtype elem){
	Node * node =(Node *)malloc(sizeof(Node));
	if(!node) return NULL;
	node->elem = elem;
	node->next = NULL;
	return node;
}

 void addLink(Link *head,Link *node,unsigned int * len){
	node->next = head->next;
	head->next = node;
	(*len)++;
}
 void delLink(Link *head){
	Node * node;
	int i = 0;
	node = head->next;
	while(node){
		head->next = node->next;
		node->next = NULL;
		free(node);
		node = head->next;
//		printf("i = %d\n",++i);
	}
}


 elemtype *  createStr(Link *head,unsigned int len){
	if(head->next == NULL) return NULL;
	elemtype * string;
	Link *node;
	unsigned int i = 0;
	string = (elemtype  *)malloc(len*sizeof(elemtype));
//	printf("len  =   %u",len);
	memset(string,0,len*sizeof(elemtype));
	node  = head->next;
	while(node){
		string[i++] = node->elem; 
		node = node->next;
	}
	return string;
}


/*void qsort(int queue[],int sorted[],int len){
	;
}
*/
int cmp ( const void *a , const void *b ){
	return *(unsigned long *)a - *(unsigned long*)b;
}


/*
int main(int argc ,char **argv){
	elemtype  i;
	elemtype *str;
	Node * node;
	int num= atoi(argv[1]);
	printf("num:%d\n",num);
	for(i = 0;i< num;i++){
		node = createNode(i);
		addLink(&head,node);
	}
	printf("nodeNum = %d\n",nodeNum);
	str = push(&head);

	qsort((void * )str,nodeNum,sizeof(elemtype),cmp);
	for(i = 0;i<num;i++){
		printf("%lu\n",str[i]);
	}
	delLink(&head);
	free(str);
	return 0;
}
*/
