#include"csp_policy.h"

/* ��չ����ڴ������CSP��Ʋ��� */
int main(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Needs no argv\n");
        return -1;
    }
    return csp_audit_policy_clr();
}
