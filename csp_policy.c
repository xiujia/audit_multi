#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <errno.h>
#include"csp_policy.h"
#define __TEST


#define AUDIT_CACHE_POLICY_SHMEM_KEY   1040   /* �����ڴ��ֵ */
#if GA_TEST
#define AUDIT_CACHE_POLICY_SHMEM_KEY   1009
#endif
#define AUDIT_CACHE_POLICY_SHMEM_MODE 0666    /* �����ڴ�Ȩ��ֵ */

#define SHIFT 3
#define MASK  0x7
#define PRINT_ERR(s) fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, (s))

void bit_set(int i,u_int8_t a[]) {		  a[i>>SHIFT] |=  (1<<(i & MASK)); }
void bit_clr(int i,u_int8_t a[]) {		  a[i>>SHIFT] &= ~(1<<(i & MASK)); }
int  bit_test(int i,u_int8_t a[]){ return a[i>>SHIFT] &   (1<<(i & MASK)); }

/* ��д�����ɹ�����0��ʧ�ܷ���-1 */
int wr_lock(pthread_rwlock_t *lock) {
    int ret = pthread_rwlock_wrlock(lock);
    if (ret != 0) {
        if (EINVAL == ret) {
            PRINT_ERR("��д��δ��ʼ��");
        } else if (EDEADLK == ret) {
            PRINT_ERR("��ǰ�߳���ӵ�и���(������д��)");
        }
        return -1;
    }
    return 0; /* ��д���ɹ� */
}

/* ���д�����ɹ�����0��ʧ�ܷ���-1 */
int wr_unlock(pthread_rwlock_t *lock) {
    int ret = pthread_rwlock_unlock(lock);
    if (ret != 0) {
        if (EINVAL == ret) {
            PRINT_ERR("��д��δ��ʼ��");
        } else if (EPERM == ret) {
            PRINT_ERR("��δ����");
        }
        return -1;
    }
    return 0; /* �ͷ����ɹ� */
}

/* ���������ڴ棬�ɹ����ع����ڴ�ͷ��ַ��ʧ�ܷ���NULL */
void *create_audit_shm(key_t key, size_t size, int oflag) {
	int ret = shmget(key, size, oflag);
	if(-1 == ret){
	    return NULL;
	}
	return shmat( ret, 0, 0 );
}

/* �򿪹����ڴ棬�ɹ����ع����ڴ�ͷ��ַ��ʧ�ܷ���NULL */
void *get_audit_shm(key_t key, int oflag) {
	int ret = shmget(key, 0, oflag);
	if(-1 == ret){
	    return NULL;
	}
	return shmat( ret, 0, 0 );
}

/* �½�CSP���Թ����ڴ� */
void *create_audit_cache_policy_shm() {
    return create_audit_shm(AUDIT_CACHE_POLICY_SHMEM_KEY, sizeof(CACHE_POLICY_CONF), AUDIT_CACHE_POLICY_SHMEM_MODE | IPC_CREAT);
}

/* ��ȡCSP���Թ����ڴ� */
void *get_audit_cache_policy_shm() {
	return get_audit_shm(AUDIT_CACHE_POLICY_SHMEM_KEY, AUDIT_CACHE_POLICY_SHMEM_MODE);
}

/* ��ȡ��ѯ���
 * myres���ؽ�������ؽ��������ʧ�ܷ���-1 */
static int get_query_result(MYSQL *my, char *sqlstr, MYSQL_RES **myres) {
    int query_ret = mysql_real_query(my, sqlstr, strlen(sqlstr));
    if (query_ret != 0){
		PRINT_ERR("mysql_real_queryʧ��");
		if(mysql_errno(my)){
            PRINT_ERR(mysql_error(my));
		}
		return -1;
    }
    *myres = mysql_store_result(my);
    if (NULL == *myres){
		if(mysql_errno(my)){
            PRINT_ERR(mysql_error(my));
            return -1;
		}
        return 0;
    }
    return (int)mysql_num_rows(*myres);
}

/* csp�澯���Ե�time��֧�����policy_cfg_alarm_time����cfg_time
 * �ɹ�����0��ʧ�ܷ���-1 */
