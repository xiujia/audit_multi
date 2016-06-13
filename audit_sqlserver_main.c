#define _GNU_SOURCE

#include<ctype.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<ctype.h>
#include<sys/types.h>
#include<dirent.h>
#include<time.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/stat.h>
#include <sched.h>

#include "csp_deal.h"
#include "csp_policy.h"
#include "audit_release.h"
#include "audit_ensemble.h"
//#include "TDS_parser.c"
int TDS_parser(unsigned char *data, int len, char *resultStr, int maxsize, int *line_num);
/*#include "./redis_new_api.h"  2015-11-16,����snmp */

#define ITEM_MAX 1 /* �����ݿ���ļ��������������ֵ */
#define THREAD_MAX 20

static char curFileName[THREAD_MAX][256] = {0};/* ��ǰ�����ݿ���ļ�����ȫ·���� */
static char curDstFileName[THREAD_MAX][256] = {0};
static int itemCount[THREAD_MAX] = {0};/* ÿ�������ݿ���ļ������������ֵ */
static int fileNameSuffixN[THREAD_MAX] = {0};

/* ɾ��'str'��ͷ��ĩβ�����Ŀհ��ַ�. ԭַ����.���ؽ�����ĳ��� */
static int trim(char *str) {
    char *start = str, *end = str+strlen(str)-1;
    int num;

    if (!str || '\0'==str[0]) return 0;

    while (start<=end && isspace(*start)) start++;
    while (end > start && isspace(*end)) end--;

    if (start > end) {
        str[0] = '\0';
        num = 0;
    } else {
        if (start != str) {
            memmove(str, start, end-start+1);
        }
        str[end-start+1] = '\0';
        num = end-start+1;
    }

    return num;
}

/* ����'slen'�Ĵ�'s'��, 1���������Ķ�����ɴ�ӡ�ַ����һ���ո�, ��������浽'dst'��
 * ����'dst'�ַ����ĳ��� */
static int unprintToSpace(char *src, int slen, char *dst, int dstMaxSize) {
    int i=0, j=0;

    while (i<slen && j<(dstMaxSize-1)) {
        if ('\0' == src[i]) {
            i++;
        } else if (isprint(src[i]) && !isspace(src[i])) {
            dst[j++] = src[i++];
        } else if (isspace(src[i])) {
            dst[j++] = src[i++];
            while(i<slen && !(isprint(src[i])&& !isspace(src[i]))) i++;
        } else {
            dst[j++] = ' ';
            while(i<slen && !(isprint(src[i])&& !isspace(src[i]))) i++;
        }
    }
    dst[j] = '\0';

    return j;
}

/* ��ȡ'ip'�ĵ����ʽ, 'ipstr'������, 'len'��'ipstr'�Ĵ�С
   'ip'�������� */
static int getStrIp(u_int32_t ip, char *ipstr, size_t len) {
    struct in_addr addr;
    addr.s_addr = ip;
    inet_ntop(AF_INET, &addr, ipstr, len);
    return 0;
}

/* �ѻ����request/response���ݣ��͸�������Ϣ�����浽�����ݿ���ļ��С�
   �ɹ�����0, ʧ�ܷ���-1��
   ע�� -
   <1>response��request��2���ļ�����1���ַ�������ʶ������͵ģ�����д��
      ���ݿ�������
   <2>�ļ�����ʽ='SQLSERVER_PREFIX'����audit_time�����̵߳İ�������thread_id��
   <3>�����ݿ�Ļ����ļ��ĸ�ʽ -
      rowkey=[time1]|colfam1:table=[time2]|colfam1:app_id=[app_id]|colfam1:saveflag=[0/1]|colfam1:src_ip=x.x.x.x|colfam1:src_mac=[x:x:x:x:x:x]|colfam1:src_port=[src_port]|colfam1:dst_ip=x.x.x.x|colfam1:dst_mac=x:x:x:x:x:x|colfam1:dst_port=[dst_port]|colfam1:user_id=[user_id]|colfam1:operation_command=[request]|colfam\n
      \nrowkey=[time1]|colfam1:table=[time2]|colfam1:app_id=[app_id]|colfam1:saveflag=[0/1]|colfam1:src_ip=x.x.x.x|colfam1:src_mac=[x:x:x:x:x:x]|colfam1:src_port=[src_port]|colfam1:dst_ip=x.x.x.x|colfam1:dst_mac=x:x:x:x:x:x|colfam1:dst_port=[dst_port]|colfam1:user_id=[user_id]|colfam1:response_content=[response]|colfam\n
      ÿ2��rowkey֮�䣬��1�����С�
*/
#define SQLSERVER_SQL_TMP_DIR "/data/audit/sql_tmp/"
#define SQLSERVER_SQL_DIR "/data/audit/sql/"
#define SQLSERVER_PREFIX "Sql_sqlserver_"

/* �ļ����ϴ��޸�ʱ�䵽��ǰʱ�䣬����'secs'�룬�򷵻�1�����򷵻�0 */
static int isOld(char *filePath, int secs) {
    time_t t;
    int retval;
    struct stat ft;

    retval = time(&t);
    if (retval < 0) return 1;

    retval = stat(filePath, &ft);
    if (retval < 0) return 1;

    if ((long)t - (long)(ft.st_mtime) > secs) return 1;
    return 0;
}

