#ifndef _QSORT_H
#define _QSORT_H

typedef  unsigned long   elemtype ;

typedef  struct  _link{
	elemtype elem;
	struct _link  *next;
}Link,Node;



Node * createNode( elemtype elem);
void addLink(Link *head,Link *node,unsigned int * len);
void delLink(Link *head);
elemtype *	createStr(Link *head,unsigned int len);
int cmp ( const void *a , const void *b );



#endif