static int alarm_policy_time_branch(MYSQL *my, CSP_POLICY *policy) {
    int i, rows_num, time_start, time_end, time_span;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[256] = {0};

    sprintf(sqlstr,
        "SELECT cfg_time.time_start, cfg_time.time_end "
		"FROM policy_cfg_alarm_time JOIN cfg_time "
		"ON policy_cfg_alarm_time.time_id=cfg_time.id "
		"WHERE policy_cfg_alarm_time.alarm_id = %d", policy->id); /* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);

	for (i=0; i<rows_num; i++) {
        row = mysql_fetch_row(myres);
        if (NULL == row) {
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }

        time_start = atoi(row[0]);
        time_end = atoi(row[1]);

#ifdef __TEST
        fprintf(stderr, "id = %d, time_start=%d, time_end=%d\n", policy->id, time_start, time_end);
#endif

        if (time_start < 0) {
            time_start = 0;
        }else if (time_start > 10080) {
            time_start = 10080;
        }

        if (time_end < 0){
            time_end = 0;
        }else if (time_end > 10080) {
            time_end = 10080;
        }

        for(time_span=time_start; time_span<=time_end; time_span++) {
            if (time_span>=0 && time_span<=TIME_MAP_SIZE*8-8) {
	            bit_set(time_span, policy->time);
            }
        }
    }
    mysql_free_result(myres);
    return 0;
}

/* csp�澯���Ե�requesturl��֧��
 * ���policy_cfg_alarm_requesturl����cfg_monitor_web_requesturl
 * �ɹ�����0��ʧ�ܷ���-1 */
static int alarm_policy_requesturl_branch(MYSQL *my, CSP_POLICY *policy) {
    int iurl, iuser;
    int url_rows_num, user_rows_num;
    MYSQL_RES *urlres, *userres;
    MYSQL_ROW urlrow, userrow;
    char sqlstr[1024] = {0};

    sprintf(sqlstr,
        "SELECT cfg_monitor_web_requesturl.id, cfg_monitor_web_requesturl.request_url "
        "FROM policy_cfg_alarm_requesturl JOIN cfg_monitor_web_requesturl "
        "ON policy_cfg_alarm_requesturl. requesturl_id = cfg_monitor_web_requesturl.id "
        "WHERE policy_cfg_alarm_requesturl.alarm_id = %d", policy->id);/* С��Խ�� */
    url_rows_num = get_query_result(my, sqlstr, &urlres);

	policy->url_num = 0;
    for (iurl=0; iurl<url_rows_num; iurl++) {
        urlrow = mysql_fetch_row(urlres);
        if (NULL == urlrow) {
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���url_rows_num�����ݣ�������");
            return -1;
        }
        policy->url[iurl].requesturl_id = atoi(urlrow[0]);
        policy->url_num++;

        /* ��һ�����cfg_monitor_web_requesturl_account_relation */
        sprintf(sqlstr,
            "SELECT user_id "
            "FROM cfg_monitor_web_requesturl_account_relation "
            "WHERE url_id = %d", policy->url[iurl].requesturl_id);/* С��Խ�� */
        user_rows_num = get_query_result(my, sqlstr, &userres);

        for (iuser=0; iuser<user_rows_num; iuser++) {
            userrow = mysql_fetch_row(userres);
            if (NULL == userrow) {
                if(mysql_errno(my)) {
                    PRINT_ERR(mysql_error(my));
                }
                PRINT_ERR("ȡ�ò���user_rows_num�����ݣ�������");
                return -1;
            }

            if (atoi(userrow[0])>=0 && atoi(userrow[0])<=(URL_ACCOUNT_MAP_SIZE*8-8) ) {
	            bit_set(atoi(userrow[0]), policy->url[iurl].account);
            }
        }

        mysql_free_result(userres);

    }

    mysql_free_result(urlres);
    return 0;
}

/* csp�澯���Ե�account��֧�����policy_cfg_alarm_account
 * �ɹ�����0��ʧ�ܷ���-1 */
static int alarm_policy_account_branch(MYSQL *my, CSP_POLICY *policy) {
    int i, rows_num;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[256] = {0};

    sprintf(sqlstr,
        "SELECT account_id "
        "FROM policy_cfg_alarm_account "
        "WHERE alarm_id = %d", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);

    for (i=0; i<rows_num; i++) {
        row = mysql_fetch_row(myres);
        if (NULL == row) {
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            return -1;
        }
        if (atoi(row[0])>=0 && atoi(row[0])<=(URL_ACCOUNT_MAP_SIZE*8-8) ) {
            bit_set(atoi(row[0]), policy->account);
        }
    }

    mysql_free_result(myres);
    return 0;
}

/* csp�澯���Ե�operate��֧�����policy_cfg_alarm_operation
 * �ɹ�����0��ʧ�ܷ���-1 */
static int alarm_policy_operate_branch(MYSQL *my, CSP_POLICY *policy) {
    int i, rows_num;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[256] = {0};

    sprintf(sqlstr,
        "SELECT operation_id "
        "FROM policy_cfg_alarm_operation "
        "WHERE alarm_id = %d", policy->id);/* С��Խ�� */

    rows_num = get_query_result(my, sqlstr, &myres);

    for (i=0; i<rows_num; i++) {
        row = mysql_fetch_row(myres);
        if (NULL == row) {
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }
        if (atoi(row[0])>=0 && atoi(row[0])<=(OPERATE_MAP_SIZE*8-8)) {
            bit_set(atoi(row[0]), policy->operate);
        }
    }

    mysql_free_result(myres);
    return 0;
}

/* ����csp�澯���Եķ�֧���ԣ��ɹ�����0��ʧ�ܷ���-1 */
static int alarm_policy_branchs(MYSQL *my, CSP_POLICY *policy) {
    if (1 == policy->time_flag) {
        if (alarm_policy_time_branch(my, policy) < 0) return -1;
    }
    if (1 == policy->url_flag) {
        if (alarm_policy_requesturl_branch(my, policy) < 0) return -1;
    }
    else if (0 == policy->url_flag) {
        if (alarm_policy_account_branch(my, policy) < 0) return -1;
    }

    if (1 == policy->operate_flag) {
        if (alarm_policy_operate_branch(my, policy) < 0) return -1;
    }

    return 0;
}

/* ����csp�澯���ԣ��ɹ�����0��ʧ�ܷ���-1 */
int alarm_policy_set(MYSQL *my) {
    CACHE_POLICY_CONF *shm;
    CSP_POLICY *tmp;
    int i, policy_rows_num;
    MYSQL_RES *policyres;
    MYSQL_ROW policyrow;
    char sqlstr[] =
        "SELECT id, alarm_type, alarm_level, obj_id, time_flag, url_flag, account_flag, operate_flag "
        "FROM policy_cfg_alarm "
        "WHERE valid_flag=1";

    /* �ȰѲ��Ա��浽'tmp'���������ٽ������ķ�Χ */
    tmp = (CSP_POLICY *)calloc(CSP_POLICY_NUM, sizeof(CSP_POLICY));
    if (NULL == tmp) {
        PRINT_ERR("callocʧ��");
	    return -1;
    }

    policy_rows_num = get_query_result(my, sqlstr, &policyres);
    if (policy_rows_num < 0) {
        PRINT_ERR("���ݿ�û�鵽����");
        return -1;
    } else if (policy_rows_num > CSP_POLICY_NUM){
        PRINT_ERR("����CSP_POLICY_NUM������");
        return -1;
    }
    for (i=0; i<policy_rows_num && i<CSP_POLICY_NUM; i++) {
        policyrow = mysql_fetch_row(policyres);
        if (NULL == policyrow) {
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }

        /* ��ʱ������� */
        tmp[i].valid_flag = 1;
        tmp[i].id = atoi(policyrow[0]);
        tmp[i].alarm_type = atoi(policyrow[1]);
        tmp[i].alarm_level = atoi(policyrow[2]);
        tmp[i].obj_id = atoi(policyrow[3]);
        tmp[i].time_flag = atoi(policyrow[4]);
        tmp[i].url_flag = atoi(policyrow[5]);
        tmp[i].account_flag = atoi(policyrow[6]);
        tmp[i].operate_flag = atoi(policyrow[7]);
        tmp[i].url_num = 0;

        if (alarm_policy_branchs(my, tmp+i) < 0) return -1;
    }

    mysql_free_result(policyres);

    /* ��д���Թ����ڴ� */
    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
        free(tmp);
	    return -1;
    }

    if (wr_lock(&(shm->alarm_lock)) < 0) { /* ���� */
        PRINT_ERR("����ʧ��");
        free(tmp);
	    return -1;
    }

    memcpy(shm->cache_policy, tmp, sizeof(CSP_POLICY)*CSP_POLICY_NUM);
    shm->flag = (policy_rows_num > 0) ? 2 : 0;

    wr_unlock(&(shm->alarm_lock));
    free(tmp);
    return 0;
}

