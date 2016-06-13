#ifndef _DPI_H
#define _DPI_H
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
//#include <pthread.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <stdio.h>
//#include "policy.h"


#define APPDL_SOFILE	"/dhcc/forceview/lib/libfc.so"

void *dpi_match_callback(void *_param);
void *udapp_server(void *param);

void *thread_app_add(void *share_data);
void *thread_app_del(void *share_data);
int fst_pkt_match(unsigned int ssn_hash, char *eth);

#endif
