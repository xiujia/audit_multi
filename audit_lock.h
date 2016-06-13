#ifndef _AUDIT_LOCK
#define _AUDIT_LOCK

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



int file_set_lock(int fd, int cmd);
int file_get_lock(int fd);
int file_setlease(int fd, int cmd);



#endif
