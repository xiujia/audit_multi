#include"csp_policy.h"

/* 清空共享内存的所有CSP审计策略 */
int main(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Needs no argv\n");
        return -1;
    }
    return csp_audit_policy_clr();
}
