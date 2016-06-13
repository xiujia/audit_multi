#include <stdio.h>
#include <stdlib.h>
#include"csp_policy.h"

/* 添加指定id的告警策略
 * 命令行格式 - <command> id，id参数不可缺少 */
int main(int argc, char **argv)
{
    MYSQL my;
    int ret, id;

    if (argc != 2) {
        fprintf(stderr, "ERROR: %s Need an argument as <id>\n", argv[0]);
	    return -1;
    }

    id = atoi(argv[1]);
    if (id < 1 || id > 10) {
        fprintf(stderr, "error : %s : Wrong Argument(id=(1...10))\n", argv[0]);
	    return -1;
    }

    ret = connect_mysql(&my);
    if (ret < 0){
        fprintf(stderr, "error : %s : Fails to connect to MySQL\n", argv[0]);
	    return -1;
    }
    ret = alarm_policy_add(&my, id);

    close_mysql(&my);

    return ret;
}