/* ����csp�澯���ԣ��ɹ�����0��ʧ�ܷ���-1 */
int alarm_policy_add(MYSQL *my, int id) {
    CACHE_POLICY_CONF *shm;
    CSP_POLICY tmp, *cp, *cur_policy, *first_empty;
    int i, policy_rows_num;
    MYSQL_RES *policyres;
    MYSQL_ROW policyrow;
    char sqlstr[256] = {0};

    /* �ȰѲ����ݿ⣬��'id'��Ӧ�ĸ�����������'tmp' */
    memset(&tmp, 0, sizeof(CSP_POLICY));
    sprintf(sqlstr,
        "SELECT id, alarm_type, alarm_level, obj_id, time_flag, url_flag, account_flag, operate_flag "
        "FROM policy_cfg_alarm "
        "WHERE valid_flag=1 and id=%d", id);
    policy_rows_num = get_query_result(my, sqlstr, &policyres);
    if (policy_rows_num < 1) {
        PRINT_ERR("���ݿ���û���ҵ���Ӧid����Ч�澯����");
	    return -1;
    } else if (policy_rows_num > 1) {
        PRINT_ERR("���ݿ����ж�����id����Ч�澯����");
	    return -1;
    }

    policyrow = mysql_fetch_row(policyres);
    if (NULL == policyrow){
        if(mysql_errno(my)){
            PRINT_ERR(mysql_error(my));
        }
        PRINT_ERR("���ݿ���û���ҵ���Ӧid����Ч����");
        return -1;
    }

    tmp.valid_flag = 1;
    tmp.id = atoi(policyrow[0]);
    tmp.alarm_type = atoi(policyrow[1]);
    tmp.alarm_level = atoi(policyrow[2]);
    tmp.obj_id = atoi(policyrow[3]);
    tmp.time_flag = atoi(policyrow[4]);
    tmp.url_flag = atoi(policyrow[5]);
    tmp.account_flag = atoi(policyrow[6]);
    tmp.operate_flag = atoi(policyrow[7]);
    tmp.url_num = 0;

    mysql_free_result(policyres);

    alarm_policy_branchs(my, &tmp);

    /* �Ѳ������빲���ڴ� */
    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
	    return -1;
    }
    cp = shm->cache_policy;

    if (wr_lock(&(shm->alarm_lock)) < 0) {
        PRINT_ERR("����ʧ��");
	    return -1;
    }

    cur_policy = first_empty = NULL;
    for (i=0; i<CSP_POLICY_NUM; i++) {
	    if (cp[i].id == id && 1 == cp[i].valid_flag) { /* �Ѵ�����Ч�ģ���id�Ĳ��� */
            cur_policy = &(cp[i]);
            continue;
        }
        if (NULL==first_empty && 0 == cp[i].valid_flag) {/* ��1����λ */
	        first_empty = &(cp[i]);
        }
    }

    if (NULL == cur_policy) {
        cur_policy = first_empty;
        if (NULL == cur_policy) {
            wr_unlock(&(shm->alarm_lock));
            PRINT_ERR("����������������");
            return -1;
        }
    }

    memcpy(cur_policy, &tmp, sizeof(CSP_POLICY));
    shm->flag = 2;/* ��ʾ�����б䶯 */

    wr_unlock(&(shm->alarm_lock));
    return 0;
}

