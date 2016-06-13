#include"csp_policy.h"

/* ÖØÔØ¸æ¾¯²ßÂÔ */
int main(int argc, char **argv)
{
    MYSQL my;
    int ret;

    if (argc > 1) {
        fprintf(stderr, "Needs no argv\n");
        return -1;
    }

    ret = connect_mysql(&my);
    if (ret < 0){
        fprintf(stderr, "ERROR: Failed to connect MySQL(%s)", mysql_error(&my));
	    return -1;
    }
    ret = alarm_policy_set(&my);

    close_mysql(&my);

    return ret;
}


