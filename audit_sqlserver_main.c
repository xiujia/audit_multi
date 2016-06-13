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
/*#include "./redis_new_api.h"  2015-11-16,新增snmp */

#define ITEM_MAX 1 /* 填数据库的文件的内容条数最大值 */
#define THREAD_MAX 20

static char curFileName[THREAD_MAX][256] = {0};/* 当前填数据库的文件的完全路径名 */
static char curDstFileName[THREAD_MAX][256] = {0};
static int itemCount[THREAD_MAX] = {0};/* 每个填数据库的文件的条数的最大值 */
static int fileNameSuffixN[THREAD_MAX] = {0};

/* 删除'str'开头和末尾连续的空白字符. 原址进行.返回结果串的长度 */
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

/* 将长'slen'的串's'中, 1个或连续的多个不可打印字符变成一个空格, 结果串保存到'dst'中
 * 返回'dst'字符串的长度 */
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

/* 获取'ip'的点分形式, 'ipstr'保存结果, 'len'是'ipstr'的大小
   'ip'是网络序 */
static int getStrIp(u_int32_t ip, char *ipstr, size_t len) {
    struct in_addr addr;
    addr.s_addr = ip;
    inet_ntop(AF_INET, &addr, ipstr, len);
    return 0;
}

/* 把缓存的request/response数据，和该流的信息，缓存到填数据库的文件中。
   成功返回0, 失败返回-1。
   注意 -
   <1>response和request这2个文件，第1个字符是用于识别包类型的，不能写到
      数据库的内容里。
   <2>文件名格式='SQLSERVER_PREFIX'加上audit_time，该线程的包计数，thread_id。
   <3>填数据库的缓存文件的格式 -
      rowkey=[time1]|colfam1:table=[time2]|colfam1:app_id=[app_id]|colfam1:saveflag=[0/1]|colfam1:src_ip=x.x.x.x|colfam1:src_mac=[x:x:x:x:x:x]|colfam1:src_port=[src_port]|colfam1:dst_ip=x.x.x.x|colfam1:dst_mac=x:x:x:x:x:x|colfam1:dst_port=[dst_port]|colfam1:user_id=[user_id]|colfam1:operation_command=[request]|colfam\n
      \nrowkey=[time1]|colfam1:table=[time2]|colfam1:app_id=[app_id]|colfam1:saveflag=[0/1]|colfam1:src_ip=x.x.x.x|colfam1:src_mac=[x:x:x:x:x:x]|colfam1:src_port=[src_port]|colfam1:dst_ip=x.x.x.x|colfam1:dst_mac=x:x:x:x:x:x|colfam1:dst_port=[dst_port]|colfam1:user_id=[user_id]|colfam1:response_content=[response]|colfam\n
      每2条rowkey之间，有1个空行。
*/
#define SQLSERVER_SQL_TMP_DIR "/data/audit/sql_tmp/"
#define SQLSERVER_SQL_DIR "/data/audit/sql/"
#define SQLSERVER_PREFIX "Sql_sqlserver_"

/* 文件的上次修改时间到当前时间，过了'secs'秒，则返回1，否则返回0 */
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
    int line_num;  /* 2015-11-17: select语句返回的行数 */

    if (data->request_size <= 0) { fprintf(stderr, "%s:%d:data->reqsize <= 0\n", __FILE__, __LINE__); return 0; }

    /* 生成填数据库的缓存文件的完全路径 */
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

    /* 点分式ip (cli对应 表里面的src    ser 对应des)*/
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

    /* 2015-11-16, 新增snmp
    char tmpSrcmac[64]={0}, tmpDstmac[64]={0};
    get_mac_str(srcipstr, tmpSrcmac);
    get_mac_str(dstipstr, tmpDstmac);
    if (tmpSrcmac[0] != '\0' && tmpDstmac[0] != '\0') {
        strcpy(srcmac, tmpSrcmac);
        strcpy(dstmac, tmpDstmac);
    }*/

    /* 策略匹配 */
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

    /* request缓存
       operation_command的偏移量不计算偏移内容后的\n */
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
        "|colfam1:interval_time=%lu" /* 2015-11-09新增字段 */
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

    /* response缓存 */
    data->response[0] = '\0';
    if (data->request_size > 0) {
        TDS_parser(data->response_data, data->response_size, data->response, 1024*1024*5-10, &line_num);
    }

    /* response_content的偏移量不计算偏移内容后的\n */
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

    /* 移动填数据库的文件 */
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
