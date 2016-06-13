#include <stdio.h>
#include <stdlib.h>
#include"csp_policy.h"

/* ɾ�������ڴ���ָ��idֵ��CSP��Ʋ��ԣ�һ�ν���1��
 * �������1������idֵ��Ϊ�����в���
 * Ŀǰֻ��1��������Ч */
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
