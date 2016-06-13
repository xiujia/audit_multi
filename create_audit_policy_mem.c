#include "csp_policy.h"

int main(int argc ,char ** argv){
	CACHE_POLICY_CONF * policy=NULL;
	policy = create_audit_cache_policy_shm();
	if(!policy){
		printf("audit_cache_policy_shm create fail!\n");
	}
	return 0;
}
