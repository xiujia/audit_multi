#include <stdio.h>
#include <stdlib.h>
#include"csp_policy.h"

/* 删除共享内存中指定id值的CSP审计策略，一次仅限1条
 * 必须带有1个或多个id值作为命令行参数
 * 目前只第1个参数有效 */
int main(int argc, char **argv)
{
    int id, i;
    CACHE_POLICY_CONF *shm;
    CSP_AUDIT_POLICY *cp;

    if (argc < 2) {
        fprintf(stderr, "ERROR: Need one argument as <id>\n");
	    return -1;
    }

    id = atoi(argv[1]);
    if (id < 1 || id > 10) {
        fprintf(stderr, "error : %s : Wrong Argument(id=(1...10))\n", argv[0]);
	    return -1;
    }

    return csp_audit_policy_del(id);
}