/* ɾ��ָ��id�ĸ澯���ԣ��ɹ�����0��ʧ�ܷ���-1 */
int alarm_policy_del(int id) {
    int i, has_policy;
    CACHE_POLICY_CONF *shm;

    /* ��ȡ���Խṹ */
    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
	    return -1;
    }
    if (wr_lock(&(shm->alarm_lock)) < 0) {
        PRINT_ERR("����ʧ��");
	    return -1;
    }

    if (1 == shm->flag) {
        /* ɾ��ָ������ */
        for (i=0; i<CSP_POLICY_NUM; i++) {
            if (id == shm->cache_policy[i].id) {
                shm->cache_policy[i].valid_flag = 0;
#ifdef __TEST
                PRINT_ERR("�ҵ����ɹ�ɾ����id�Ĳ���");
#endif
                continue;
            }
            if (1 == shm->cache_policy[i].valid_flag) {
                has_policy = 1;
            }
        }

        shm->flag = has_policy ? 2 : 0;
    }

    wr_unlock(&(shm->alarm_lock));
    return 0;
}

/* ����csp��Ʋ��Ե�time��֧���ɹ�����0��ʧ�ܷ���-1 */
static int csp_audit_policy_time_branch(MYSQL *my, CSP_AUDIT_POLICY *policy) {
    int i, rows_num, time_start, time_end, time_span;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[256] = {0};

    sprintf(sqlstr,
        "SELECT cfg_time.time_start, cfg_time.time_end "
		"FROM policy_cfg_monitor_times JOIN cfg_time "
		"ON policy_cfg_monitor_times.time_id=cfg_time.id "
		"WHERE policy_cfg_monitor_times.monitor_id = %d", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);

	for (i=0; i<rows_num; i++) {
        row = mysql_fetch_row(myres);
        if (NULL == row){
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }

        time_start = atoi(row[0]);
        time_end = atoi(row[1]);

#ifdef __TEST
        fprintf(stderr, "id = %d, time_start=%d, time_end=%d\n", policy->id, time_start, time_end);
#endif

        if (time_start < 0) {
            time_start = 0;
        } else if (time_start > 10080) {
            time_start = 10080;
        }

        if (time_end < 0) {
            time_end = 0;
        } else if (time_end > 10080) {
            time_end = 10080;
        }


        for(time_span=time_start; time_span<=time_end; time_span++) {
            if (time_span>=0 && time_span<=TIME_MAP_SIZE*8-8) {
                bit_set(time_span, policy->time);
            }
        }
    }
    mysql_free_result(myres);
    return 0;
}

