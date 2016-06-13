#ifndef AUDIT_MULTI_H
#define AUDIT_MULTI_H



#include "audit_ensemble.h"


struct multi_parm{
	unsigned short thread_id;
	redisContext * conn;
	AUDIT_ENSEMBLE_REL *rel;
};

typedef struct {
	struct multi_parm * mp;
	CACHE_POLICY_CONF * policy;
}AUDIT_MULTI_THREAD_PARM;


#if MULTI_THREADS

extern void * myprocess(char * ,AUDIT_MULTI_THREAD_PARM * );

#endif

#endif
