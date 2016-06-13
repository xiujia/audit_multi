#include <stdio.h>
#include <stdlib.h>
#include"csp_policy.h"

/* ɾ��ָ��id�ĸ澯���� */
int main(int argc, char **argv)
{
    int id;

    if (argc != 2){
        fprintf(stderr, "ERROR: Need one argument as <id>\n");
	    return -1;
    }

    id = atoi(argv[1]);
    if (id < 1 || id > 10) {
        fprintf(stderr, "error : %s : Wrong Argument(id=(1...10))\n", argv[0]);
	    return -1;
    }
    return alarm_policy_del(id);
}