static CSP_FILE_INFO zinfo;
int sql_server(AUDIT_ENSEMBLE_REL *data, CACHE_POLICY_CONF *policy) {
    char buffer[1024]={0},
         srcipstr[32]={0}, dstipstr[32]={0}, srcmac[64]={0}, dstmac[64]={0};
    int pkt_type;
    int line_num;  /* 2015-11-17: select��䷵�ص����� */

    if (data->request_size <= 0) { fprintf(stderr, "%s:%d:data->reqsize <= 0\n", __FILE__, __LINE__); return 0; }

    /* ���������ݿ�Ļ����ļ�����ȫ·�� */
    if (0==itemCount[data->thid] || '\0'==curFileName[data->thid][0] || '\0'==curDstFileName[data->thid][0]) {
        itemCount[data->thid] = 0;
        sprintf(curFileName[data->thid], "%s%s%s_%d_%d", SQLSERVER_SQL_TMP_DIR, SQLSERVER_PREFIX, data->table, fileNameSuffixN[data->thid], data->thid);
        sprintf(curDstFileName[data->thid], "%s%s%s_%d_%d", SQLSERVER_SQL_DIR, SQLSERVER_PREFIX, data->table, fileNameSuffixN[data->thid], data->thid);
        fileNameSuffixN[data->thid]++;
    }

    pkt_type = TDS_parser(data->request_data, data->request_size, data->operation, 1024*1024*5-10, &line_num);
    if ((pkt_type>=12 && pkt_type<=20) || (pkt_type>=29)) return 0;
    //if ((strlen(data->operation)) < 5) {
    //    unprintToSpace(data->request_data+8, data->request_size-8, data->operation, 1024*1024*5-10);
    //    trim(data->operation);
    //    if (strlen(data->operation) < 5) return 0;
    //}

    /* ���ʽip (cli��Ӧ �������src    ser ��Ӧdes)*/
    getStrIp(ntohl(data->userip), srcipstr, 32);
    getStrIp(ntohl(data->desip), dstipstr, 32);

    /* mac
    sprintf(srcmac, "%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",
            data->srcmac[0], data->srcmac[1], data->srcmac[2], data->srcmac[3], data->srcmac[4], data->srcmac[5],
            data->srcmac[6], data->srcmac[7], data->srcmac[8], data->srcmac[9], data->srcmac[10], data->srcmac[11]);
    sprintf(dstmac, "%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",
            data->desmac[0], data->desmac[1], data->desmac[2], data->desmac[3], data->desmac[4], data->desmac[5],
            data->desmac[6], data->desmac[7], data->desmac[8], data->desmac[9], data->desmac[10], data->desmac[11]);
    */
    strcpy(srcmac, data->srcmac);
    strcpy(dstmac, data->desmac);

    /* 2015-11-16, ����snmp
    char tmpSrcmac[64]={0}, tmpDstmac[64]={0};
    get_mac_str(srcipstr, tmpSrcmac);
    get_mac_str(dstipstr, tmpDstmac);
    if (tmpSrcmac[0] != '\0' && tmpDstmac[0] != '\0') {
        strcpy(srcmac, tmpSrcmac);
        strcpy(dstmac, tmpDstmac);
    }*/

    /* ����ƥ�� */
	//CACHE_POLICY_CONF *policy = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm();
	if(NULL == policy){
        printf("Fails to get CACHE_POLICY_CONF");
		return 0;
	}
    sprintf(zinfo.cspHead.policytime, "%d", data->policytime);
    strcpy(zinfo.cspHead.userip, srcipstr);
    zinfo.type = pkt_type;
    zinfo.security_level = 0;
    if (0 == policy_match(&zinfo, policy)) {
        return 0;
    }

    /* request����
       operation_command��ƫ����������ƫ�����ݺ��\n */
    sprintf(buffer,
        "rowkey=%lu"
        "|colfam1:table=%s"
        "|colfam1:app_id=%d"
        "|colfam1:src_ip=%s"
        "|colfam1:src_mac=%s"
        "|colfam1:src_port=%hu"
        "|colfam1:dst_ip=%s"
        "|colfam1:dst_mac=%s"
        "|colfam1:dst_port=%hu"
        "|colfam1:user_id=%hu"
        #if GA_TEST
        "|colfam1:security_level=%hu"
        #endif
        "|colfam1:charset=%s"
        "|colfam1:interval_time=%lu" /* 2015-11-09�����ֶ� */
        "|colfam1:operation_command=%d"
        "|colfam\n",
        data->times, data->table, pkt_type, srcipstr, srcmac, data->cliport,
        dstipstr, dstmac, data->serport, data->userid,
        #if GA_TEST
        zinfo.security_level,
        #endif
        "UTF-8", data->interval_time, strlen(data->operation));

    int fd = open(curFileName[data->thid], O_CREAT|O_RDWR|O_APPEND, 0666);
    if (fd < 0) { printf("%d:open file failed", __LINE__); return -1; }

    write(fd, buffer, strlen(buffer));
    write(fd, data->operation, strlen(data->operation));
    write(fd, "\n", 1);

    /* response���� */
    data->response[0] = '\0';
    if (data->request_size > 0) {
        TDS_parser(data->response_data, data->response_size, data->response, 1024*1024*5-10, &line_num);
    }

    /* response_content��ƫ����������ƫ�����ݺ��\n */
    sprintf(buffer,
        "rowkey=%lu"
        "|colfam1:line_num=%d"
        "|colfam1:response_content=%d"
        "|colfam\n",
        data->times, line_num, strlen(data->response));

    write(fd, buffer, strlen(buffer));
    write(fd, data->response, strlen(data->response));
    write(fd, "\n", 1);
    close(fd);

    /* �ƶ������ݿ���ļ� */
    itemCount[data->thid]++;
    if (itemCount[data->thid] >= ITEM_MAX || isOld(curFileName[data->thid], 60)) {
        rename(curFileName[data->thid], curDstFileName[data->thid]);
        unlink(curFileName[data->thid]);
        itemCount[data->thid] = 0;
        curFileName[data->thid][0] = '\0';
        curDstFileName[data->thid][0] = '\0';
    }
    return 0;
}
