#include <stdio.h>
#include <stdlib.h>
#include"csp_policy.h"

/* 把MySQL中指定id的csp审计策略填入共享内存
 * 必须带有1个或多个id值作为命令行参数
 * 目前只第1个参数有效 */
int main(int argc, char **argv)
{
    MYSQL my;
    int ret, id;

    if (argc < 2) {
        fprintf(stderr, "error : %s : Needs an argument as (id)\n", argv[0]);
	    return -1;
    }

    id = atoi(argv[1]);
    if (id < 1 || id > 10) {
        fprintf(stderr, "error : %s : Wrong Argument(id=(1...10))\n", argv[0]);
	    return -1;
    }

    ret = connect_mysql(&my);
    if (ret < 0) {
        fprintf(stderr, "error : %s : Fails to connect to MySQL\n", argv[0]);
	    return -1;
    }
    ret = csp_audit_policy_add(&my, id);

    close_mysql(&my);

    return ret;
}

