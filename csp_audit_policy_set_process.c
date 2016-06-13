#include <stdio.h>
#include"csp_policy.h"

/* ��MySQL�б����CSP��Ʋ������빲���ڴ� */
int main(int argc, char **argv)
{
    MYSQL my;
    int ret;

    if (argc > 1) {
        fprintf(stderr, "Needs no argv\n");
        return -1;
    }

    ret = connect_mysql(&my);
    if (ret < 0) {
        fprintf(stderr, "error : %s : Fails to connect to MySQL\n", argv[0]);
	    return -1;
    }
    ret = csp_audit_policy_set(&my);

    close_mysql(&my);

    return ret;
}


