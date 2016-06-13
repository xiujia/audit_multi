#include <stdio.h>
#include <stdlib.h>
#include"csp_policy.h"

/* ��MySQL��ָ��id��csp��Ʋ������빲���ڴ�
 * �������1������idֵ��Ϊ�����в���
 * Ŀǰֻ��1��������Ч */
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