/* ����csp��Ʋ��Ե�ip��֧���ɹ�����0��ʧ�ܷ���-1 */
static int csp_audit_policy_user_branch(MYSQL *my, CSP_AUDIT_POLICY *policy, int n) {
    int i, rows_num;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[1024] = {0};
	u_int32_t ip_map_addr, start_ip_map_addr, end_ip_map_addr, iip0, iip1, iip2, iip3, ipidx;

    /* ���õ���ip����� */
    sprintf(sqlstr,
        "SELECT cfg_static_user.ip "
		"FROM policy_cfg_monitor_user JOIN cfg_static_user "
		"ON policy_cfg_monitor_user.user_id = cfg_static_user.user_id "
		"WHERE (cfg_static_user.type_flag=1 or cfg_static_user.type_flag=2) and "
		"policy_cfg_monitor_user.monitor_id = %d", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);

#ifdef __TEST
    if (0 == rows_num) {
        fprintf(stderr, "no single ip\n");
    }
#endif

    for (i=0; i<rows_num; i++) {
        row = mysql_fetch_row(myres);
        if (NULL == row){
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }

        /* ��ipλͼ��ֵȡ������ĺ�n�ֽ� */
        sscanf(row[0], "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);
        if(3 == n) {
            ip_map_addr = iip2*256*256 + iip1*256 + iip0;
        } else if(2 == n){
            ip_map_addr = iip1*256 + iip0;
        }

#ifdef __TEST
        fprintf(stderr, "id = %d, ip=%s, ip_map_addr=%d\n", policy->id, row[0], ip_map_addr);
#endif
        if (ip_map_addr >=0 && ip_map_addr<=(256*256*256-1)) {
	        bit_set(ip_map_addr, policy->ip);/* ip��λͼ */
        }
    }

    mysql_free_result(myres);

    /* ����ip�ε���� */
    sprintf(sqlstr,
        "SELECT cfg_static_user.start_ip, cfg_static_user.end_ip "
		"FROM policy_cfg_monitor_user JOIN cfg_static_user "
		"ON policy_cfg_monitor_user.user_id = cfg_static_user.user_id "
		"WHERE (cfg_static_user.type_flag=3 or cfg_static_user.type_flag=4) and "
		"policy_cfg_monitor_user.monitor_id = %d", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);

#ifdef __TEST
    if (0 == rows_num) {
        fprintf(stderr, "no ip range\n");
    }
#endif

    for (i=0; i<rows_num; i++) {
        row = mysql_fetch_row(myres);
        if (NULL == row){
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }

        /* ��ipλͼ��ֵȡ������ĺ�n�ֽ� */
        sscanf(row[0], "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);
        if(3 == n) {
            start_ip_map_addr = iip2*256*256 + iip1*256 + iip0;
        } else if(2 == n){
            start_ip_map_addr = iip1*256 + iip0;
        }
        sscanf(row[1], "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);
        if(3 == n) {
            end_ip_map_addr = iip2*256*256 + iip1*256 + iip0;
        } else if(2 == n){
            end_ip_map_addr = iip1*256 + iip0;
        }

#ifdef __TEST
        fprintf(stderr, "id = %d, start_ip=%s, end_ip=%s, start_ip_map_addr=%d, end_ip_map_addr=%d\n", policy->id, row[0], row[1], start_ip_map_addr, end_ip_map_addr);
#endif
        for (ipidx=start_ip_map_addr; ipidx<=end_ip_map_addr; ipidx++) {
            if (ipidx >=0 && ipidx<=(256*256*256-1)) {
                bit_set(ipidx, policy->ip);/* ip��λͼ */
            }
        }
    }

    mysql_free_result(myres);
    return 0;
}

/* ����csp��Ʋ��Ե�app_id��֧���ɹ�����0��ʧ�ܷ���-1 */
static int csp_audit_policy_app_id_branch(MYSQL *my, CSP_AUDIT_POLICY *policy) {
    int i, rows_num;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[256] = {0};

    sprintf(sqlstr,
        "SELECT app_id "
        "FROM policy_cfg_monitor_app_relation "
        "WHERE monitor_id = %d and flag=1", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);

    for (i=0; i<rows_num; i++){
        row = mysql_fetch_row(myres);
        if (NULL == row){
            if(mysql_errno(my)){
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }

#ifdef __TEST
        fprintf(stderr, "id = %d, app_id=%d\n", policy->id, atoi(row[0]));
#endif
        if(atoi(row[0])>=0 && atoi(row[0])<=8*8-1) {
            bit_set(atoi(row[0]), policy->app_id);/* app_id��λͼ */
        } else {
            fprintf(stderr, "Out of Band\n");
        }
    }

    mysql_free_result(myres);
    return 0;
}

/* ����csp��Ʋ��Ե�requesturl��֧���ɹ�����0��ʧ�ܷ���-1 */
static int csp_audit_policy_requesturl_branch(MYSQL *my, CSP_AUDIT_POLICY *policy) {
    int i, rows_num, have_urlid = 0;
    MYSQL_RES *myres;
    MYSQL_ROW row;
    char sqlstr[1024] = {0};

    sprintf(sqlstr,
        "SELECT requesturl_id "
        "FROM policy_cfg_monitor_requesturl "
        "WHERE monitor_id = %d", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);
    for (i=0; i<rows_num; i++){
        row = mysql_fetch_row(myres);
        if (NULL == row){
            if(mysql_errno(my)){
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }
        have_urlid = 1;

#ifdef __TEST
        fprintf(stderr, "id = %d, requesturl_id=%s, %d\n", policy->id, row[0], atoi(row[0]));
#endif
        if (atoi(row[0])>=0 && atoi(row[0])<=1000) {
	        bit_set(atoi(row[0]), policy->requesturl_id);/* requesturl_id��λͼ */
        }
    }
    mysql_free_result(myres);

    sprintf(sqlstr,
        "SELECT cfg_monitor_web_requesturl_group_relation.url_id "
        "FROM policy_cfg_monitor_requesturl_group join cfg_monitor_web_requesturl_group_relation "
        "ON policy_cfg_monitor_requesturl_group.requesturl_group_id = cfg_monitor_web_requesturl_group_relation.group_id "
        "WHERE policy_cfg_monitor_requesturl_group.monitor_id = %d", policy->id);/* С��Խ�� */
    rows_num = get_query_result(my, sqlstr, &myres);
    for (i=0; i<rows_num; i++){
        row = mysql_fetch_row(myres);
        if (NULL == row){
            if(mysql_errno(my)){
                PRINT_ERR(mysql_error(my));
            }
            PRINT_ERR("ȡ�ò���rows_num�����ݣ�������");
            return -1;
        }
        have_urlid = 1;
#ifdef __TEST
        fprintf(stderr, "id = %d, group requesturl_id=%s, %d\n", policy->id, row[0], atoi(row[0]));
#endif
        if (atoi(row[0])>=0 && atoi(row[0])<=1000) {
            bit_set(atoi(row[0]), policy->requesturl_id);/* url_id��λͼ */
        }
    }
    mysql_free_result(myres);

    /* ��û��url���ƵĲ���, ��urlλͼȫ��1 */
    policy->obj_url_flag = (0 == have_urlid) ? 0 : 1;

#ifdef __TEST
    fprintf(stderr, "id = %d, obj_url_flag=%d\n", policy->id, policy->obj_url_flag);
#endif
    return 0;
}

/* ����csp��Ʋ��Եķ�֧���ԣ��ɹ�����0��ʧ�ܷ���-1 */
static int csp_audit_policy_branchs(MYSQL *my, CSP_AUDIT_POLICY *policy) {
    int i;
    if (0 == policy->obj_user_flag) {
        if (csp_audit_policy_user_branch(my, policy, 3) < 0) return -1;
    } else if (1 == policy->obj_user_flag) {
        for (i=0; i<sizeof(policy->ip); i++) {
            policy->ip[i] = 0xff;
        }
    }

    if (0 == policy->obj_time_flag) {
        if (csp_audit_policy_time_branch(my, policy) < 0) return -1;
    } else if (1 == policy->obj_time_flag) {
        for (i=0; i<sizeof(policy->time); i++) {
            policy->time[i] = 0xff;
        }
    }

    if (csp_audit_policy_app_id_branch(my, policy) < 0) return -1;

    if (bit_test(1,policy->app_id) || bit_test(4,policy->app_id)) {
        if (csp_audit_policy_requesturl_branch(my, policy) < 0) return -1;
    }

    return 0;
}

/* ����csp��Ʋ��ԣ��ɹ�����0��ʧ�ܷ���-1 */
int csp_audit_policy_set(MYSQL *my) {
    CACHE_POLICY_CONF *shm;
    CSP_AUDIT_POLICY *tmp;
    int i, policy_rows_num;
    MYSQL_RES *policyres;
    MYSQL_ROW policyrow;

    #if GONG_AN_2015
    char sqlstr[] =
        " SELECT id, monitor_obj_id, obj_user_flag, obj_time_flag, priority, security_level "
        " FROM policy_cfg_monitor "
        " WHERE valid_flag=1 "
        " ORDER BY priority";
    #else
    char sqlstr[] =
        " SELECT id, monitor_obj_id, obj_user_flag, obj_time_flag, priority "
        " FROM policy_cfg_monitor "
        " WHERE valid_flag=1 "
        " ORDER BY priority";
    #endif

    /* �ȰѲ��Ա��浽'tmp'���������ٽ������ķ�Χ */
    tmp = (CSP_AUDIT_POLICY *)calloc(CSP_POLICY_NUM, sizeof(CSP_AUDIT_POLICY));
    if (NULL == tmp) {
        PRINT_ERR("callocʧ��");
	    return -1;
    }

    policy_rows_num = get_query_result(my, sqlstr, &policyres);
    if (policy_rows_num < 0) {
        PRINT_ERR("���ݿ�û�鵽����");
        return -1;
    } else if (policy_rows_num > CSP_POLICY_NUM){
        PRINT_ERR("����CSP_POLICY_NUM������");
        return -1;
    }
    for (i=0; i<policy_rows_num && i<CSP_POLICY_NUM; i++) {
        /* ��ȡһ�в�ѯ��� */
        policyrow = mysql_fetch_row(policyres);
        if (NULL == policyrow) {
            if(mysql_errno(my)) {
                PRINT_ERR(mysql_error(my));
            }
            return -1;
        }

        /* ��¼�ò��� */
        tmp[i].valid_flag = 1;
        tmp[i].id = atoi(policyrow[0]);
        tmp[i].monitor_obj_id = atoi(policyrow[1]);
        tmp[i].obj_user_flag = atoi(policyrow[2]);
        tmp[i].obj_time_flag = atoi(policyrow[3]);
        tmp[i].priority = atoi(policyrow[4]);
        #if GONG_AN_2015
        tmp[i].security_level = atoi(policyrow[5]);
        #endif

#ifdef __TEST
        fprintf(stderr, "seq = %d\n", i);
        fprintf(stderr, "id = %d\n", tmp[i].id);
        fprintf(stderr, "valid_flag = %d\n", tmp[i].valid_flag);
        fprintf(stderr, "monitor_obj_id = %d\n", tmp[i].monitor_obj_id);
        fprintf(stderr, "obj_time_flag = %d\n", tmp[i].obj_time_flag);
        fprintf(stderr, "obj_user_flag = %d\n", tmp[i].obj_user_flag);
        fprintf(stderr, "priority = %d\n", tmp[i].priority);
        #if GONG_AN_2015
        fprintf(stderr, "security_level = %d\n", tmp[i].security_level);
        #endif
#endif

	    if (csp_audit_policy_branchs(my, tmp+i) < 0) return -1;
    }
    mysql_free_result(policyres);

    /* ��д���Թ����ڴ� */
    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
        if (tmp != NULL) free(tmp);
	    return -1;
    }

    //if (wr_lock(&(shm->audit_lock)) < 0) {
    //    PRINT_ERR("����ʧ��");
    //    if (tmp != NULL) free(tmp);
	//    return -1;
    //}

    memcpy(shm->csp_audit_policy, tmp, sizeof(CSP_AUDIT_POLICY)*CSP_POLICY_NUM);
    shm->csp_audit_policy_visit_flag = (policy_rows_num > 0) ? 2 : 0;

    //wr_unlock(&(shm->audit_lock));
    if (tmp != NULL) free(tmp);

    return 0;
}

/* ����һ��ָ��id��csp��Ʋ��ԣ��ɹ�����0��ʧ�ܷ���-1 */
int csp_audit_policy_add(MYSQL *my, int id) {
    CACHE_POLICY_CONF *shm;
    CSP_AUDIT_POLICY tmp, *cp, *cur_policy, *first_empty;
    int i, policy_rows_num;
    MYSQL_RES *policyres;
    MYSQL_ROW policyrow;
    char sqlstr[256] = {0};

    /* �ȰѲ����ݿ⣬��'id'��Ӧ�ĸ�����������'tmp' */
    memset(&tmp, 0, sizeof(CSP_AUDIT_POLICY));
    sprintf(sqlstr,
        "SELECT id, monitor_obj_id, obj_user_flag, obj_time_flag "
        "FROM policy_cfg_monitor "
        "WHERE valid_flag=1 and id=%d", id);
    policy_rows_num = get_query_result(my, sqlstr, &policyres);
    if (policy_rows_num < 1) {
        PRINT_ERR("���ݿ���û���ҵ���Ӧid����Ч����");
	    return -1;
    } else if (policy_rows_num > 1) {
        PRINT_ERR("���ݿ����ж�����id����Ч����");
	    return -1;
    }

    policyrow = mysql_fetch_row(policyres);
    if (NULL == policyrow) {
        if(mysql_errno(my)) {
            PRINT_ERR(mysql_error(my));
        }
        PRINT_ERR("���ݿ���û���ҵ���Ӧid����Ч����");
        return -1;
    }

    tmp.valid_flag = 1;
    tmp.id = atoi(policyrow[0]);
    tmp.monitor_obj_id = atoi(policyrow[1]);
    tmp.obj_user_flag = atoi(policyrow[2]);
    tmp.obj_time_flag = atoi(policyrow[3]);

#ifdef __TEST
    fprintf(stderr, "id = %d\n", tmp.id);
    fprintf(stderr, "valid_flag = %d\n", tmp.valid_flag);
    fprintf(stderr, "monitor_obj_id = %d\n", tmp.monitor_obj_id);
    fprintf(stderr, "obj_time_flag = %d\n", tmp.obj_time_flag);
    fprintf(stderr, "obj_user_flag = %d\n", tmp.obj_user_flag);
#endif

    mysql_free_result(policyres);

    csp_audit_policy_branchs(my, &tmp);

    /* �Ѳ������빲���ڴ� */
    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
	    return -1;
    }
    cp = shm->csp_audit_policy;
    if ( wr_lock(&(shm->audit_lock)) < 0 ) { /* ���� */
        PRINT_ERR("����ʧ��");
	    return -1;
    }

    cur_policy = first_empty = NULL;
    for (i=0; i<CSP_POLICY_NUM; i++) {
	    if (cp[i].id == id && 1 == cp[i].valid_flag) { /* �Ѵ�����Ч�ģ���id�Ĳ��� */
            cur_policy = &(cp[i]);
            continue;
        }
        if (NULL==first_empty && 0 == cp[i].valid_flag) {/* ��1����λ */
	        first_empty = &(cp[i]);
        }
    }
    if (NULL == cur_policy) {
        cur_policy = first_empty;
        if (NULL == cur_policy){
            wr_unlock(&(shm->audit_lock));
            PRINT_ERR("����������������");
            return -1;
        }
    }

    memcpy(cur_policy, &tmp, sizeof(CSP_AUDIT_POLICY));
    shm->csp_audit_policy_visit_flag = 2;/* ��ʾ���²��� */

    wr_unlock(&(shm->audit_lock));
    return 0;
}

/* ɾ��ָ��id��csp��Ʋ��ԣ��ɹ�����0��ʧ�ܷ���-1 */
int csp_audit_policy_del(int id) {
    int i, has_policy;
    CACHE_POLICY_CONF *shm;

    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
	    return -1;
    }

    if (wr_lock(&(shm->audit_lock)) < 0) {
        PRINT_ERR("����ʧ��");
	    return -1;
    }

    if (1 == shm->csp_audit_policy_visit_flag) {
        /* ɾ��ָ������ */
        has_policy = 0;
        for (i=0; i<CSP_POLICY_NUM; i++) {
            if (id == shm->csp_audit_policy[i].id
                && 1 == shm->csp_audit_policy[i].valid_flag)
            {
                shm->csp_audit_policy[i].valid_flag = 0;
#ifdef __TEST
				PRINT_ERR("�ҵ����ɹ�ɾ����id�Ĳ���");
#endif
                continue;
            }
            if (1 == shm->csp_audit_policy[i].valid_flag) {
	            has_policy = 1;
            }
        }

        shm->csp_audit_policy_visit_flag = (has_policy) ? 2 : 0;
    }

    wr_unlock(&(shm->audit_lock));
    return 0;
}

/* �������csp��Ʋ��ԣ��ɹ�����0��ʧ�ܷ���-1 */
int csp_audit_policy_clr() {
    CACHE_POLICY_CONF *shm;

    shm = get_audit_cache_policy_shm();
    if (NULL == shm) {
        PRINT_ERR("��ù����ڴ�ʧ��");
	    return -1;
    }

    if ( wr_lock(&(shm->audit_lock)) < 0 ) {
        PRINT_ERR("����ʧ��");
	    return -1;
    }

    if (1 == shm->csp_audit_policy_visit_flag) {
        /* ��ղ��� */
        memset(shm->csp_audit_policy, 0, sizeof(CSP_AUDIT_POLICY)*CSP_POLICY_NUM);
        shm->csp_audit_policy_visit_flag = 0;
    }

    wr_unlock(&(shm->audit_lock));

    return 0;
}
/* --end--  */
