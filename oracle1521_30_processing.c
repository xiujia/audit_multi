/*
Ŀǰ�����:
ODBC:
032b_08xx
0347_0602

qingyang:
11xx_1017
035e_0602

home:
11xx_1017
0305_0602
������Ҫ��2�������İ汾���ұ������еĻ�ϰ汾
�Ժ�����õ����İ汾, ���������������������û�ϰ汾

1. request��response����Ҫ�ֿ�����, ��Ϊrequest�����꣬�ж�app_id������ͨ��, ��ʡȥresponse����.
������һ��, ������1�κ�������, �ò���ʧ��
2. ��ϰ汾Ӧ�û����ò���, �����Ա������ο�
3. ���ð汾�ź����ú�, �Ա���ò�ͬ�Ľ�������

�޸�:
================================================================================
2015-12-11
mainoracle()��
1. requet�ļ���, ����response�ļ���, �򻯹���
2. ��request,response�ļ�����������ȫ�ָ�Ϊ�ֲ�����, ����������,requestFile, responseFile
3. ��ɾ����������cmdRmRequest, cmdRmResponse��Ϊ�ֲ�����, ��Ϊ��ʱ�ò���
4. sscanf���checkOldListǰ�Ӹ������ж�, ���ٽ��뺯���Ĵ���
5. sql_str�����㶯��ȡ��, ����������Ҫ��֤�ַ���ĩβ��\0
6. pktType��ȫ�ֱ���, ��Ϊ�ֲ�����, response��������responseParser(), responseParserHelper()�����б�ĩβ�Ӳ���int type.
7. mainoracle()����������ж�һ����pktType
8. �û�����, ��������Ϊ�յİ�, ������sql�����ݰ�, ��һ��switch�д���, ȡ��if-else�ṹ
9. ��request��������ҽԺ�ĸ�ʽ�������ͼ����64�����ֳ�2����������:request_qingyang64bit10g_plsql(),request_home64bit10g_plsql
10. ֻ��11xx��035e��ͷ��request���ý�������, �������ò������ַ�������
11. ȥ��response_str��memset�������, response���������б�֤�ַ���ĩβ��\0
12. ȥ��bufForColValuestmp��memset����
13. ȥ��select�ķ��������ɲ��֣�bufForColValuesmain��memset����
14. ��select�ķ��������ɲ��ֵ�resPos += strlen(resPos);
    ��ΪresPos += (strlen(colInfos[i].colName) + 3);
    ��ΪbufForColValuesmain����֤ȫ��\0��
15. ��select�ķ��������ɲ���, ��Ϊֻ��1017��0602���ذ�ʱ������

1017��0602�Ľ�������:
1. ��1017��0602�Ľ��������������2�������ĺ������ֱ��Ӧ����ҽԺ��64λ10g�ͼ����64λ10g�汾����ԭ���Ļ�ϰ汾������
2. ��1017��������(2���汾), ȥ���޹ؽ�Ҫ��ֵ�Ķ�ȡ��ֻȡ����,�ַ���,����. ������Ϣ��������
3. ɾ��1017��0602����������ͷ���Գ��ȵ���𣬱�֤�ڵ���ǰ���˵����̵�����
4. ����2��ȫ�ֵĺ���ָ��response_035e_0602, response_035e_1017�����ݲ�ͬ�汾�����ò�ͬ�Ľ�������
5. ��response_035e_1017_qingyang��response_035e_1017_home64bit10g����ĩβ��0602������������Ϊ��Ӧ�汾�ĺ���
6. ��response_035e_0602_qingyang�Ļ�ȡ����Ϣ�Ĳ��ִ�ѭ���е�seq==0���֣��ƶ��������ʼ��
7. 0602���������ĵ��ã�����2�������1�ǻ�ȡ�����ʱ���ص�0602��2����1017����׷����0602��(�����sqllus������)
   ��˶�redis��ȡ����ϢҪ�ж��£������2�����ʱ������Ϣ��ûдredis�ˣ��ͻ᷵�ؿյ�����Ϣ����������
8. ȥ��requestParser(),responseParser()���bufForTnsRebuid��memset���㣬��ΪtnsRebuild
   ��������ݲ����ַ�������һֱ�г��Ȳ�����û��Ҫ����Ϊ\0
9. ��request_home64bit10g_plsql(), request_qingyang64bit10g_plsql()��requestParserHelper_64bit10g_plsql_homeAndqingyang()�ʼ���Ȱ�res��ʼ��Ϊ�մ�

segmentsReform()
1. ��������ڽ����ĩβ���\0

parseColValue()
1. ȥ������parseCHAR()֮ǰ��bufForBigCharRebuid��memset����
2. �ѵ�1���ֵ�strlen(res), ��ΪresPos-res, ��Ϊres����֤����

parseCHAR()
1. �ڵ�3���������������ĩβ��\0����ֹû��������

saveColInfos
1. ȥ��tmpColInfos��memset���㣬ֻ����1���ֽ���Ϊ\0
2. ȥ��redisColInfos��memset����
3. �ڶ�redis�������Ϣʱ��posû��ʼ��, �޸��˸�bug

saveSelectColInfosColValues
1. checkold��free���ǿյģ���ô�����⣬ֱ��exit��ȡ��return -1
2. ���е�дredis��EX��ʱ��ֵ������Ϊ120s��ȡ��1200s
3. ����rows����, bufForColValuestmp��memset���㣬��ȫû��Ҫ
4. ȡ���ú�����colName����, ���ڻ���rows������ȡ��bufForColValuestmp��Ӧ�ã�ֱ����colValues
5. ����colInfos����, redisColInfos�ں����Ѿ���֤����\0�ˣ�����ȥ��memset����

saveColValues
1. ȥ��colNames����
2. �ڶ�redisǰ��ȥ��bufForColValuestmp��memset����

shmDelOneRecodWithLastColValues
1. ȥ������getSelectAndResponse()����ǰ, bufForColValuestmp��memset����
2. while�в���return����Ϊbreak
3. �ں������Ҫ��dummy.next���¹һص�hash��

shmDelOneRecod
1. ȥ������getSelectAndResponse()����ǰ, bufForColValuestmp��memset����(��shmDelOneRecodWithLastColValues()������һ��)
2. while�в���return����Ϊbreak(��shmDelOneRecodWithLastColValues()������һ��)
3. �ں������Ҫ��dummy.next���¹һص�hash��(��shmDelOneRecodWithLastColValues()������һ��)

getSelectAndResponse
1. �ڿ�ͷ��sqlbuf��resultbuf�ĵ�1���ֽ���Ϊ\0����ֹ����return

checkOldList
1. ȥ������getSelectAndResponse()����ǰ, bufForColValuestmp��memset����

getHighestLevelSqlAction
1. ��delete�ȹؼ��ֵ�strlen, ��Ϊ����, ʡȥ��κ�������

================================================================================
2015-12-14
mainoracle():
1, �޸���requset�����������õ�����, ԭ����pktType==PKT_035E__XXXX�Ƚϣ������select�Ķ�©����.
   ��Ϊ(0x11 == requestBuf[10])
2. �޸���response�����������ã�ԭ���ǱȽ�responseBuf[10]/[11]����Ϊ(PKT_035E__1017==pktType || PKT_035E__0602==pktType)
   �����Щ
3. ��colInfos��totalColInfoNum�����㣬��mainoracle()�Ŀ�ͷδ֪���ƶ���responseParser()�У�ʡȥ��select���ذ�ʱ�Ķ���
4. ��û�д�ӡ��returnǰ�����ϴ�ӡ
5. ��response_str��sql_str�ڽ���֮ǰ��Ϊ�մ�����Ϊ1017���ذ�����ʱ����������0602�ְ�����ô����
   response_str���κβ���������Ҫ���ÿմ�����ô��������δ֪��������select�����ʱ�������


GET_TYPE():
1. ����request��response����û��ƫ��10�ֽڵģ��Ƚϵ��ֽڸ�Ϊ10��11

checkOldList():
1.�ڼ��isoldǰ�����������ӡ, �鿴isold��ʶ�����
2.�޸�isold����Ĳ���, ԭ����capture_time/1000000, ����������ͣ��һ��ʱ���, �ٿ���, ��ô���еİ�������isold��
  ����Ҫ��ʶ���ʱ���Ϊ������ʱ��.

setUsername_0376()
1.��setUsername_0376()�����ֳ������汾, ��������ҽԺ�İ汾���ͼ����64λ10g�İ汾

responseParser()
1. �ڿ�ͷ������res[0]='\0';
2. ����resΪ�մ��󣬼��볤���жϣ��������ȹ�С�İ�

================================================================================
2015-12-15
1.������LONG, LONG RAW, RAW���͵�ʶ����������parseColValue()�����Ӧ����
2. parseColValue()���if-else�ṹ����Ϊswitch-case�ṹ
3. writeDBfile()�����е���system()���ƶ���������ʱ1500΢�����ϣ��ĳ���rename()����, ��ʱ130΢������

2015-12-23:
1.��writeFile()�������˲���ƥ�����
2. �޸��ַ���ת�����ִ��룬NCHAR,NVARCHAR��UCS-2���룬��תΪΪ��CHAR,VARCHAR����ͬ�ı��룬
   ����һ����TNS_CHARSET������ָ��CHAR,VARCHAR�ı��롣
   ɾ��CHAR,VARCHAR�Ȳ��ֵ�codeConv()���ã���Ϊֱ��memcpy()

================================================================================
2016-01-28
1, ��0376������������ӿͻ������͵��ж���
   ����ȫ�ֱ�Ǳ���clientType,
   ���ӱ�ʾ�ͻ������͵ĺ�:
#define CLI_MINI 11
#define CLI_PL_SQL_DEVELOPER 11
#define CLI_SQLPLUS 12
#define CLI_ORACLE_SQL_DEVELOPER 13
#define CLI_OTHER 14
#define CLI_MAX 14
#define IS_VALID_CLIENT(n) ((n)>=CLI_MINI && (n)<=CLI_MAX)
2, setUsername_0376_home64bit10g��������Ϊrequest_0376_home64bit10g
   setUsername_0376_qingyang64bit10g_plsql��������Ϊrequest_0376_qingyang64bit10g_plsql
3, request_0376_home64bit10g��ļ���if-else��Ϊ?:���ʽʵ�֡�
4, response_035e_0602ȫ�ֱ���������REQUEST_PARSER��ΪRESPONSE_PARSER.
   ����REQUEST_PARSER���͵�ȫ�ֱ���request_035e
5, �ں���getUsername()�У����Ӷ�ȡclientTypeֵ�Ķ��������Ѻ�������ΪgetUsernameAndClientType()

2016-02-02:
1,���Ӻ�
#define ORACLE_10G 10
#define ORACLE_11G 11
2����ȫ�ֱ���oracleVersion��ΪserverVersion,�������е�ֵ��Ϊʹ�����涨��ĺ꣬ȡ��Ӳ����
3, ���logMsg()��־���������ڵ��ԣ����
#if TNS_DEBUG
fprintf(...)
#endif
���ֵ�����䣬�Դ�����ӽ��ա�(ȱ���ǵ�TNS_DEBUGΪ0ʱ, ����ǰ���������պ�������)
4, requestParser()������������������Ϊ����ȫ�ֱ���request_035e()��
5��ȥ��mainoracle()�����request���ֵ�unprintToSpace()��ʹֻ֮����11, 035e���ֻظ���
   �������ͻظ��Ľ����������Ϊ�մ���

2016-02-03:
1, ��response�������֣���1017��0602�İ�������unprintToSpace()��ֱ�Ӱѽ����Ϊ�մ�

2016-02-23:
1, ���쿪ʼ����oracle 11g r2 32λ�汾��
2���ѱ��ļ��ṹ��Ϊÿ��oracle�汾һ�׺�������Ҫ��ͬһ�������Ｏ�϶���汾�����׻��ң��Ҳ����޸�
3�������������������ƣ�Ҳ�Ѱ汾�ֶθĵ���ͷ, ����ÿ���汾�Ľ��������ŵ�һ��
   home64bit10g_request_0376()
   qingyang64bit10g_plsql_request_0376()
4����������:
   home32bit11gr2_request(),
   home32bit11gr2_response_035e_1017(),
   home32bit11gr2_response_035e_0602().
5��GET_TYPE()��mainoracle()��0602��������0601��ʶ��
6��getUsernameAndClientType()����һ������ct�����ڷ���clientType��ȡ����ǰֱ�Ӱ�
   clientTypeֱ�ӷŵ�ȫ�ֱ���������

2016-02-25:
�����е����Ͱ����ֳɲ�ͬ��client�ֱ���

2016-20-26
����һ��bug��mainoracle()���������sql��䱣����ȫ�ֱ���sql_str, ���ڽ������֮ǰ����Ҫ������һ��sql
����shmDelOneRecod()�л���sql_str������һ��sql���ͻ�ѵ�ǰ��sql���ǵ��ˡ�
*/

#include <time.h>
#include <string.h> /* strlen()... */
#include <sys/mman.h> /* mmap()... */
#include <sys/stat.h> /* stat() */
#include <fcntl.h> /* open()... */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h> /* ntohs() */
#include <iconv.h> /* iconv() */
#include <signal.h> /* signal() */
#include <ctype.h> /* isalpha() */
#include <hiredis/hiredis.h>
#include <errno.h>
#include "/usr/src/inp/audit_new/oracleShm.c"

#include "csp_deal.h"
#include "csp_policy.h"
#include "/usr/src/inp/audit_new/redis_new_api.h" /* 2015-11-16,����snmp */
#include "audit_release.h"

#define TNS_DEBUG 0
#define TNS_BINARY_DEBUG 0

#define TNS_DEBUG_FUN_TIME 0
#if TNS_DEBUG_FUN_TIME
#define DEBUG_TIME_INIT() struct timeval timeval1, timeval2
#define DEBUG_TIME_START() gettimeofday(&timeval1, NULL)
#define DEBUG_TIME_STOP(msg) \
do {\
    gettimeofday(&timeval2, NULL);\
    fprintf(stderr, "%s : sec=%lu, usec=%lu\n", (msg), timeval2.tv_sec-timeval1.tv_sec, timeval2.tv_usec-timeval1.tv_usec);\
}while(0)
#else
#define DEBUG_TIME_INIT()
#define DEBUG_TIME_START()
#define DEBUG_TIME_STOP(msg)
#endif

/* ����ʵ�ʵ�oracle�ַ������� */
#define TNS_CHARSET "GBK"

/* client���� */
#define CLI_MINI 11
#define CLI_PL_SQL_DEVELOPER 11
#define CLI_SQLPLUS 12
#define CLI_ORACLE_SQL_DEVELOPER 13
#define CLI_OTHER 14
#define CLI_MAX 14
#define IS_VALID_CLIENT(n) ((n)>=CLI_MINI && (n)<=CLI_MAX)
int clientType = -1;

#define ORACLE_10G 10
#define ORACLE_11G 11
int serverVersion = ORACLE_10G;

/* redis��� */
#define REDISSERVERHOST      "127.0.0.1"
#define REDISSERVERPORT      6379
#define REDIS_ORACLE_TABLE  14

#define TEMP_SQL_DIR "/data/audit/sql_tmp"
#define SQL_DIR "/data/audit/sql"

#define MAX_COLUMN 1024

/* TNSͷtype�ֶγ���ȡֵ, ֻ��0x06(DATA����)���õ������г����Ƿ�ֹ�Ժ��õ� */
#define PKT_TYPE_CONNECT 0x01
#define PKT_TYPE_ACCEPT 0x02
#define PKT_TYPE_ACK 0x03
#define PKT_TYPE_REFUSE 0x04
#define PKT_TYPE_REDIRECT 0x05
#define PKT_TYPE_DATA 0x06 /* ����(����Ҫ������,sql����ͷ��ض�������) */
#define PKT_TYPE_NULL 0x07
#define PKT_TYPE_ABORT 0x09
#define PKT_TYPE_RESEND 0x0b
#define PKT_TYPE_MARKER 0x0c
#define PKT_TYPE_ATTENTION 0x0d
#define PKT_TYPE_CONTROL 0x0e

#define TNS_NORMAL 0 /* ��0x06�����ҽ������� */
#define TNS_DEL -1   /* ��0x06�� */
#define TNS_SAVE -2  /* ��0x06���������쳣 */

#define DATA_TYPE_VARCHAR2 0x01
#define DATA_TYPE_NUMBER 0x02
#define DATA_TYPE_LONG 0x08
#define DATA_TYPE_RAW 0x17
#define DATA_TYPE_LONG_RAW 0x18
#define DATA_TYPE_ROWID 0x0b
#define DATA_TYPE_DATE 0x0c
#define DATA_TYPE_CHAR 0x60
#define DATA_TYPE_BINARY_FLOAT 0x64
#define DATA_TYPE_BINARY_DOUBLE 0x65
#define DATA_TYPE_CLOB 0x70
#define DATA_TYPE_BLOB 0x71
#define DATA_TYPE_BFILE 0x72
#define DATA_TYPE_TIMESTAMP 0xb4
#define DATA_TYPE_TIMESTAMP_TZ 0xb5
#define DATA_TYPE_INTERVAL_YEAR 0xb6
#define DATA_TYPE_INTERVAL_DAY 0xb7
#define DATA_TYPE_UROWID 0xd0
#define DATA_TYPE_TIMESTAMP_LTZ 0xe7

typedef struct {
    unsigned short length; /* TNS�����ȣ�����TNSͷ */
    unsigned short packetCheckSum; /* ����У��� */
    unsigned char type; /* TNS������ */
    unsigned char flag; /* ״̬ */
    unsigned short headerCheckSum; /* TNSͷ��У��� */
} TNS_HEADER;

typedef struct {
    char colName[1024];
    unsigned char type;
    unsigned char charWidth; /* ��CHAR,VARCHARʱ��1��NCHAR,NVARCHARʱ��2 */

    /* ����NUMBER,FLOAT. ��NUMBER(p,s)�У�
    p�Ǿ��ȣ���ʾ��Ч���ֵ�λ��,
    sΪ����ʱ����ʾ��С���㵽�����Ч���ֵ�λ������Ϊ����ʱ����ʾ�������Ч���ֵ�С�����λ�� */
    unsigned char precision;
    unsigned char scale;
    unsigned long maxByteSize; /* ��ռ���ռ� */
    unsigned char nullbit;
    unsigned int seq;
}COLUMN_INFO;

typedef struct {
    char *colValues[MAX_COLUMN+1];
    int colSizes[MAX_COLUMN+1];
}PREV_ROW_INFO;

/* �ͻ�PL/sql developer 7����oracle 10g 64bits */
#define PKT_11XX_035E__1017 0 /* 03 5E�������sql���, 10 17��������Ϣ */
#define PKT_035E__1017 1
#define PKT_035E__0602 2 /* 03 5E��������, 06 02������ֵ */
#define PKT_035E__04XX 3 /* 03 5E��������, 04 01��ʾ������Ϊ�� */
#define PKT_035E__XXXX 4 /* 03 5E����, �ظ�������1017,0602,04XX������� */

#define PKT_0305__0602 5 /* 03 05��������, 06 02�ظ���ֵ */
#define PKT_0305__04XX 6 /* 03 05��������, 04 01��ʾ������Ϊ�� */

/* �ͻ�odbc����oracle 10g 64bits */
#define PKT_034A__XXXX 7 /* 03 4A����sql���, 04XX����ͳ����Ϣ */
#define PKT_032B__08XX 8 /* 06 2B��������Ϣ, 08XX��������Ϣ */
#define PKT_0347__0602 9 /* 06 02������ֵ, 06 02�ظ���ֵ */
#define PKT_0347__04XX 10 /* 03 47���������ֵ, 04xx����û������ */
#define PKT_0376 11

#define PKT_OTHER 100

char pre_sql_str[5*1024] = {0};       /* �洢request������� */
char sql_str_main[5*1024] = {0};           /* �洢request������� */
char response_str[1024*1024] = {0}; /* �洢response������� */
unsigned char bufForTnsRebuid[2*1024*1024]; /* ����TNS����������, ��Ҫunsigned */
unsigned char requestBuf[2*1024*1024], responseBuf[2*1024*1024]; /* ������ļ������� */

char bufForBigCharRebuid[5*1024];
char bufForColValuesmain[10*1024*1024];
char bufForColValuestmp[10*1024*1024];
COLUMN_INFO colInfos[MAX_COLUMN];
unsigned char redisColInfos[10240]; /* ��Ҫunsigned, ���ֽڱȽ� */
unsigned char tmpColInfos[10240];   /* ��Ҫunsigned, ���ֽڱȽ� */
PREV_ROW_INFO prevRow;
int isLast = 0; /* 1��ʾselect���һ���غ� */
int totalColInfoNum = 0;
int line_num = 0; /* 06 02������� */

CSP_FILE_INFO zinfo;

unsigned int src_ip, src_port, dst_ip, dst_port; /*255.255.255.255(0xffffffff)��ip���ֵ, ����unsigned int��ʾ*/
int flagLocation;

typedef struct {
    COLUMN_INFO colinfos[MAX_COLUMN];
    char colValues[1024*1024];
    int colNum;
    int rowNum;
    int islast;
} RESPONSE_RES;
RESPONSE_RES responseRes;
typedef int (*PARSER)(unsigned char *, long, char *, int);

PARSER request_035e;
PARSER response_035e_0602;
PARSER response_035e_1017;
PARSER request_0376;

#define getRowsKey(src_ip, src_port, dst_ip, dst_port, key) sprintf(key, "%u-%u-%u-%u-rows", src_ip, src_port, dst_ip, dst_port)
#define getColInfosKey(src_ip, src_port, dst_ip, dst_port, key) sprintf(key, "%u-%u-%u-%u-cloInfos", src_ip, src_port, dst_ip, dst_port)
#define getSqlKey(src_ip, src_port, dst_ip, dst_port, key) sprintf(key, "%u-%u-%u-%u-sql", src_ip, src_port, dst_ip, dst_port)
#define getUsernameKey(src_ip, src_port, dst_ip, dst_port, key) sprintf(key, "%u-%u-%u-%u-username", src_ip, src_port, dst_ip, dst_port)
#define getClientKey(src_ip, src_port, dst_ip, dst_port, key) sprintf(key, "%u-%u-%u-%u-client", src_ip, src_port, dst_ip, dst_port)

char username[256];

/* ��־�������ɹ�����0�������û��ʼ������-1��*/
char outputlogMsg[10240];
int logMsg(char *format, ...) {
    va_list ptr;
    char *lineend;

#if TNS_DEBUG
    va_start(ptr, format);

    /* ���������е�ϵͳ��֧��vsnprintf() */
#ifdef NO_SNPRINTF
    vsprintf(outputlogMsg, format, ptr);
#else
    vsnprintf(outputlogMsg, sizeof(outputlogMsg)-1, format, ptr);
#endif

    /* ��֤���һ���ַ��ǻ��� */
    lineend = ('\n' == outputlogMsg[strlen(outputlogMsg)-1]) ? "" : "\n";

    fprintf(stderr, "%s%s", outputlogMsg, lineend);
#endif
    return 0;
}

/* ����20s���ϻ� */
#define isold(n, now) ((n)-(now)>=200 || (now)-(n)>=200)
//#define isold(n, now) 0

int getPolicyTime(unsigned long time){
    int  time_sec=0;
    int  time_min=0;
    const int time_zone = 8*60;
    const int week_min = 7*24*60;
    //const int day_min = 24*60;
    const int monday_start = 4*24*60;
    int res;
    time_sec = time/1000000;
    time_min = time_sec/60;

    res = (time_min + time_zone - monday_start)%week_min;

    return res;
}

/* ��redis���ȡusername��clientTypeֵ������һ���ٵ�һ�����ӹ���
   "username"�ǳ�"len"�Ļ��棬clientType������"ct"��*/
int getUsernameAndClientType(char *username, int len, int *ctype) {
    char key[128] = {0};
    redisContext *conn = NULL;
    redisReply *reply =NULL;
    char tmp[16];

    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    getUsernameKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "get %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    if (REDIS_REPLY_STRING == reply->type && reply->len>0) {
        int tocopy = ((len-1) > (reply->len)) ? (reply->len) : (len-1);
        strncpy(username, reply->str, reply->len);
        username[tocopy] = '\0';
    } else {
        username[0] = '\0';
    }
    freeReplyObject(reply);

    getClientKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "get %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    if (REDIS_REPLY_STRING == reply->type && reply->len>0) {
        int tocopy = ((len-1) > (reply->len)) ? (reply->len) : (len-1);
        strncpy(tmp, reply->str, reply->len);
        tmp[tocopy] = '\0';
        *ctype = atoi(tmp);
    } else {
        username[0] = '\0';
        *ctype = CLI_PL_SQL_DEVELOPER;
    }
    if (!IS_VALID_CLIENT(*ctype)) *ctype = CLI_PL_SQL_DEVELOPER;
    freeReplyObject(reply);
    redisFree(conn);
    reply = NULL;

    logMsg("getUsernameAndClientType: username=%s, key=%s, clientType=%d\n", username, key, clientType);
    return 0;
}

CSP_FILE_INFO zinfo;
int writeDBfile(int dir_id, unsigned long capture_time, int app_id,
                unsigned int src_ip, unsigned int src_port, char *src_mac,
                unsigned int dst_ip, unsigned int dst_port, char *dst_mac,
                unsigned long interval_time, int line_num, char *sql_str, char *response_str){
    char filename_sql[1024], temp_filename_sql[1024], command[2048],
         src_ip_str[100], dst_ip_str[200], src_mac2[200], dst_mac2[200];
    FILE *fd_sql;
    struct in_addr in;
    char c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12;
    time_t t1;
    struct tm *tm1;

    logMsg("writeDBfile: sql = %s\n, response = %s, app_id=%d, line_num=%d\n", sql_str, response_str, app_id, line_num);

    if (6==app_id && line_num<=0) {
        logMsg("writeDBfile: not write(app_id != 6 || line_num<=0)\n");
        return 0;
    }

    /* IP */
    in.s_addr=src_ip;
    strcpy(src_ip_str, inet_ntoa(in));

    in.s_addr=dst_ip;
    strcpy(dst_ip_str, inet_ntoa(in));

    /* MAC */
    sscanf(src_mac,"%c%c%c%c%c%c%c%c%c%c%c%c",&c1,&c2,&c3,&c4,&c5,&c6,&c7,&c8,&c9,&c10,&c11,&c12);
    sprintf(src_mac2,"%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12);
    sscanf(dst_mac,"%c%c%c%c%c%c%c%c%c%c%c%c",&c1,&c2,&c3,&c4,&c5,&c6,&c7,&c8,&c9,&c10,&c11,&c12);
    sprintf(dst_mac2,"%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12);

    /* 2015-11-16, ����snmp
    char tmpSrcmac[64]={0}, tmpDstmac[64]={0};
    get_mac_str(src_ip_str, tmpSrcmac);
    get_mac_str(dst_ip_str, tmpDstmac);
    if (tmpSrcmac[0] != '\0' && tmpDstmac[0] != '\0') {
        strcpy(src_mac, tmpSrcmac);
        strcpy(dst_mac, tmpDstmac);
    }*/


    /* ����ƥ�� */
    CACHE_POLICY_CONF *policy = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm();
    if(NULL == policy){
        printf("Fails to get CACHE_POLICY_CONF");
        return 0;
    }
    sprintf(zinfo.cspHead.policytime, "%d", getPolicyTime(capture_time));
    strcpy(zinfo.cspHead.userip, src_ip_str);
    zinfo.type = app_id;
    zinfo.security_level = 0;
    if (0 == policy_match(&zinfo, policy)) {
        return 0;
    }

    sprintf(temp_filename_sql, "%s/Sql_oracle_%lu_%d", TEMP_SQL_DIR, capture_time, dir_id);
    sprintf(filename_sql, "%s/Sql_oracle_%lu_%d", SQL_DIR, capture_time, dir_id);
    fd_sql = fopen(temp_filename_sql, "w+");

    time(&t1);
    tm1 = localtime(&t1);
#if 0
    sprintf(bufForTnsRebuid,
        "rowkey=%lu|"
        "colfam1:table=%04d_%02d|"
        "colfam1:user_name=%s|"
        "colfam1:app_id=%d|"
        "colfam1:src_ip=%s|colfam1:dst_ip=%s|"
        "colfam1:src_mac=%s|colfam1:dst_mac=%s|"
        "colfam1:src_port=%u|colfam1:dst_port=%u|"
        "colfam1:interval_time=%lu|"
        "colfam1:line_num=%d|"
        "colfam1:operation_command=%s|"
        "colfam1:response_content=%d|colfam\n%s\n",
        capture_time, tm1->tm_year+1900, tm1->tm_mon+1, username,
        app_id, src_ip_str, dst_ip_str, src_mac2, dst_mac2, src_port, dst_port,
        interval_time, line_num, sql_str, strlen(response_str), response_str);
        fprintf(fd_sql, "%s", bufForTnsRebuid);
#endif

    fprintf(fd_sql, "rowkey=%lu|", capture_time);
    fprintf(fd_sql, "colfam1:table=%04d_%02d|", tm1->tm_year+1900, tm1->tm_mon+1);
    fprintf(fd_sql, "colfam1:user_name=%s|", username);
    fprintf(fd_sql, "colfam1:app_id=%d|", app_id);
    fprintf(fd_sql, "colfam1:src_ip=%s|colfam1:dst_ip=%s|", src_ip_str, dst_ip_str);
    fprintf(fd_sql, "colfam1:src_mac=%s|colfam1:dst_mac=%s|", src_mac2, dst_mac2);
    fprintf(fd_sql, "colfam1:src_port=%u|colfam1:dst_port=%u|", src_port, dst_port);
    fprintf(fd_sql, "colfam1:interval_time=%lu|", interval_time);
    fprintf(fd_sql, "colfam1:line_num=%d|", line_num);
#if GA_TEST
    fprintf(fd_sql, "colfam1:security_level=%hu|", zinfo.security_level);
#endif
    fprintf(fd_sql, "colfam1:operation_command=%s|", sql_str);
    fprintf(fd_sql, "colfam1:response_content=%d|colfam\n%s\n", strlen(response_str), response_str);

    fclose(fd_sql);
    rename(temp_filename_sql, filename_sql);
    return 0;
}


/* 1,��redis���select���
   2,��redis���select���� */
int getSelectAndResponse(unsigned int src_ip, unsigned int src_port, unsigned int dst_ip, unsigned int dst_port, char *sqlbuf, char *reulstbuf) {
    char key[256];
    redisContext * conn;
    redisReply *reply =NULL;

    sqlbuf[0] = '\0';
    reulstbuf[0] = '\0';

    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    //��ȡ��ɾ��select
    getSqlKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "get %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    if (REDIS_REPLY_STRING == reply->type) {
        strncpy(sqlbuf, reply->str, reply->len);
        sqlbuf[reply->len] = '\0';
    } else {
        sqlbuf[0] = '\0';
    }
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(conn, "del %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);

    //��ȡ��ɾ����ֵ
    getRowsKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "get %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    if (REDIS_REPLY_STRING == reply->type) {
        strncpy(reulstbuf, reply->str, reply->len);
        reulstbuf[reply->len] = '\0';
    } else {
        reulstbuf[0] = '\0';
    }
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(conn, "del %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);

    //����Ϣ��ɾ��, ��redis�Լ���ʱɾ��, ����Ϣ��ÿ�α���select���ʱ����
    //select * from help֮��Ĳ�ѯ, PL/sql developer����һ��ȫ������, Ҫ��
    //��һҳ�Ŵ�, ���Ҫ������Ϣ��select���Ҫ���������ʱ��.��select����
    //���ϻ��������.
    /*getColInfosKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "del %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);*/

    redisFree(conn);
    reply = NULL;

    return 0;
}

/* �ӿ�ʼ����, �ѳ���10s�����
   ע��: �жϵ�ʱ�䣬Ӧ������������ļ��ĵ�ǰʱ�䣬ǧ�����ð��ļ�������ʱ��
   ��Ϊ��������ӳٹ���Ļ��������Ϲ��ܶ��� */
int checkOldList(ORACLE_SHM_MEM *shm) {
    struct SHM_NODE *cur, *node, *prev, dummy;
    time_t now;
    time(&now);

    logMsg("checkOldList\n");

    if (shm->count <= 0) return 0;

    cur = shm->old;
    while (cur) {
        logMsg( "cur->savetime = %lu, now=%lu\n", cur->savetime, now);
        if (isold(cur->savetime, now)) {
            logMsg( "isold\n");
            node = cur;
            cur = cur->oldnext; /* cur��ǰһ�� */

            /* �������, ֻ����node, ��Ҫ��cur�� */
            if (node->oldprev) node->oldprev->oldnext = node->oldnext;
            else shm->old = node->oldnext;
            if (node->oldnext) node->oldnext->oldprev = node->oldprev;

            dummy.next = shm->hashTable[node->hash % HASH_TABLE_SIZE];
            prev = &dummy;
            while (prev->next && (prev->next != node)) prev = prev->next;
            if (NULL == prev->next) {
                fprintf(stderr, "%d: cannot find node in hash table\n", __LINE__);
                return -1;
            }

            prev->next = node->next;
            shm->hashTable[node->hash % HASH_TABLE_SIZE] = dummy.next;

            getSelectAndResponse(node->src_ip, node->src_port, node->dst_ip, node->dst_port, pre_sql_str, bufForColValuestmp);

            writeDBfile(node->dir_id, node->capture_time, 6,
                        node->src_ip, node->src_port, node->src_mac,
                        node->dst_ip, node->dst_port, node->dst_mac,
                        node->lastResutlPktCaptureTime - node->selectPktCaptureTime, node->line_num, pre_sql_str, bufForColValuestmp);

        } else {
            logMsg( "isnew\n");
            cur = cur->oldnext;
        }
    }
    return 0;
}

/* ��hash��, ����, ��д����ļ�, ������������ û�������ֵ��redis���Ѿ���������ֵ��  */
int shmDelOneRecod(ORACLE_SHM_MEM *shm, unsigned int hash, unsigned int src_ip, unsigned int src_port, unsigned int dst_ip, unsigned int dst_port) {
    struct SHM_NODE *cur, *prev, dummy;
    dummy.next = shm->hashTable[hash % HASH_TABLE_SIZE];
    prev = &dummy;
    while (prev->next) {
        cur = prev->next;
        if (cur->src_ip == src_ip && cur->src_port==src_port && cur->dst_ip==dst_ip && cur->dst_port==dst_port) {
            prev->next = cur->next;
            shm->hashTable[hash % HASH_TABLE_SIZE] = dummy.next;

            if (cur->oldprev) cur->oldprev->oldnext = cur->oldnext;
            else shm->old = cur->oldnext;
            if (cur->oldnext) cur->oldnext->oldprev = cur->oldprev;

            getSelectAndResponse(cur->src_ip, cur->src_port, cur->dst_ip, cur->dst_port, pre_sql_str, bufForColValuestmp);

            writeDBfile(cur->dir_id, cur->capture_time, 6,
                        cur->src_ip, cur->src_port, cur->src_mac,
                        cur->dst_ip, cur->dst_port, cur->dst_mac,
                        cur->lastResutlPktCaptureTime - cur->selectPktCaptureTime, cur->line_num, pre_sql_str, bufForColValuestmp);

            memset(cur, 0, sizeof(struct SHM_NODE));
            cur->next = shm->free;
            shm->free = cur;
            shm->count--;
            break;
        } else {
            prev = prev->next;
        }
    }
    shm->hashTable[hash % HASH_TABLE_SIZE] = dummy.next;
    return 0;
}

/* ��hash��, ����, ��д����ļ�, ����������, ��Ҫ׷�����һ������ֵ*/
int shmDelOneRecodWithLastColValues(ORACLE_SHM_MEM *shm, unsigned int hash, unsigned int src_ip, unsigned int src_port, unsigned int dst_ip, unsigned int dst_port, char *lastColValues, int line_num, unsigned long capture_time, unsigned long responseCaptureTime) {
    struct SHM_NODE *cur, *prev, dummy;
    if (shm->count <= 0) {
         return 0;
    }

    dummy.next = shm->hashTable[hash % HASH_TABLE_SIZE];
    prev = &dummy;

    while (prev->next) {
        cur = prev->next;
        if (cur->src_ip == src_ip && cur->src_port==src_port && cur->dst_ip==dst_ip && cur->dst_port==dst_port) {
            prev->next = cur->next;
            shm->hashTable[hash % HASH_TABLE_SIZE] = dummy.next;

            if (cur->oldprev) cur->oldprev->oldnext = cur->oldnext;
            else shm->old = cur->oldnext;
            if (cur->oldnext) cur->oldnext->oldprev = cur->oldprev;

            getSelectAndResponse(cur->src_ip, cur->src_port, cur->dst_ip, cur->dst_port, pre_sql_str, bufForColValuestmp);

            strcat(bufForColValuestmp, lastColValues);
            writeDBfile(cur->dir_id, cur->capture_time, 6,
                        cur->src_ip, cur->src_port, cur->src_mac,
                        cur->dst_ip, cur->dst_port, cur->dst_mac,
                        responseCaptureTime - cur->selectPktCaptureTime, cur->line_num+line_num, pre_sql_str, bufForColValuestmp);

            memset(cur, 0, sizeof(struct SHM_NODE));
            cur->next = shm->free;
            shm->free = cur;
            shm->count--;
            break;
        } else {
            prev = prev->next;
        }
    }
    shm->hashTable[hash % HASH_TABLE_SIZE] = dummy.next;
    return 0;
}

int saveSelectColInfosColValues(ORACLE_SHM_MEM *shm,
    int hash,
    unsigned int src_ip, unsigned int src_port, char *src_mac,
    unsigned int dst_ip, unsigned int dst_port, char *dst_mac,
    unsigned long capture_time, long requestCaptureTime, long responseCaptureTime,
    int line_num, int dir_id,
    char *sql,
    COLUMN_INFO *res, unsigned int num, unsigned int totalNum, char *colValues) {

    unsigned char *pos;
    struct SHM_NODE *node;
    char key[256];
    time_t now;
    int i, len;

    time(&now);

    /* �����ϻ��� */
    if (!(shm->free)) {
        if (shm->count > 0) checkOldList(shm);
        if (!(shm->free)) {
            printf("(!(shm->free))\n");
            exit(-1);
        }
    }

    /* ȡ��1��free�ṹ, ����������ṹ */
    node = shm->free;
    shm->free = node->next;

    node->hash = hash;
    node->src_ip = src_ip;
    node->src_port = src_port;
    strcpy(node->src_mac, src_mac);
    node->dst_ip = dst_ip;
    node->dst_port = dst_port;
    strcpy(node->dst_mac, dst_mac);
    node->capture_time = capture_time;
    node->selectPktCaptureTime = requestCaptureTime;
    node->lastResutlPktCaptureTime = responseCaptureTime;
    node->line_num = line_num;
    node->dir_id = dir_id;
    node->next = NULL;
    node->oldnext = NULL;
    node->oldprev = NULL;
    time(&(node->savetime));

    /* ����ͬһ��Ԫ��ļ�¼ */
    if (shm->count > 0) shmDelOneRecod(shm, hash, src_ip, src_port, dst_ip, dst_port);

    /* �ҵ�hash�� */
    node->next = shm->hashTable[hash % HASH_TABLE_SIZE];
    shm->hashTable[hash % HASH_TABLE_SIZE] = node;

    /* �ҵ��ϻ��� */
    node->oldnext = shm->old;
    if (shm->old) shm->old->oldprev = node;
    shm->old = node;

    shm->count++;

    redisContext * conn;
    redisReply *reply =NULL;
    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    /* ����rows�� */
    getRowsKey(src_ip, src_port, dst_ip, dst_port, key);
    reply =(redisReply*)redisCommand(conn,"set %s %s EX 120", key, colValues);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    /* ����colInfos�� */
    pos = redisColInfos;
    if (totalNum > 255) {
        *pos++ = 0x02;
        *pos++ = totalNum%256;
        *pos++ = totalNum/256;
    } else {
        *pos++ = 0x01;
        *pos++ = totalNum;
    }

    if (num > 255) {
        *pos++ = 0x02;
        *pos++ = num%256;
        *pos++ = num/256;
    } else {
        *pos++ = 0x01;
        *pos++ = num;
    }

    for (i=0; i<num; i++) {
        *pos++ = res[i].type;
        if (DATA_TYPE_VARCHAR2 == res[i].type || DATA_TYPE_CHAR == res[i].type) {
            *pos++ = (0x00 == res[i].charWidth) ? 0x01 : (res[i].charWidth); /* ��ֹcharWidthȡֵ����, ʹ���ڴ���\0 */
        }
        len = strlen(res[i].colName);
        *pos++ = len;
        strcpy(pos, res[i].colName);
        pos += len;
    }
    *pos = '\0';

    getColInfosKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %s EX 120", key, redisColInfos);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);
    reply = NULL;

    /* sql�����redis */
    getSqlKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %s EX 120", key, sql);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);
    redisFree(conn);
    reply = NULL;

    return 0;
}

/* 1, ��ȡ���е�
   2, ׷�������еĺ���
   3, дredis */
int saveColValues(ORACLE_SHM_MEM *shm, int hash, unsigned int src_ip, unsigned int src_port, unsigned int dst_ip, unsigned int dst_port, char *colValues, int line_num, unsigned long capture_time, unsigned long responseCaptureTime) {
    redisContext *conn = NULL;
    redisReply *reply =NULL;
    char key[128] = {0};
    struct SHM_NODE *cur, *prev, dummy;

    dummy.next = shm->hashTable[hash % HASH_TABLE_SIZE];
    prev = &dummy;
    while (prev->next) {
        cur = prev->next;
        if (cur->src_ip == src_ip && cur->src_port==src_port && cur->dst_ip==dst_ip && cur->dst_port==dst_port) {
            cur->lastResutlPktCaptureTime = responseCaptureTime;
            cur->line_num += line_num;
            break;
        } else {
            prev = prev->next;
        }
    }

    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    getRowsKey(src_ip, src_port, dst_ip, dst_port, key);

    /* �����Ѿ�д��redis��, �ȶ������е�, ��׷���ں��� */
    reply = (redisReply*)redisCommand(conn, "get %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    if (REDIS_REPLY_STRING == reply->type) {
        strncpy(bufForColValuestmp, reply->str, reply->len);
        bufForColValuestmp[reply->len] = '\0';
    } else {
        bufForColValuestmp[0] = '\0';
    }
    freeReplyObject(reply);

    strcat(bufForColValuestmp, colValues);

    reply = (redisReply*)redisCommand(conn, "set %s %s EX 120", key, bufForColValuestmp);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);
    redisFree(conn);
    reply = NULL;
    return 0;
}

int getColInfos(unsigned int src_ip, unsigned int src_port, unsigned int dst_ip, unsigned int dst_port, COLUMN_INFO *res, int *num) {
    unsigned char *pos = redisColInfos;
    int i;
    unsigned int len;
    int valueLen, totalNum;
    redisContext * conn;
    redisReply *reply =NULL;
    char key[128] = {0};

    getColInfosKey(src_ip, src_port, dst_ip, dst_port, key);

    conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        *num = 0;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    reply = (redisReply*)redisCommand(conn, "get %s", key);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    if (REDIS_REPLY_STRING == reply->type) {
        strncpy(redisColInfos, reply->str, reply->len);
        redisColInfos[reply->len] = '\0';
    } else {
        redisColInfos[0] = '\0';
    }
    freeReplyObject(reply);
    redisFree(conn);
    reply = NULL;

    #if 0
    printf("\n");
    for (i=0; redisColInfos[i]!= '\0'; i++) {
        printf("%02x ", redisColInfos[i]);
        if (i%16 == 0) printf("\n");
    }
    printf("\n");
    #endif

    if ('\0' == redisColInfos[0]) {
        *num = 0;
        return -1;
    }

    pos = redisColInfos;
    valueLen = strlen(redisColInfos);
    if (0x01 == *pos) {
        totalNum = pos[1];
        pos += 2;
    } else {
        totalNum = pos[1] + pos[2]*256;
        pos += 3;
    }

    if (0x01 == *pos) {
        *num = pos[1];
        pos += 2;
    } else {
        *num = pos[1] + pos[2]*256;
        pos += 3;
    }

    if (totalNum != *num) {
        printf("%d: not all infos\n", __LINE__);
    }

    for (i=0; i<*num && pos < redisColInfos+valueLen; i++) {
        res[i].type = *pos++;
        if (DATA_TYPE_VARCHAR2 == res[i].type || DATA_TYPE_CHAR == res[i].type) {
            res[i].charWidth = *pos++;
        }
        len = *pos++;
        memcpy(res[i].colName, pos, len);
        res[i].colName[len] = '\0';
        pos += len;
    }
    return 0;
}

/* ��ʽ:
   ����     �ֽ���
   ������     ���ֽ��Ǹ��ֶγ���
   ��ǰ����     ���ֽ��Ǹ��ֶγ���

   ��1����  1�ֽ�
   �ַ���� 1�ֽ�(ֻ�е�������0x01ʱ����)
   ����     ���ֽ��Ǹ��ֶγ���
   �����ظ���1�ĸ�ʽ
   */
int saveColInfos(char *key, COLUMN_INFO *res, unsigned int num, unsigned int totalNum) {
    unsigned char *pos;
    int i;
    unsigned int len, tmpNum, tmpTotal;
    redisContext * conn;
    redisReply *reply =NULL;

    /* ����redis */
    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }
    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    /* ��Ҫ: ����������, ��ֱ�Ӹ��� */
    /* tmpColInfos����redis���ѱ��������Ϣ(ODBCʱ������Ϣ�Ƕ���غϴ���) */
    tmpColInfos[0] = '\0';
    if (num == totalNum) {
        tmpNum = 0;
    } else {
        /* ��get */
        reply = (redisReply*)redisCommand(conn, "get %s", key);
        if(conn->err){
            fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
            if(reply){
                freeReplyObject(reply);
                return 0;
            }
            redisFree(conn);
        }
        if (REDIS_REPLY_STRING == reply->type) {
            strncpy(redisColInfos, reply->str, reply->len);
            redisColInfos[reply->len] = '\0';
            pos = redisColInfos;
            if (0x01 == *pos) {
                tmpTotal = pos[1];
                pos += 2;
            } else {
                tmpTotal = pos[1] + pos[2]*256;
                pos += 3;
            }

            if (0x01 == *pos) {
                tmpNum = pos[1];
                pos += 2;
            } else {
                tmpNum = pos[1] + pos[2]*256;
                pos += 3;
            }
            strcpy(tmpColInfos, pos);
        }
        freeReplyObject(reply);

        if (totalNum == tmpNum || totalNum != tmpTotal || num+tmpNum > totalNum) {
            tmpColInfos[0] = '\0';
            tmpNum = 0;
        }
    }

    /* ���� */
    pos = redisColInfos;
    if (totalNum > 255) {
        *pos++ = 0x02;
        *pos++ = totalNum%256;
        *pos++ = totalNum/256;
    } else {
        *pos++ = 0x01;
        *pos++ = totalNum;
    }

    tmpNum += num;
    if (tmpNum > 255) {
        *pos++ = 0x02;
        *pos++ = tmpNum%256;
        *pos++ = tmpNum/256;
    } else {
        *pos++ = 0x01;
        *pos++ = tmpNum;
    }
    strcpy(pos, tmpColInfos);
    pos += strlen(tmpColInfos);

    for (i=0; i<num; i++) {
        *pos++ = res[i].type;
        if (DATA_TYPE_VARCHAR2 == res[i].type || DATA_TYPE_CHAR == res[i].type) {
            *pos++ = (0x00 == res[i].charWidth) ? 0x01 : (res[i].charWidth);/* ��ֹcharWidthȡֵ����, ʹ���ڴ���\0 */
        }
        len = strlen(res[i].colName);
        *pos++ = len;
        strcpy(pos, res[i].colName);
        pos += len;
    }
    *pos = '\0';

    /* ��set */
    reply = (redisReply*)redisCommand(conn, "set %s %s EX 120", key, redisColInfos);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);
    redisFree(conn);
    reply = NULL;

    #if 0
    printf("\n");
    for (i=0; redisColInfos[i]!= '\0'; i++) {
        printf("%02x ", redisColInfos[i]);
        if (i%16 == 0) printf("\n");
    }
    printf("\n");
    #endif

    return 0;
}

int exists(ORACLE_SHM_MEM *shm, int hash, unsigned int src_ip, unsigned int src_port, unsigned int dst_ip, unsigned int dst_port) {
    struct SHM_NODE *cur;
    cur = shm->hashTable[hash % HASH_TABLE_SIZE];
    while (cur) {
        if (cur->src_ip == src_ip && cur->src_port==src_port && cur->dst_ip==dst_ip && cur->dst_port==dst_port) {
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}

long getFileTime(char *filename) {
    struct stat buf;
    if(stat(filename, &buf)<0) {
        return -1;
    }
    return (long)buf.st_mtime;
}

/* ɾ��'str'��ͷ��ĩβ�����Ŀհ��ַ�. ԭַ����
   ���ؽ�����ĳ��� */
int trim(char *str) {
    char *start = str,
         *end = str+strlen(str)-1;
    int num;

    if (NULL==str || '\0'==str[0]) return 0;

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

/* ��1���������Ķ�����ɴ�ӡ�ַ����һ���ո��Һ���\0
   ���ؽ�������� */
int unprintToSpace(char *src, int slen, char *dst, int dstMaxSize) {
    int i=0, j=0;

    while (i<slen && j<(dstMaxSize-1)) {
        if ('\0' == src[i]) {
            i++;
        } else if (isprint(src[i]) || (unsigned char)src[i] > 0x80) {
            dst[j++] = src[i++];
        } else {
            dst[j++] = ' ';
            while(i<slen && !isprint(src[i])) i++;
        }
    }
    dst[j] = '\0';

    return trim(dst);
}



char* memfind(const char* buf, const char* tofind, size_t len,size_t findlen) {
    if (findlen > len) {
        return((char*)NULL);
    }
    if (len < 1) {
        return((char*)buf);
    } else {
        const char* bufend = &buf[len - findlen + 1];
        const char* c = buf;
        for (; c < bufend; c++) {
            if (*c == *tofind) { // first letter matches
                if (!memcmp(c + 1, tofind + 1, findlen - 1)) { // found
                    return((char*)c);
                }
            }
        }
    }
    return((char*)NULL);
}

int getHexStr(char *data, int len, char *res, int resMaxSize) {
    int i, j;
    for (i=0, j=0; i<len && j<resMaxSize; i++, j+=3) {
        sprintf(res+j, "%02x ", data[i]);
    }
    return j;
}

int getHexStr2(char *data, int len, char *res, int resMaxSize) {
    int i, j;
    for (i=0, j=0; i<len && j<resMaxSize; i++, j+=2) {
        sprintf(res+j, "%02x", data[i]);
    }
    res[j] = '\0';
    return j;
}


#define deleteFile(file_request,file_response) \
do {\
    unlink(file_request);\
    unlink(file_response);\
}while(0)

int mvFile(char *file_request,char *file_response) {
    char command[2048];
    sprintf(command,"mv %s /home/oracle_debug/",file_request);
    system(command);
    sprintf(command,"mv %s /home/oracle_debug/",file_response);
    system(command);
    return 1;
}


/* �ַ���ת�����ɹ��򷵻ؽ�������ȣ�ʧ�ܷ���-1
   ע�� - GBK/GB2312�ȣ�����һ�֣���������֮��ת������iconv()�ϳ��� */
int codeConv(char* srcCharSet, char *dstCharSet, char *src, size_t srcLen, char *dst, size_t dstMaxSize) {
    iconv_t cd;
    size_t tmpLen = dstMaxSize;
    char *tmppos = dst;

    if (NULL==srcCharSet || NULL==dstCharSet || NULL==src || NULL==dst || srcLen<0 || dstMaxSize<0 ) {
        perror("Incorrect parameter\n");
        return -1;
    }

    /* �����߶���GBK��һ��ʱ������Ҫת�� */
    if ((('G'==srcCharSet[0] || 'g'==srcCharSet[0]) && ('B'==srcCharSet[1] || 'b'==srcCharSet[1]))
        && (('G'==dstCharSet[0] || 'g'==dstCharSet[0]) && ('B'==dstCharSet[1] || 'b'==dstCharSet[1]))) {
        memcpy(dst, src, srcLen);
        return srcLen;
    }

    cd = iconv_open(dstCharSet, srcCharSet);
    if((iconv_t)-1 == cd ){ perror("iconv_open() failed\n"); return -1; }

    memset(dst, 0, dstMaxSize);
    if(iconv(cd, &src, &srcLen, &tmppos, &tmpLen) < 0){
        iconv_close(cd);
        perror("iconv() failed\n");
        return -1;
    }
    iconv_close(cd);

    dst[dstMaxSize - tmpLen] = '\0';
    return (dstMaxSize - tmpLen);
}

void ucs2Swap(char *data, int datalen) {
    char tmp;
    int i;
    for (i=0; i<datalen-1; i+=2) {
        tmp = data[i];
        data[i] = data[i+1];
        data[i+1] = tmp;
    }
}

int ucs2Swap2(char *data, int datalen, char *res, int resMaxSize) {
    int i, toSwap;
    toSwap = (datalen <= resMaxSize) ? datalen : resMaxSize;
    for (i=0; i<toSwap-1; i+=2) {
        res[i] = data[i+1];
        res[i+1] = data[i];
    }
    return toSwap;
}

void setBitOpposite(unsigned char *data, int datalen) {
    int i;
    for (i=0; i<datalen; i++) {
        data[i] ^= 0xff;
    }
}

/* 111.1 : data[] = {0x40, 0x5b, 0xc7, 0x0a, 0x3d, 0x70, 0xa3}
   ������ = 0100 0000 0101 1011 1100 0111 0000 1010 0011 1101 0111 0000 1010 0011
   ����λF(1λ) = 0(+)
   ָ��λZ(11λ) = (100 0000 0101)2 = (1029)10
   β��W(23λ) = 0.1011 1100 0111 0000 1010 0011 1101 0111 0000 1010 0011 = 0.73609374999995225152815692126751
   ��IEEE754����ʾ��ʮ������ = (-1)^F * 2^(Z-1023) * (1+W) = 111.10999999999694409780204296112

   ��Z = 0, �����(-1)^F * 2^(-126) * W
   ��Z��0��0xff, �����(-1)^F * 2^(Z-127) * (1+W)
*/
double getFloat64IEEE754(unsigned char *data) {
    int F, Z, i;
    double W, base, n;
    F = (data[0]&0x80) >> 7;
    Z = ((data[0]&0x70)>>4)*256 + ((data[0]&0x0f)<<4) + ((data[1]&0xf0)>>4);
    W = 0;
    base = 0.5;
    W += ((data[1] & 0x08)>>3)*base; base *= 0.5;
    W += ((data[1] & 0x04)>>2)*base; base *= 0.5;
    W += ((data[1] & 0x02)>>1)*base; base *= 0.5;
    W += (data[1] & 0x01)*base; base *= 0.5;

    for (i=2; i<=7; i++) {
        W += ((data[2] & 0x80)>>7)*base; base *= 0.5;
        W += ((data[2] & 0x40)>>6)*base; base *= 0.5;
        W += ((data[2] & 0x20)>>5)*base; base *= 0.5;
        W += ((data[2] & 0x10)>>4)*base; base *= 0.5;
        W += ((data[2] & 0x08)>>3)*base; base *= 0.5;
        W += ((data[2] & 0x04)>>2)*base; base *= 0.5;
        W += ((data[2] & 0x02)>>1)*base; base *= 0.5;
        W += (data[2] & 0x01)*base; base *= 0.5;
    }

    if (0 == Z) return (W * ((1==F)?(-1.0):(1.0))* (2.2250738585072013830902327173324e-308));

    n = 1;
    if (Z >= 1023) {
        for (i=0 ;i<Z-1023; i++) {
            n *= 2;
        }
    } else {
        for (i=0 ;i<1023-Z; i++) {
            n *= 0.5;
        }
    }

    return ((W+1.0)* n * ((1==F)? -1.0 : 1.0));
}

/* data[] = {0xb6, 0x33, 0x24, 0x40},���λ�����
   ��ʾIEEE754�� = 0x402433b6
   ������ = 0100 0000 0010 0100 0011 0011 1011 0110
   ����λF(1λ) = 0(+)
   ָ��λZ(8λ) = (100 0000 0)2 = (128)10
   β��W(23λ) = 0.0100100001100111011010100111001
   ��IEEE754����ʾ��ʮ������ = (-1)^F * 2^(Z-127) * (1+W)
   Ӧ����2.565656

   ��Z = 0, �����(-1)^F * 2^(-126) * W
   ��Z��0��0xff, �����(-1)^F * 2^(Z-127) * (1+W)

=========================================================================
111.1 = 01000010110111100011001100110010,(42DE3332)
F = 0
Z = 10000101 = 133
W = 0.10111100011001100110010 = 0.7359373569488525390625
(-1)^F * 2^(Z-127) * (1+W) = 1 * 64 * 1.7359373569488525390625 = 111.0999908447265625

-111.1 = 11000010110111100011001100110010,(C2DE3332)
F = 1
Z = 10000101 = 133
W = 0.10111100011001100110010 = 0.7359373569488525390625
(-1)^F * 2^(Z-127) * (1+W) = -1 * 64 * 1.7359373569488525390625 = 111.0999908447265625

=========================================================================
4321.5324 = 01000101100001110000110001000010,(45870C42)
F = 0
Z = 10001011 = 139
M = 0.00001110000110001000010 =0.0550615787506103515625
F = 0
(-1)^F * 2^(Z-127) * (1+W) = 1 * 4096 * 1.0550615787506103515625 = 4321.5322265625

-4321.5324 = 11000101100001110000110001000010,(C5870C42)
F = 1
Z = 10001011 = 139
M = 0.00001110000110001000010 = 0.0550615787506103515625
(-1)^F * 2^(Z-127) * (1+W) = -1 * 4096 * 1.0550615787506103515625 = 4321.5322265625

=========================================================================
0 = 00000000000000000000000000000000,(0)

=========================================================================
1 = 00111111100000000000000000000000,(3F800000)
F = 0
Z = 01111111 = 127
W = 00000000000000000000000
(-1)^F * 2^(Z-127) * (1+W) = 1 * 1 * 1 = 1

-1 = 10111111100000000000000000000000,(BF800000)
F = 1
Z = 01111111 = 127
W = 00000000000000000000000 = 0
(-1)^F * 2^(Z-127) * (1+W) = -1 * 1 * 1 = -1

=========================================================================
0.75 = 00111111010000000000000000000000,(3F400000)
F = 0
Z = 01111110 = 126
W = 0.10000000000000000000000 = 0.5
(-1)^F * 2^(Z-127) * (1+W) = 1 * 0.5 * 1.5 = 0.75

=========================================================================
-2.5 = 11000000001000000000000000000000,(C0200000)
F = 1
Z = 10000000 = 128
W = 0.01000000000000000000000 = 0.25
(-1)^F * 2^(Z-127) * (1+W) = -1 * 2 * 1.25 = -2.5
*/

double getFloat32IEEE754(unsigned char *data) {
    int F, Z, i;
    double W, base, n;
    F = (data[0]&0x80) >> 7;
    Z = ((data[0]&0x7f)<<1) + ((data[1]&0x80)>>7);
    W = 0;
    base = 0.5;
    W += ((data[1] & 0x40)>>6)*base; base *= 0.5;
    W += ((data[1] & 0x20)>>5)*base; base *= 0.5;
    W += ((data[1] & 0x10)>>4)*base; base *= 0.5;
    W += ((data[1] & 0x08)>>3)*base; base *= 0.5;
    W += ((data[1] & 0x04)>>2)*base; base *= 0.5;
    W += ((data[1] & 0x02)>>1)*base; base *= 0.5;
    W += (data[1] & 0x01)*base; base *= 0.5;

    W += ((data[2] & 0x80)>>7)*base; base *= 0.5;
    W += ((data[2] & 0x40)>>6)*base; base *= 0.5;
    W += ((data[2] & 0x20)>>5)*base; base *= 0.5;
    W += ((data[2] & 0x10)>>4)*base; base *= 0.5;
    W += ((data[2] & 0x08)>>3)*base; base *= 0.5;
    W += ((data[2] & 0x04)>>2)*base; base *= 0.5;
    W += ((data[2] & 0x02)>>1)*base; base *= 0.5;
    W += (data[2] & 0x01)*base; base *= 0.5;

    W += ((data[3] & 0x80)>>7)*base; base *= 0.5;
    W += ((data[3] & 0x40)>>6)*base; base *= 0.5;
    W += ((data[3] & 0x20)>>5)*base; base *= 0.5;
    W += ((data[3] & 0x10)>>4)*base; base *= 0.5;
    W += ((data[3] & 0x08)>>3)*base; base *= 0.5;
    W += ((data[3] & 0x04)>>2)*base; base *= 0.5;
    W += ((data[3] & 0x02)>>1)*base; base *= 0.5;
    W += (data[3] & 0x01)*base; base *= 0.5;

    if (0 == Z) return (W * ((1==F)?(-1.0):(1.0))* (1.1754943508222875079687365372222e-38));

    n = 1;
    if (Z >= 127) {
        for (i=0 ;i<Z-127; i++) {
            n *= 2;
        }
    } else {
        for (i=0 ;i<127-Z; i++) {
            n *= 0.5;
        }
    }

    return ((W+1.0)* n * ((1==F)? -1.0 : 1.0));
}

int tnsRebuild(unsigned char *data, long datalen, unsigned char *res, int resMaxSize) {
    unsigned char *pos=data;
    char *resPos=res;
    int leftlen = datalen, reslen=0, len;
    TNS_HEADER *tnsh = (TNS_HEADER *)pos;

    len = ntohs(tnsh->length);
    if (len >= datalen) {
        memcpy(res+10, data, datalen-10);
        return datalen-10;
    }

    while (leftlen > 0) {
        memcpy(resPos, pos+10, len-10);
        reslen += (len-10);
        resPos += (len-10);
        pos += len;
        leftlen -= len;

        if (leftlen <= 0) break;
        tnsh = (TNS_HEADER *)pos;
        len = ntohs(tnsh->length);
    }
    return reslen;
}

/* ���ؽ�����data�����ֽ�
   @resMaxSize����@res���ռ䣬����󷵻ش���res���ֽ���
   ע: �ڽ����ĩβ���\0 */
int segmentsReform(unsigned char *data, char *res, int *resMaxSize) {
    int len, parsedLen=0, reslen=0;
    unsigned char *pos = data;
    char *respos = res;

    while ((unsigned char)*pos != 0x00) {
        len = *pos++;
        memcpy(respos, pos, len);
        pos += len;
        parsedLen += (1 + len);
        respos += len;
        reslen += len;
    }
    res[reslen] = '\0';

    *resMaxSize = reslen;
    return parsedLen+1; /* ����������β��0x0 */
}

/* ���ؽ�������ĳ��ȣ�@datalen���ؽ�����data�ĳ��� */
int parseCHAR(unsigned char *data, int *datalen, char *res, int resSize) {
    unsigned char *pos = data;
    int len, tmplen;

    /* null */
    if (0x00 == *pos) {
        strcpy(res, "null");
        *datalen = 1;
        return 4;
    }

    if (0xfe == *pos) {
        pos++;
        len = segmentsReform(pos, res, &tmplen);
        *datalen = len + 1; /* �����˿�ͷ��0xfe */
        return tmplen;
    }

    len = *pos++;
    memcpy(res, pos, len);
    res[len] = '\0';
    *datalen = len+1;
    return len;
}

/* ���ؽ�������ĳ��ȣ�@datalen���ؽ�����data�ĳ��� */
int parseNUMBER(unsigned char *data, int *datalen, char *res, int resSize) {
    /* 0x02: NUMBER, INTEGER, FLOAT */
    unsigned char *pos = data;
    char *resPos = res;
    int len;
    int intLen, docLen, padLen, j;

    /* null */
    if (0x00 == (unsigned char)*pos) {
        strcpy(res, "null");
        *datalen = 1;
        return 4;
    }

    /* 0x01 0x80 */
    if (0x01 == (unsigned char)pos[0] && 0x80 == (unsigned char)pos[1]) {
        res[0] = '0';
        res[1] = '\0';
        *datalen = 2;
        return 1;
    }

    len = *pos++;
    if ((unsigned char)*pos >= 0xc0) {
        /* ���� */
        intLen = (unsigned char)*pos - 0xc0;
        if (intLen >= len-1) {
            padLen = intLen - (len -1);
            intLen = len - 1;
            docLen = 0;
        } else {
            padLen = 0;
            docLen = len - intLen -1;
        }
        pos++;

        if ( 0 == intLen) {
            *resPos++ = '0';
        } else {
            for (j=0; j<intLen; j++) {
                if (j!=0) sprintf(resPos, "%02d", pos[j]-1);
                else sprintf(resPos, "%d", pos[j]-1);
                resPos += strlen(resPos);
            }
            pos += intLen;
            for (j=0; j<padLen; j++) {
                *resPos++ = '0';
                *resPos++ = '0';
            }
        }
        if (docLen != 0) {
            *resPos++ = '.';
            for (j=0; j<docLen; j++) {
                sprintf(resPos, "%02d", pos[j]-1);
                resPos += strlen(resPos);
            }
            pos += docLen;
        }
    } else if ((unsigned int)*pos <= (unsigned int)0x3f){
        /* ���� */
        intLen = 0x3f - (unsigned int)*pos;
        *resPos++ = '-';
        if (intLen >= len-1-1) {
            padLen = intLen - (len - 1 - 1);
            intLen = len - 1 - 1;
            docLen = 0;
        } else {
            padLen = 0;
            docLen = len - intLen - 1 - 1; /* ��1���ַ������1���ַ�0x66(0x66�Ǹ�������) */
        }
        pos++;

        if ( 0 == intLen) {
            *resPos++ = '0';
        } else {
            for (j=0; j<intLen; j++) {
                if (j!=0) sprintf(resPos, "%02d", 101-pos[j]);
                else sprintf(resPos, "%d", 101-pos[j]);
                resPos += strlen(resPos);
            }
            for (j=0; j<padLen; j++) {
                *resPos++ = '0';
                *resPos++ = '0';
            }
            pos += intLen;
        }
        if (docLen != 0) {
            *resPos++ = '.';
            for (j=0; j<docLen; j++) {
                sprintf(resPos, "%02d", 101-pos[j]);
                resPos += strlen(resPos);
            }
            pos += docLen;
        }
        pos++; /* ����ĩβ��0x66 */
    }

    *datalen = pos-data;
    return resPos-res;
}

/* ���ؽ�������ĳ��ȣ�@datalen���ؽ�����data�ĳ��� */
int parseDATE(unsigned char *data, int *datalen, char *res, int resSize) {
    /* null */
    if (0x00 == data[0]) {
        strcpy(res, "null");
    } else if (0x07 == data[0]){
        if (0x64==data[1]) {
            sprintf(res, "%02d-%02d-%02d %02d:%02d:%02d",
                data[2]-100, data[3], data[4], data[5]-1, data[6]-1, data[7]);
        } else {
            sprintf(res, "%d%02d-%02d-%02d %02d:%02d:%02d",
                ((data[1]>100) ? (data[1]-100) : (100-data[1])),
                data[2]-100, data[3], data[4], data[5]-1, data[6]-1, data[7]);
        }
    } else {
        printf("not DATE type\n");
        res[0] = '\0';
        /* *datalen���䣬����������������ֶ� */
        return 0;
    }
    *datalen = (data[0]+1);
    return strlen(res);
}

/* ���ؽ�������ĳ��ȣ�@datalen���ؽ�����data�ĳ��� */
int parseTIMESTAMP(unsigned char *data, int *datalen, char *res, int resSize) {
    /* null */
    if (0x00 == data[0]) {
        strcpy(res, "null");
    } else if (0x07 == data[0]) {
        if (0x64==data[1]) {
            sprintf(res, "%02d-%02d-%02d %02d:%02d:%02d.000000000",
                data[2]-100, data[3], data[4], data[5]-1, data[6]-1, data[7]-1);
        } else {
            sprintf(res, "%d%02d-%02d-%02d %02d:%02d:%02d.000000000",
                ((data[1]>=100) ? (data[1]-100) : (100-data[1])),
                data[2]-100, data[3], data[4], data[5]-1, data[6]-1, data[7]-1);
        }
    } else if (0x0b == data[0]) {
        unsigned long n = 0;
        int i;
        for (i=8; i<= 11; i++) n = n * 256 + data[i];
        if (0x64==data[1]) {
            sprintf(res, "%02d-%02d-%02d %02d:%02d:%02d.%lu",
                data[2]-100, data[3], data[4], data[5]-1, data[6]-1, data[7]-1, n);
        } else {
            sprintf(res, "%d%02d-%02d-%02d %02d:%02d:%02d.%lu",
                ((data[1]>=100) ? (data[1]-100) : (100-data[1])),
                data[2]-100, data[3], data[4], data[5]-1, data[6]-1, data[7]-1, n);
        }
    } else {
        printf("not TIMESTAMP type\n");
        res[0] = '\0';
        /* *datalen���䣬����������������ֶ� */
        return 0;
    }

    *datalen = (data[0]+1);
    return strlen(res);
}

#define parseLONG_RAW(data, datelen, res, resSize) parseLONG(data, datelen, res, resSize)

/* ���ؽ�������ĳ��ȣ�@datalen���ؽ�����data�ĳ��� */
int parseLONG(unsigned char *data, int *datalen, char *res, int resSize) {
    unsigned char *pos = data;
    int len, tmplen;

    /* null */
    if (0x00 == data[0]) {
        strcpy(res, "null");
        *datalen = 5;
        return 4;
    } else if (0xfe == data[0]) {
        pos++;
        len = segmentsReform(pos, res, &tmplen);
        *datalen = len + 5; /* ��ͷ��0xfe, ĩβ��4�ֽ�0x00 */
        return tmplen;
    }else {
        printf("not LONG type\n");
        res[0] = '\0';
        /* *datalen���䣬����������������ֶ� */
        return 0;
    }

    return 0;
}

/* ���ؽ�������ĳ��ȣ�@datalen���ؽ�����data�ĳ��� */
/* RAW���ͽ�������16������ʽ��ʾ, ������ʽ��char���� */
int parseRAW(unsigned char *data, int *datalen, char *res, int resSize) {
    unsigned char *pos = data;
    int len, tmplen;

    /* null */
    if (0x00 == *pos) {
        strcpy(res, "null");
        *datalen = 1;
        return 4;
    }

#if 0
    /* ԭ�� */
    if (0xfe == *pos) {
        pos++;
        len = segmentsReform(pos, res, &tmplen);
        *datalen = len+1; /* �����˿�ͷ��0xfe */
        return tmplen;
    }

    len = *pos++;
    memcpy(res, pos, len);
    res[len] = '\0';
    *datalen = len + 1;
    return len;
#else
    /* ʮ�����Ƹ�ʽ */
    if (0xfe == *pos) {
        pos++;
        len = segmentsReform(pos, bufForBigCharRebuid, &tmplen);
        *datalen = len+1; /* �����˿�ͷ��0xfe */
        return getHexStr2(bufForBigCharRebuid, tmplen, res, resSize);
    }

    len = *pos++;
    memcpy(bufForBigCharRebuid, pos, len);
    bufForBigCharRebuid[len] = '\0';
    *datalen = len + 1;
    return getHexStr2(bufForBigCharRebuid, len, res, resSize);
#endif
}

/* ������ֵ�����ֽ��ǳ���
   @info������Ϣ, @res��������, @datalen����data����, ���ؽ�������ֵ���ֳ���.
   �������ؽ��������. */
int parseColValue(unsigned char *data, int *datalen, COLUMN_INFO *info, char *res, int resMaxSize) {
    unsigned char *pos = data;
    char *resPos=res;
    int len, tmp, j, leftlen;

    switch (info->type) {
        case DATA_TYPE_VARCHAR2:
        case DATA_TYPE_CHAR:
            /* 0x60:CHAR, NCHAR; 0x01:VARCHAR2, NVARCHAR */
            leftlen = *datalen - (pos - data);
            tmp = parseCHAR(pos, &leftlen, bufForBigCharRebuid, sizeof(bufForBigCharRebuid)-1);
            pos += leftlen;
            if (tmp > 0) {
                if (0x02 == info->charWidth) {
                    ucs2Swap(bufForBigCharRebuid, tmp);
                    tmp = codeConv("UCS-2", TNS_CHARSET, bufForBigCharRebuid, tmp, resPos, resMaxSize-(resPos-res));
                    if (tmp < 0) {
                        return TNS_SAVE;
                    }
                } else if (0x01 == info->charWidth) {
                    if (CLI_ORACLE_SQL_DEVELOPER == clientType) {
                        tmp = codeConv("UTF-8", TNS_CHARSET, bufForBigCharRebuid, tmp, resPos, resMaxSize-(resPos-res));
                        if (tmp < 0) {
                            return TNS_SAVE;
                        }
                    } else {
                        memcpy(resPos, bufForBigCharRebuid, tmp);
                        resPos[tmp] = '\0';
                    }
                }
                resPos += tmp;
            }
            break;
        case DATA_TYPE_NUMBER:
            /* 0x02: NUMBER, INTEGER, FLOAT */
            leftlen = *datalen - (pos - data);
            tmp = parseNUMBER(pos, &leftlen, resPos, resMaxSize-(resPos-res));
            pos += leftlen;
            resPos += tmp;
            break;
        case DATA_TYPE_BINARY_FLOAT:
            /* 0x64: BINARY_FLOAT */
            len = *pos++;
            if ((unsigned char)*pos >= 0x80) *pos = *pos - 0x80;
            else setBitOpposite(pos, 8);
            sprintf(resPos, "%lf", getFloat32IEEE754(pos));
            resPos += strlen(resPos);
            pos += len;
            break;
        case DATA_TYPE_BINARY_DOUBLE:
            /* 0x65: BINARY_DOUBLE */
            len = *pos++;
            if ((unsigned char)*pos >= 0x80) *pos = *pos - 0x80;
            else setBitOpposite(pos, 8);
            sprintf(resPos, "%lf", getFloat64IEEE754(pos));
            resPos += strlen(resPos);
            pos += len;
            break;
        case DATA_TYPE_DATE:
            /* 0x0c: DATE */
            leftlen = *datalen - (pos - data);
            tmp = parseDATE(pos, &leftlen, resPos, resMaxSize-(resPos-res));
            pos += leftlen;
            resPos += tmp;
            break;
        case DATA_TYPE_TIMESTAMP:
            /* 0xb4: TIMESTAMP */
            leftlen = *datalen - (pos - data);
            tmp = parseTIMESTAMP(pos, &leftlen, resPos, resMaxSize-(resPos-res));
            pos += leftlen;
            resPos += tmp;
            break;
        case DATA_TYPE_LONG:
            /* 0x08: LONG */
            leftlen = *datalen - (pos - data);
            tmp = parseLONG(pos, &leftlen, resPos, resMaxSize-(resPos-res));
            pos += leftlen;
            resPos += tmp;
            break;
        case DATA_TYPE_LONG_RAW:
            /* 0x18: LONG RAW */
            leftlen = *datalen - (pos - data);
            tmp = parseLONG_RAW(pos, &leftlen, resPos, resMaxSize-(resPos-res));
            pos += leftlen;
            resPos += tmp;
            break;
        case DATA_TYPE_RAW:
            /* 0x17: RAW */
            leftlen = *datalen - (pos - data);
            tmp = parseRAW(pos, &leftlen, resPos, resMaxSize-(resPos-res));
            pos += leftlen;
            resPos += tmp;
            break;
        default:
            len = *pos++;
            for (j=0; j<len; j++) {
                sprintf(resPos, "%02x", (unsigned char)pos[j]);
                resPos += 2;
            }
            pos += len;
    }

    *datalen = pos - data;
    return resPos-res;
}

int isInBitmap(char *bitmap, int i) {
    return ((0x1 << (i%8)) & bitmap[i/8]);
}

#if 0
/* request = 03 2b, response = 08 0xnn, �ظ�����Ϣ
���� : 10g server, ODBC(�ɶ���������ҽԺ, �ն˷���)
*/
int response_032B_08XX(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos, *pos22;
    int len, i;
    int totalColNum, curColNum;

    if (datalen < 47) return TNS_DEL;

    pos = data;
    totalColNum = pos[1] + pos[2]*256;
    curColNum = pos[3] + pos[4]*256;
    pos += 5;
    if (totalColNum<=0 || curColNum<=0 || totalColNum<=curColNum ) return -1;

    if (totalColNum > MAX_COLUMN) {
        printf("%d:too many column (%d)\n", __LINE__, totalColNum);
        return TNS_SAVE;
    }

    for (i=0; i<curColNum && pos<data+datalen; i++) {
    #if TNS_BINARY_DEBUG
        int iii;
        printf("%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            printf("0x%02x ", (unsigned char)pos[iii]);
        }
        printf("\n");
    #endif
        colInfos[i].type = *pos++;
        pos += 23;

        /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
        if (0x60 == colInfos[i].type || 0x01 == colInfos[i].type) {
            colInfos[i].charWidth = *pos;
        }
        pos += 15;
    }
    len = pos[0] + pos[1]*256;

    /* �������� */
    if (len+2 > datalen-(pos-data)) {
        len = datalen-(pos-data) - 2;
    }
    pos += 2;
    memcpy(res, pos, len);
    res[len] = '\0';

    for (i=0; i<curColNum && pos<data+datalen; i++) {
        pos22 = pos;
        while (*pos22 != 0x22 && pos22 <data+datalen) pos22++;
        if (pos22 >= data+datalen) break;
        if (pos22 != pos) {
            memcpy(colInfos[i].colName, pos, pos22-pos);
            colInfos[i].colName[pos22-pos] = '\0';
        }
        else {
            strcpy(colInfos[i].colName, "--");
        }
        pos = pos22+1;
    }
    if (i < curColNum) {
        for (; i<curColNum; i++) {
            strcpy(colInfos[i].colName, "--");
        }
    }

    char key[200];
    getColInfosKey(src_ip, src_port, dst_ip, dst_port, key);
    saveColInfos(key, colInfos, curColNum, totalColNum);

    return TNS_NORMAL;
}

/* request = 03 47, response = 06 02, �ظ���ֵ
���� : ODBC ���� 64bits oracle 10g server(�ɶ���������ҽԺ, �ն˷���)
*/
int response_0347_0602(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    char *resPos=res;
    int i, tmp, leftlen;
    int curColNum, totalColNum, seq;
    char bitmap[MAX_COLUMN/8+1];
    int bitmapSize;
    int bb;

    if (datalen < 16) return TNS_DEL;

    pos = data;
    totalColNum = pos[2] + pos[3]*256;
    memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
    totalColInfoNum = MAX_COLUMN;
    getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
    if (totalColInfoNum != totalColNum) {
        return TNS_SAVE;
    }
    for (i=0; i<totalColNum && pos<data+datalen; i++) {
        if (i != totalColNum-1) sprintf(resPos, "%s;:s", colInfos[i].colName);
        else sprintf(resPos, "%s;:n", colInfos[i].colName);
        resPos += strlen(resPos);
    }
    while (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1]) && pos<data+datalen) {
        line_num++;

        pos += 2;

        /* ��06 02��������, ������һ����ͬʱ��ȱ�� */
        curColNum = pos[0] + pos[1]*256;
        pos += 2;

        seq = pos[0] + pos[1]*256;
        pos += 2;

        if (0 == seq) {
            pos += 12;
            for (bb=0; bb<(totalColInfoNum/8+1); bb++) {
                bitmap[bb] = 0xff;
            }
        } else {
            pos += 8;
            bitmapSize = *pos++;
            memcpy(bitmap, pos, bitmapSize);
            pos += (bitmapSize + 4);
        }
        pos++;

        for (i=0; i<totalColNum && pos<data+datalen; i++) {
            if (!isInBitmap(bitmap, i)) {
                if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                resPos += prevRow.colSizes[i];
                if (i != totalColNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
                continue;
            }
            prevRow.colValues[i] = resPos;

            if (0x00 == *pos) {
                /* 00 ff ff 00 00 */
                strcpy(resPos, "null");
                resPos += 4;
                pos += 3;
            } else if (0xff == *pos) {
                strcpy(resPos, "null");
                resPos += 4;
                while (0xff == *pos && pos<data+datalen) pos++;
            }else {
                leftlen = datalen - (pos - data);
                tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                pos += leftlen;
                resPos += tmp;

                pos += 2;
            }
            prevRow.colSizes[i] = resPos - prevRow.colValues[i];
            if (i != totalColNum-1) strcpy(resPos, ";:s");
            else strcpy(resPos, ";:n");
            resPos += 3;
        }
    }
    return TNS_NORMAL;
}
#endif

/* request = 03 5e, response = 06 02, �ظ���ֵ
���� : 10g server, PL/sql developer(�ɶ���������ҽԺ)
*/
#define UNMEET_PKT 10
#define QINGYANG_PKT 1
#define HOME_PKT 2

int saveUsernameAndClientType(char *username, int clientType) {
    char key[128] = {0};
    redisContext *conn = NULL;
    redisReply *reply =NULL;

    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn, "select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    getUsernameKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %s EX 86400", key, username);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);

    getClientKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %d EX 86400", key, clientType);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);

    redisFree(conn);
    reply = NULL;

    logMsg("home32bit11gr2_request_0376: username=%s, clientType=%d, key=%s\n", username, clientType, key);
}


int qingyang64bit10g_plsql_response_035e_0602(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    char *resPos=res;
    int i, tmp, leftlen;
    int curColNum, seq;
    char bitmap[MAX_COLUMN/8+1];
    int bitmapSize;
    int bb;

    res[0] = '\0';

    pos = data;
    while (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1]) && pos<data+datalen) {
        line_num++;

        pos += 2;

        /* ��06 02��������, λͼ����ʱ, ����С�ڲ�ѯ���� */
        curColNum = pos[0] + pos[1]*256;
        pos += 2;

        seq = pos[0] + pos[1]*256;
        pos += 2;

        if (0 == seq) {
            pos += 12;

            /* ��ȡ�и�ʽ, ��������� */
            if (0 == totalColInfoNum) {
                totalColInfoNum = MAX_COLUMN;
                getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
                if (totalColInfoNum != curColNum) {
                    return TNS_SAVE;
                }
            }

            /* ����λͼ */
            for (bb=0; bb<(totalColInfoNum/8+1); bb++) {
                bitmap[bb] = 0xff;
            }

            /* ��ʼ����һ������ */
            memset(&prevRow, 0, sizeof(PREV_ROW_INFO));

            /* ���ɱ�ͷ */
            for (i=0; i<totalColInfoNum && pos<data+datalen; i++) {
                if (i != totalColInfoNum-1) sprintf(resPos, "%s;:s", colInfos[i].colName);
                else sprintf(resPos, "%s;:n", colInfos[i].colName);
                resPos += strlen(resPos);
            }
        } else {
            pos += 8;
            bitmapSize = *pos++;
            memcpy(bitmap, pos, bitmapSize);
            pos += (bitmapSize + 4);
        }
        pos++;

        for (i=0; i<totalColInfoNum && pos<data+datalen; i++) {
            if (!isInBitmap(bitmap, i)) {
                if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                resPos += prevRow.colSizes[i];
                if (i != totalColInfoNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
                continue;
            }
            prevRow.colValues[i] = resPos;

            if (0x00 == *pos) {
                /* 00 ff ff 00 00 */
                strcpy(resPos, "null");
                resPos += 4;
                pos += 5;
            } else if (0xff == *pos) {
                strcpy(resPos, "null");
                resPos += 4;
                while (0xff == *pos && pos<data+datalen) pos++;
                pos += 2;
            }else {
                leftlen = datalen - (pos - data);
                tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                pos += leftlen;
                resPos += tmp;

                pos += 4;
            }
            prevRow.colSizes[i] = resPos - prevRow.colValues[i];
            if (i != totalColInfoNum-1) strcpy(resPos, ";:s");
            else strcpy(resPos, ";:n");
            resPos += 3;
        }
    }
    if (datalen-(pos-data) > 0 && memfind(pos, "ORA-01403", datalen-(pos-data), 9)) {
        isLast = 1;
    }
    return TNS_NORMAL;
}

/* request = 03 5e, response = 10 17, sql��������, �ͻظ�����Ϣ
��ʱ03 5e��ǰ����1��11 6b��, ��Ҫ���ƫ�Ƶ�03 5e������
���� : 10g server, PL/sql developer(�ɶ���������ҽԺ)
�����06 02, ����Ž����������˳�
*/
int qingyang64bit10g_plsql_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';

    pos = data + 29; /* ����0x10 0x17 */

    /* ���� */
    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
    if (colNum > MAX_COLUMN) {
        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
        return TNS_SAVE;
    }
    totalColInfoNum = colNum;

    pos += 5;

    /* ������ */
    for (i=0; i<colNum && pos<data+datalen; i++) {
        #if TNS_BINARY_DEBUG
        int iii;
        fprintf(stderr, "%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
        }
        fprintf(stderr, "\n");
        #endif

        colInfos[i].type = *pos++; /* ������ */
        pos += 23;
        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
        pos += 6;

        /* ���� */
        memcpy(colInfos[i].colName, pos+1, pos[0]);
        colInfos[i].colName[pos[0]] = '\0';
        pos += (1 + pos[0]);

        pos += 8;
    }

    if (i < colNum) {
        return TNS_SAVE;
    }

    pos += 12;

    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
        return qingyang64bit10g_plsql_response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
    }

    return TNS_NORMAL;
}

int qingyang64bit10g_plsql_request(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int sqlLen, tocopy;

    /* �ȳ�ʼ��Ϊ�մ�, ��ֹû�ҵ�sql���ֱ�ӷ��� */
    res[0] = '\0';

    pos = memfind(data, "\x03\x5e", datalen, 2);
    if (pos) {
        pos += 12;
        sqlLen = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
        pos += (4 + 29);
        tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize-1;
        memcpy(res, pos, tocopy);
        res[tocopy] = '\0';
        return TNS_NORMAL;
    } else {
        return TNS_SAVE;
    }
}

int qingyang64bit10g_plsql_request_0376(unsigned char *data, int len) {
    unsigned char *pos = data+ (10 + 4);
    char username[512] = {0};
    char key[128] = {0};
    int sqllen;

    sqllen = *pos;
    pos += 15;

    memcpy(username, pos, len);
    username[len] = '\0';

    redisContext *conn = NULL;
    redisReply *reply =NULL;

    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    getUsernameKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %s EX 86400", key, username);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);
    redisFree(conn);
    reply = NULL;

    logMsg("qingyang64bit10g_plsql_request_0376: username=%s, key=%s\n", username, key);

    return 0;
}


int response_035e_0602_reserve(unsigned char *data, long datalen, char *res, int resMaxSize) {
    if (datalen < 16) return TNS_DEL;

    if (0x07 == data[18]) flagLocation = QINGYANG_PKT;
    else flagLocation = HOME_PKT;

    if (QINGYANG_PKT == flagLocation)
        return qingyang64bit10g_plsql_response_035e_0602(data, datalen, res, resMaxSize);
    else
        return home64bit10g_response_035e_0602(data, datalen, res, resMaxSize);
    return TNS_NORMAL;
}



/* request = 03 5e, response = 10 17, sql��������, �ͻظ�����Ϣ
���� : 10g server, PL/sql developer(�ɶ���������ҽԺ)������64λ10g�汾�Ļ�Ͻ���
�����06 02, ����Ž����������˳�
*/
int response_035e_1017_reserve(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;
    int flagLocation = UNMEET_PKT; /* �������ķ������� */

    res[0] = '\0';

    if (datalen < 34) return TNS_DEL;

    pos = data + (2 + 16 + 7 + 4); /* ����0x10 0x17 */

    /* ���� */
    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
    pos += 4;
    if (colNum > MAX_COLUMN) {
        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
        return TNS_SAVE;
    }
    totalColInfoNum = colNum;

    pos++;

    /* ������ */
    for (i=0; i<colNum && pos<data+datalen; i++) {
        #if TNS_BINARY_DEBUG
        int iii;
        fprintf(stderr, "%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
        }
        fprintf(stderr, "\n");
        #endif

        /* ������ */
        colInfos[i].type = *pos++;
        pos++; /* δ֪, Ŀǰ����varchar2��char��clob��long�����г��֣�ֵΪ0x80 */

        /* NUMBER,FLOAT��precision��scale */
        colInfos[i].precision = *pos++;
        colInfos[i].scale = *pos++;
        colInfos[i].maxByteSize = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
        pos += 4;

        pos += 16;

        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */

        if (UNMEET_PKT == flagLocation) {
            if (DATA_TYPE_VARCHAR2 == colInfos[i].type || DATA_TYPE_CHAR == colInfos[i].type) {
                if ((pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256) == colInfos[i].maxByteSize)
                    flagLocation = HOME_PKT;
                else
                    flagLocation = QINGYANG_PKT;
            } else {
                if (pos[1] == pos[2] && pos[1]!=0x0) flagLocation = QINGYANG_PKT;
                else flagLocation = HOME_PKT;
            }
        }

        if (HOME_PKT == flagLocation) {
            pos += 4;
        }

        colInfos[i].nullbit = *pos++;
        pos += 5;

        /* ���� */
        memcpy(colInfos[i].colName, pos+1, pos[0]);
        colInfos[i].colName[pos[0]] = '\0';
        pos += (1 + pos[0]); /* ����colName */

        pos += 8;

        if (HOME_PKT == flagLocation) {
            colInfos[i].seq = pos[0] + pos[1]*256;
            pos += 2;
        }
    }

    if (i < colNum) {
        return TNS_SAVE;
    }

    pos += 12;
    if (HOME_PKT == flagLocation) {
        pos += 16;
    }

    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
    }

    return TNS_NORMAL;
}

int responseParserHelper(unsigned char *data, long datalen, char *res, int resMaxSize, int type) {
    int tmp = -1;
    /* PL/sql developer7 ����64bit Oracle 10g */
    switch (type) {
        case PKT_035E__1017:
            tmp = response_035e_1017(data, datalen, res, resMaxSize); break;
        case PKT_035E__0602:
            tmp = response_035e_0602(data, datalen, res, resMaxSize); break;
        /* ODBC
        case PKT_034A__XXXX:
            tmp = -1; break;
        case PKT_032B__08XX:
            tmp = response_032B_08XX(data, datalen, res, resMaxSize); break;
        case PKT_0347__0602:
            tmp = response_0347_0602(data, datalen, res, resMaxSize); break; */
        default:
            tmp = -1;
    }
    if (tmp < 0) unprintToSpace(data, datalen, res, resMaxSize);
    return TNS_NORMAL;
}

int responseParser(unsigned char *data, long datalen, char *res, int resMaxSize, int type) {
    int len;
    TNS_HEADER *tnsh = (TNS_HEADER *)data;
    memset(colInfos, 0, sizeof(COLUMN_INFO)*MAX_COLUMN);
    totalColInfoNum = 0;
    res[0] = '\0';
    if (datalen < 34) return 0;

    if (ntohs(tnsh->length) >= datalen) {
        return responseParserHelper(data+10, datalen-10, res, resMaxSize, type);
    }

    len = tnsRebuild(data, datalen, bufForTnsRebuid, sizeof(bufForTnsRebuid)-1);
    return responseParserHelper(bufForTnsRebuid, len, res, resMaxSize, type);
}

/*======================================================================================*/
/* oracle10g��oracle11g,
   Oracle sql developer 11g*/
int requestParserHelper(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos047fffffff, *pos;
    int sqlLen, tocopy;
    int offset, i, n;

    if (ORACLE_11G == serverVersion) offset = 15;
    else if (ORACLE_10G == serverVersion) offset = 10;
    else offset = 10;

    /* ��04 7F FF FF FF */
    pos047fffffff = memfind(data, "\x04\x7f\xff\xff\xff", datalen, 5);
    if(pos047fffffff) {
        pos = pos047fffffff + 5;
        pos ++;
        pos += ((unsigned char)*pos+1);
        pos += offset;
        if(pos >= (data+datalen)) {
            //printf("file length error  \n");
            return TNS_SAVE;
        }
    } else {
        if (0x03==(unsigned char)data[0] && 0x5e==(unsigned char)data[1]) {
            pos = data + 34;
            if(pos >= (data+datalen)) {
                //printf("%d:file length error\n", __LINE__);
                return TNS_DEL;
            }
        } else {
        /* 03 4A���������:
03 4A FD 01 00 00
00 3A 00 00 00 01 CF 00  00 00 00 00 00 00 00 01
01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 (������0xcf�ֽڵ�����)73 65 6C 65  63 74 ...*/
            pos = data + 11;
            n = *pos++;
            sqlLen = 0;
            for (i=0; i<n; i++) {
                sqlLen = 256*sqlLen + (unsigned char)pos[i];
            }
            pos += n;
            pos += 29;
            if (pos >= data+datalen) {
                //printf("%d:file length error\n", __LINE__);
                return TNS_DEL;
            }
            tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize;
            strncpy(res, pos, tocopy);
            res[tocopy] = '\0';
            if (strlen(res) != tocopy) {
                return TNS_DEL;
            }
            return TNS_NORMAL;
        }
    }

    /* �����ֶ� */
    if (0xfe == (unsigned char)*pos) {
        segmentsReform(pos+1, res, &resMaxSize);
    } else {
        sqlLen = *pos;
        if (pos + sqlLen > data+datalen) {
            //printf("file length error\n");
            return TNS_SAVE;
        }
        tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize;
        strncpy(res, pos+1, tocopy);
        res[tocopy] = '\0';
    }
    return TNS_NORMAL;
}

int homeAndqingyang64bit10g_plsql_requestParserHelper(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos047fffffff, *pos, *pos035e, *pos034a;
    int sqlLen, tocopy;
    int offset, i;

    /* �ȳ�ʼ��Ϊ�մ�, ��ֹû�ҵ�sql���ֱ�ӷ��� */
    res[0] = '\0';

    if (ORACLE_11G == serverVersion) offset = 15;
    else if (ORACLE_10G == serverVersion) offset = 10;
    else offset = 10;

    /* ��04 7F FF FF FF */
    pos047fffffff = memfind(data, "\x04\x7f\xff\xff\xff", datalen, 5);
    if(pos047fffffff) {
        pos = pos047fffffff + 5;
        pos ++;
        pos += ((unsigned char)*pos+1);
        pos += offset;
        if(pos >= (data+datalen)) {
            //printf("file length error  \n");
            return TNS_SAVE;
        }
        /* �����ֶ� */
        if (0xfe == (unsigned char)*pos) {
            segmentsReform(pos+1, res, &resMaxSize);
        } else {
            sqlLen = *pos;
            if (pos + sqlLen > data+datalen) {
                //printf("file length error\n");
                return TNS_SAVE;
            }
            tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize-1;
            strncpy(res, pos+1, tocopy);
            res[tocopy] = '\0';
        }
        return TNS_NORMAL;
    } else {
        pos035e = memfind(data, "\x03\x5e", datalen, 2);
        if (pos035e) {
            pos = pos035e + 12;
            sqlLen = 0;
            for (i=3; i>=0; i--) {
                sqlLen += (sqlLen * 256 + pos[i]);
            }
            pos += 4;
            pos += 29;
            tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize-1;
            strncpy(res, pos, tocopy);
            res[tocopy] = '\0';
            return TNS_NORMAL;
        } else {
        /* 03 4A���������:
03 4A FD 01 00 00
00 3A 00 00 00 01 CF 00  00 00 00 00 00 00 00 01
01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 (������0xcf�ֽڵ�����)73 65 6C 65  63 74 ...*/
            pos034a = memfind(data, "\x03\x4a", datalen, 2);
            if (pos034a) {
                pos = pos034a + 12;
                sqlLen = 0;
                for (i=3; i>=0; i--) {
                    sqlLen += (sqlLen * 256 + pos[i]);
                }
                pos += 4;
                pos += 26;
                tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize-1;
                strncpy(res, pos, tocopy);
                res[tocopy] = '\0';
                return TNS_NORMAL;
            } else {
                return TNS_SAVE;
            }
        }
    }
    return TNS_SAVE;
}



/*==============================================================================*/
int home64bit10g_plsql_request(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos, *pos035e;
    int sqlLen, tocopy;

    /* �ȳ�ʼ��Ϊ�մ�, ��ֹû�ҵ�035eֱ�ӷ��� */
    res[0] = '\0';

    pos035e = memfind(data, "\x03\x5e", datalen, 2);
    if (pos035e) {
        pos = pos035e + 56;
        if (0xfe == *pos) {
            segmentsReform(pos+1, res, &resMaxSize);
        } else {
            sqlLen = *pos;
            if (pos + sqlLen > data+datalen) {
                fprintf(stderr, "%d : sql length error\n", __LINE__);
                return TNS_SAVE;
            }
            tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize;
            memcpy(res, pos+1, tocopy);
            res[tocopy] = '\0';
        }
        return TNS_NORMAL;
    } else  {
        return TNS_SAVE;
    }
}

/* oracle10g��oracle11g,
   Oracle sql developer 11g*/
int home64bit10g_response_035e_0602(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    char *resPos=res, bitmap[MAX_COLUMN/8+1];
    int colNum, i, tmp, leftlen;

    res[0] = '\0';

    pos = data + 2;
    colNum = pos[0] + pos[1]*256;
    pos += 12;

    pos += (*pos+1);
    pos += (*pos+1);
    pos += (*pos+1);
    pos += (*pos+1);
    pos += 4;

    if (0 == totalColInfoNum) {
        getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
    }
    if (colNum != totalColInfoNum) {
        logMsg("(colNum(%d) != totalColInfoNum(%d))\n", colNum, totalColInfoNum);
        return TNS_SAVE;
    }

    memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
    int bb;
    for (bb=0; bb<(colNum/8+1); bb++) {
        bitmap[bb] = 0xff;
    }
    while (0x07 == *pos) {
        #if TNS_BINARY_DEBUG
        fprintf(stderr, "035e-0602--------------------------------------------\n");
        int idx;
        for (idx=0; idx<20; idx++) {
            fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
        }
        fprintf(stderr, "\n");
        #endif
        pos++;
        line_num++;
        for (i=0; i<totalColInfoNum; i++) {
            #if TNS_BINARY_DEBUG
            fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
            int idx;
            for (idx=0; idx<20; idx++) {
                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
            }
            fprintf(stderr, "\n");
            #endif
            if (i>=colNum) {
                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
                continue;
            }

            if (!isInBitmap(bitmap, i)) {
                if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                resPos += prevRow.colSizes[i];

                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
                continue;
            }

            prevRow.colValues[i] = resPos;
            if (0x00 == *pos) {
                strcpy(resPos, "null");
                resPos += 4;
                pos++;
            } else {
                leftlen = datalen - (pos - data);
                tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                pos += leftlen;
                resPos += tmp;
            }
            prevRow.colSizes[i] = resPos - prevRow.colValues[i];
            if (i != colNum-1) strcpy(resPos, ";:s");
            else strcpy(resPos, ";:n");
            resPos += 3;
        }

        /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ� */
        if (0x15 == *pos) {
            int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
            pos += 3;
            memcpy(bitmap, pos, bitmapSize);
            pos += bitmapSize;
        } else if (0x07 == *pos){
            for (bb=0; bb<(colNum/8+1); bb++) {
                bitmap[bb] = 0xff;
            }
        } else {
            break;
        }
    }
    if (datalen-(pos-data) > 0 && memfind(pos, "ORA-01403", datalen-(pos-data), 9)) {
        isLast = 1;
    }
    return TNS_NORMAL;
}


/* request = 03 5e, response = 10 17, sql��������, �ͻظ�����Ϣ
���� : 10g server, PL/sql developer(��)
�����06 02, ����Ž����������˳� */
int home64bit10g_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';

    pos = data + 29;

    /* ���� */
    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
    if (colNum > MAX_COLUMN) {
        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
        return TNS_SAVE;
    }
    totalColInfoNum = colNum;

    pos += 5;

    /* ������ */
    for (i=0; i<colNum && pos<data+datalen; i++) {
        #if TNS_BINARY_DEBUG
        int iii;
        fprintf(stderr, "%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
        }
        fprintf(stderr, "\n");
        #endif

        colInfos[i].type = *pos++; /* ������ */
        pos += 23;
        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
        pos += 10;

        /* ���� */
        memcpy(colInfos[i].colName, pos+1, pos[0]);
        colInfos[i].colName[pos[0]] = '\0';
        pos += (1 + pos[0]);

        pos += 10;
    }

    if (i < colNum) {
        return TNS_SAVE;
    }

    pos += 28;

    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
    }

    return TNS_NORMAL;
}

/* 0376���������username��Ӧ���� */
int home64bit10g_request_0376(unsigned char *data, long len, char *res, int resMaxSize) {
    unsigned char *pos;
    char username[512] = {0};
    char key[128] = {0};
    int usernameLen;

    if (memfind(data, "SQL Developer", len, 13))
        clientType = CLI_ORACLE_SQL_DEVELOPER;
    else if (memfind(data, "plsqldev.exe", len, 12))
        clientType = CLI_PL_SQL_DEVELOPER;
    else if (memfind(data, "sqlplus.exe", len, 11))
        clientType = CLI_SQLPLUS;
    else clientType = CLI_PL_SQL_DEVELOPER;

    if (CLI_PL_SQL_DEVELOPER == clientType || CLI_SQLPLUS == clientType) {
        pos = data+ 3;

        pos += ((0xfe == *pos) ? 4 : 1);
        pos += 8;
        pos += ((0xfe == *pos) ? 4 : 1);
        pos += 4;
        pos += ((0xfe == *pos) ? 4 : 1);
        pos += ((0xfe == *pos) ? 4 : 1);

        memcpy(username, pos+1, *pos);
        username[*pos] = '\0';
    } else if (CLI_ORACLE_SQL_DEVELOPER == clientType) {
        pos += 5;
        usernameLen = (unsigned char)*pos;
        pos += 8;
        memcpy(username, pos, usernameLen);
        username[usernameLen] = '\0';
    }

    redisContext *conn = NULL;
    redisReply *reply =NULL;

    conn = redisConnect(REDISSERVERHOST, REDISSERVERPORT);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        redisFree(conn);
        conn=NULL;
        return -1;
    }

    reply =(redisReply*)redisCommand(conn,"select %d", REDIS_ORACLE_TABLE);
    if(conn->err) {
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);//free reply

    getUsernameKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %s EX 86400", key, username);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);

    getClientKey(src_ip, src_port, dst_ip, dst_port, key);
    reply = (redisReply*)redisCommand(conn, "set %s %d EX 86400", key, clientType);
    if(conn->err){
        fprintf(stderr,"error %d:%s\n",conn->err, conn->errstr);
        if(reply){
            freeReplyObject(reply);
            return 0;
        }
        redisFree(conn);
    }
    freeReplyObject(reply);

    redisFree(conn);
    reply = NULL;

    logMsg("home64bit10g_request_0376: username=%s, key=%s\n", username, key);

    return 0;
}

/*==============================================================================*/
int home64bit11gr2_request(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos, *pos035e;
    int sqlLen, tocopy;

    /* �ȳ�ʼ��Ϊ�մ�, ��ֹû�ҵ�035eֱ�ӷ��� */
    res[0] = '\0';

    pos035e = memfind(data, "\x03\x5e", datalen, 2);
    if (pos035e) {
        if (CLI_PL_SQL_DEVELOPER==clientType) {
            pos = pos035e + 0x38;
        } else if (CLI_SQLPLUS==clientType) {
            pos = pos035e + 0x46;
        } else if (CLI_ORACLE_SQL_DEVELOPER==clientType) {
            pos = pos035e + 8;
            pos += (pos[0] + 1);
            pos += 5;
            pos += (pos[0] + 1);
            pos += (pos[0] + 1);
            pos += (pos[0] + 1);
            pos += (pos[0] + 1);
            pos += 16;
        } else {
            pos = pos035e + 0x38;
        }
        if (0xfe == *pos) {
            segmentsReform(pos+1, res, &resMaxSize);
        } else {
            sqlLen = *pos;
            if (pos + sqlLen > data+datalen) {
                fprintf(stderr, "%d : sql length error\n", __LINE__);
                return TNS_SAVE;
            }
            tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize;
            memcpy(res, pos+1, tocopy);
            res[tocopy] = '\0';
        }
        return TNS_NORMAL;
    } else  {
        return TNS_SAVE;
    }
}

/* request = 03 5e, response = 10 17, sql��������, �ͻظ�����Ϣ
���� : 10g server, PL/sql developer(��)
�����06 02, ����Ž����������˳� */
int home64bit11gr2_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';
    if (CLI_PL_SQL_DEVELOPER==clientType) {
        pos = data + 29;
	    /* ���� */
	    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 5;

	    /* ������ */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* ������ */
	        pos += 23;
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
	        pos += 10;

	        /* ���� */
	        memcpy(colInfos[i].colName, pos+1, pos[0]);
	        colInfos[i].colName[pos[0]] = '\0';
	        pos += (1 + pos[0]);

	        pos += 10;
	    }

	    if (i < colNum) {
	        return TNS_SAVE;
	    }

	    pos += 28;

	    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
	        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
	    }
    } else if (CLI_SQLPLUS==clientType) {
        pos = data + 32;
	    /* ���� */
	    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 5;

	    /* ������ */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* ������ */
	        pos += 23;
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
	        pos += 10;

	        /* ���� */
	        memcpy(colInfos[i].colName, pos+1, pos[0]);
	        colInfos[i].colName[pos[0]] = '\0';
	        pos += (1 + pos[0]);

	        pos += 14;
	    }

	    if (i < colNum) {
	        return TNS_SAVE;
	    }

	    pos += 32;

	    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1]  || 0x1a==pos[1])) {
	        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
	    }
    } else if (CLI_ORACLE_SQL_DEVELOPER==clientType) {
        pos = data + 25;
        pos += (pos[0] + 1);

	    /* ���� */
        colNum = 0;
        for (i=0; i<pos[0]; i++) {
            colNum = colNum * 256 + pos[i+1];
        }
        pos += (pos[0] + 1);
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 1;

	    /* ������ */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* ������ */
	        pos += 2;
            if (pos[0] > 0x01) pos += 2;
            else pos += (1 + pos[0]);

	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
	        pos += (1 + pos[0]);
	        pos += 4;

	        /* ���� */
	        memcpy(colInfos[i].colName, pos+1, pos[0]);
	        colInfos[i].colName[pos[0]] = '\0';
	        pos += (1 + pos[0]);

	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	    }

	    if (i < colNum) {
	        return TNS_SAVE;
	    }

        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);

	    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
	        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
	    }
    }

    return TNS_NORMAL;
}


/* oracle10g��oracle11g,
   Oracle sql developer 11g*/
int home64bit11gr2_response_035e_0602(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    char *resPos=res, bitmap[MAX_COLUMN/8+1];
    int colNum, i, tmp, leftlen;

    res[0] = '\0';

    if (CLI_PL_SQL_DEVELOPER==clientType) {
        pos = data + 2;
        colNum = pos[0] + pos[1]*256;
        pos += 12;

        pos += (*pos+1);
        pos += (*pos+1);
        pos += (*pos+1);
        pos += (*pos+1);
        pos += 4;

        if (0 == totalColInfoNum) {
            getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
        }
        if (colNum != totalColInfoNum) {
            logMsg("(colNum(%d) != totalColInfoNum(%d))\n", colNum, totalColInfoNum);
            return TNS_SAVE;
        }
        memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
        int bb;
        for (bb=0; bb<(colNum/8+1); bb++) {
            bitmap[bb] = 0xff;
        }

        while (0x07 == *pos) {
            #if TNS_BINARY_DEBUG
            fprintf(stderr, "035e-0602--------------------------------------------\n");
            int idx;
            for (idx=0; idx<20; idx++) {
                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
            }
            fprintf(stderr, "\n");
            #endif
            pos++;
            line_num++;
            for (i=0; i<totalColInfoNum; i++) {
                #if TNS_BINARY_DEBUG
		        fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
                int idx;
                for (idx=0; idx<20; idx++) {
                    fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
                }
                fprintf(stderr, "\n");
                #endif
                if (i>=colNum) {
                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                if (!isInBitmap(bitmap, i)) {
                    if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                    resPos += prevRow.colSizes[i];

                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                prevRow.colValues[i] = resPos;
                if (0x00 == *pos) {
                    pos++;/* 0x00��ʾ���ֶΣ�������б�ʾΪ�մ� */
                } else {
                    leftlen = datalen - (pos - data);
                    tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                    pos += leftlen;
                    resPos += tmp;
                }
                prevRow.colSizes[i] = resPos - prevRow.colValues[i];
                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
            }

            /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ� */
            if (0x15 == *pos) {
                int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
                pos += 3;
                memcpy(bitmap, pos, bitmapSize);
                pos += bitmapSize;
            } else if (0x07 == *pos){
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            } else {
                break;
            }
        }
    } else if (CLI_SQLPLUS==clientType) {
        pos = data + 2;
        colNum = pos[0] + pos[1]*256;
        pos += 12;

        pos += (*pos+1);
        pos += (*pos+1);
        pos += (*pos+1);
        pos += (*pos+1);
        pos += 4;

        if (0 == totalColInfoNum) {
            getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
        }
        if (colNum != totalColInfoNum) {
            logMsg("(colNum(%d) != totalColInfoNum(%d))\n", colNum, totalColInfoNum);
            return TNS_SAVE;
        }
        memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
        int bb;
        for (bb=0; bb<(colNum/8+1); bb++) {
            bitmap[bb] = 0xff;
        }
        while (0x07==*pos) {
            #if TNS_BINARY_DEBUG
            fprintf(stderr, "035e-0602--------------------------------------------\n");
            int idx;
            for (idx=0; idx<20; idx++) {
                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
            }
            fprintf(stderr, "\n");
            #endif
            pos += 4;
            line_num++;
            int currentCols = *pos++;
            for (i=0; i<currentCols; i++) {
                #if TNS_BINARY_DEBUG
                fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
                int idx;
                for (idx=0; idx<20; idx++) {
                    fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
                }
                fprintf(stderr, "\n");
                #endif
                if (i>=colNum) {
                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                if (!isInBitmap(bitmap, i)) {
                    if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                    resPos += prevRow.colSizes[i];

                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                prevRow.colValues[i] = resPos;
                if (0xff == *pos) {
                    pos++; /* 0xff��ʾ���ֶΣ�������б�ʾΪ�մ� */
                } else {
                    leftlen = datalen - (pos - data);
                    tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                    pos += leftlen;
                    resPos += tmp;
                }
                prevRow.colSizes[i] = resPos - prevRow.colValues[i];
                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
            }
            if (currentCols < colNum) {
                for (i=0; i<(colNum-currentCols-1); i++) {
                    strcpy(resPos, ";:s");
                    resPos+=3;
                }
                strcpy(resPos, ";:n");
                resPos+=3;
            }

            /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ�
            if (0x07 == *pos) {
                int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
                pos += 3;
                memcpy(bitmap, pos, bitmapSize);
                pos += bitmapSize;
            } else if (0x0d == *pos){
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            } else {
                break;
            } */
            if (0x07 == *pos) {
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            }
        }
    } else if (CLI_ORACLE_SQL_DEVELOPER==clientType) {
        pos = data + 2;
        colNum = 0;
        for (i=0; i<pos[0]; i++) {
            colNum = colNum * 256 + pos[i+1];
        }
        pos += (pos[0] + 1);
        pos += 6;

        memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
        int bb;
        for (bb=0; bb<(colNum/8+1); bb++) {
            bitmap[bb] = 0xff;
        }
        while (0x07 == *pos) {
            #if TNS_BINARY_DEBUG
            fprintf(stderr, "035e-0602--------------------------------------------\n");
            int idx;
            for (idx=0; idx<20; idx++) {
                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
            }
            fprintf(stderr, "\n");
            #endif
            pos++;
            line_num++;
            for (i=0; i<totalColInfoNum; i++) {
                #if TNS_BINARY_DEBUG
		        fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
                int idx;
                for (idx=0; idx<20; idx++) {
                    fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
                }
                fprintf(stderr, "\n");
                #endif
                if (i>=colNum) {
                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                if (!isInBitmap(bitmap, i)) {
                    if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                    resPos += prevRow.colSizes[i];

                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                prevRow.colValues[i] = resPos;
                if (0x00 == *pos) {
                    pos++;/* 0x00��ʾ���ֶΣ�������б�ʾΪ�մ� */
                } else {
                    leftlen = datalen - (pos - data);
                    tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                    pos += leftlen;
                    resPos += tmp;
                }
                prevRow.colSizes[i] = resPos - prevRow.colValues[i];
                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
            }

            /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ� */
            if (0x15 == *pos) {
                int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
                pos += 3;
                memcpy(bitmap, pos, bitmapSize);
                pos += bitmapSize;
            } else if (0x07 == *pos){
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            } else {
                break;
            }
        }
    }

    if (datalen-(pos-data) > 0 && memfind(pos, "ORA-01403", datalen-(pos-data), 9)) {
        isLast = 1;
    }
    return TNS_NORMAL;
}

/* 0376���������username��Ӧ���� */
int home64bit11gr2_request_0376(unsigned char *data, long len, char *res, int resMaxSize) {
    unsigned char *pos;
    char username[512] = {0};
    char key[128] = {0};
    int usernameLen;

    if (memfind(data, "SQL Developer", len, 13))
        clientType = CLI_ORACLE_SQL_DEVELOPER;
    else if (memfind(data, "plsqldev.exe", len, 12))
        clientType = CLI_PL_SQL_DEVELOPER;
    else if (memfind(data, "sqlplus.exe", len, 11))
        clientType = CLI_SQLPLUS;
    else clientType = CLI_PL_SQL_DEVELOPER;

    if (CLI_PL_SQL_DEVELOPER == clientType || CLI_SQLPLUS == clientType) {
        pos = data+ 19;
        memcpy(username, pos+1, *pos);
        username[*pos] = '\0';
    } else if (CLI_ORACLE_SQL_DEVELOPER == clientType) {
        pos = data+ 13;
        memcpy(username, pos+1, *pos);
        username[*pos] = '\0';
    }

    saveUsernameAndClientType(username, clientType);

    return 0;
}

/*==============================================================================*/
int home32bit11gr2_request(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos, *pos035e;
    int sqlLen, tocopy;

    /* �ȳ�ʼ��Ϊ�մ�, ��ֹû�ҵ�035eֱ�ӷ��� */
    res[0] = '\0';

    pos035e = memfind(data, "\x03\x5e", datalen, 2);
    if (pos035e) {
        if (CLI_PL_SQL_DEVELOPER==clientType) {
            pos = pos035e + 95;
        } else if (CLI_SQLPLUS==clientType) {
            pos = pos035e + 0x73;
        } else if (CLI_ORACLE_SQL_DEVELOPER==clientType) {
            pos = pos035e + 8;
            pos += (pos[0] + 1);
            pos += 5;
            pos += (pos[0] + 1);
            pos += (pos[0] + 1);
            pos += (pos[0] + 1);
            pos += (pos[0] + 1);
            pos += 16;
        } else {
            pos = pos035e + 95;
        }

        if (0xfe == *pos) {
            segmentsReform(pos+1, res, &resMaxSize);
        } else {
            sqlLen = *pos;
            if (pos + sqlLen > data+datalen) {
                fprintf(stderr, "%d : sql length error\n", __LINE__);
                return TNS_SAVE;
            }
            tocopy = sqlLen<resMaxSize ? sqlLen : resMaxSize;
            memcpy(res, pos+1, tocopy);
            res[tocopy] = '\0';
        }
        return TNS_NORMAL;
    } else  {
        return TNS_SAVE;
    }
}

int home32bit11gr2_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';

    if (CLI_PL_SQL_DEVELOPER==clientType) {
        pos = data + 29;

        /* ���� */
        colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
        if (colNum > MAX_COLUMN) {
            fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
            return TNS_SAVE;
        }
        totalColInfoNum = colNum;

        pos += 5;

        /* ������ */
        for (i=0; i<colNum && pos<data+datalen; i++) {
            #if TNS_BINARY_DEBUG
            int iii;
            fprintf(stderr, "%d: ------------------------------------\n", i);
            for (iii=0; iii<50; iii++) {
                fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
            }
            fprintf(stderr, "\n");
            #endif

            colInfos[i].type = pos[1]; /* ������ */
            pos += 27;
            colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
            pos += 11;

            /* ���� */
            memcpy(colInfos[i].colName, pos+1, pos[0]);
            colInfos[i].colName[pos[0]] = '\0';
            pos += (1 + pos[0]);

            pos += 10;
        }

        if (i < colNum) {
            return TNS_SAVE;
        }

        pos += 28;

        if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
            return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
        }
    } else if (CLI_SQLPLUS==clientType) {
        pos = data + 32;
	    /* ���� */
	    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 5;

	    /* ������ */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
            #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
            #endif

	        colInfos[i].type = pos[1]; /* ������ */
	        pos += 27;
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
	        pos += 11;

	        /* ���� */
	        memcpy(colInfos[i].colName, pos+1, pos[0]);
	        colInfos[i].colName[pos[0]] = '\0';
	        pos += (1 + pos[0]);

	        pos += 14;
	    }

	    if (i < colNum) {
	        return TNS_SAVE;
	    }

	    pos += 32;

	    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1]  || 0x1a==pos[1])) {
	        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
	    }

    }else if (CLI_ORACLE_SQL_DEVELOPER==clientType) {
        pos = data + 2 + 16 + 7;
        pos += (pos[0] + 1);

	    /* ���� */
        colNum = 0;
        for (i=0; i<pos[0]; i++) {
            colNum = colNum * 256 + pos[i+1];
        }
        pos += (pos[0] + 1);
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 1;

	    /* ������ */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* ������ */
	        pos += 2;
            if (pos[0] > 0x01) pos += 2;
            else pos += (1 + pos[0]);

	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR���ַ���(1/2) */
	        pos += (1 + pos[0]);
	        pos += 4;

	        /* ���� */
	        memcpy(colInfos[i].colName, pos+1, pos[0]);
	        colInfos[i].colName[pos[0]] = '\0';
	        pos += (1 + pos[0]);

	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	    }

	    if (i < colNum) {
	        return TNS_SAVE;
	    }

        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);
        pos += (1 + pos[0]);

	    if (0x06==pos[0] && (0x02==pos[1] || 0x01==pos[1] || 0x22==pos[1] || 0x1a==pos[1])) {
	        return response_035e_0602(pos, datalen-(pos-data), res, resMaxSize);
	    }
    } else {
        fprintf(stderr, "home32bit11gr2_response_035e_1017: unknown client\n");
        return -1;
    }

    return TNS_NORMAL;
}

/* oracle10g��oracle11g,
   Oracle sql developer 11g*/
int home32bit11gr2_response_035e_0602(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    char *resPos=res, bitmap[MAX_COLUMN/8+1];
    int colNum, i, tmp, leftlen;

    res[0] = '\0';
    if (CLI_PL_SQL_DEVELOPER==clientType) {
        pos = data + 4;
        colNum = pos[0] + pos[1]*256;
        pos += 30;

	    if (0 == totalColInfoNum) {
	        getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
	    }
	    if (colNum != totalColInfoNum) {
	        logMsg("(colNum(%d) != totalColInfoNum(%d))\n", colNum, totalColInfoNum);
	        return TNS_SAVE;
	    }

	    memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
	    int bb;
	    for (bb=0; bb<(colNum/8+1); bb++) {
	        bitmap[bb] = 0xff;
	    }
	    while (0x07 == *pos) {
	        #if TNS_BINARY_DEBUG
	        fprintf(stderr, "035e-0602--------------------------------------------\n");
	        int idx;
	        for (idx=0; idx<20; idx++) {
	            fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
	        }
	        fprintf(stderr, "\n");
	        #endif
	        pos++;
	        line_num++;
	        for (i=0; i<colNum; i++) {
	            #if TNS_BINARY_DEBUG
	            fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
	            int idx;
	            for (idx=0; idx<20; idx++) {
	                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
	            }
	            fprintf(stderr, "\n");
	            #endif
	            if (i>=colNum) {
	                if (i != colNum-1) strcpy(resPos, ";:s");
	                else strcpy(resPos, ";:n");
	                resPos += 3;
	                continue;
	            }

	            if (!isInBitmap(bitmap, i)) {
	                if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
	                resPos += prevRow.colSizes[i];

	                if (i != colNum-1) strcpy(resPos, ";:s");
	                else strcpy(resPos, ";:n");
	                resPos += 3;
	                continue;
	            }

	            prevRow.colValues[i] = resPos;
	            if (0x00 == *pos) {
	                strcpy(resPos, "null");
	                resPos += 4;
	                pos++;
	            } else {
	                leftlen = datalen - (pos - data);
	                tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
	                pos += leftlen;
	                resPos += tmp;
	            }
	            prevRow.colSizes[i] = resPos - prevRow.colValues[i];
	            if (i != colNum-1) strcpy(resPos, ";:s");
	            else strcpy(resPos, ";:n");
	            resPos += 3;
	        }

	        /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ� */
	        if (0x15 == *pos) {
	            int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
	            pos += 3;
	            memcpy(bitmap, pos, bitmapSize);
	            pos += bitmapSize;
	        } else if (0x07 == *pos){
	            for (bb=0; bb<(colNum/8+1); bb++) {
	                bitmap[bb] = 0xff;
	            }
	        } else {
	            break;
	        }
	    }
    } else if (CLI_SQLPLUS==clientType) {
        pos = data + 4;
        colNum = pos[0] + pos[1]*256;
        pos += 30;

        if (0 == totalColInfoNum) {
            getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
        }
        if (colNum != totalColInfoNum) {
            logMsg("(colNum(%d) != totalColInfoNum(%d))\n", colNum, totalColInfoNum);
            return TNS_SAVE;
        }
        memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
        int bb;
        for (bb=0; bb<(colNum/8+1); bb++) {
            bitmap[bb] = 0xff;
        }
        while (0x07==*pos) {
            #if TNS_BINARY_DEBUG
            fprintf(stderr, "035e-0602--------------------------------------------\n");
            int idx;
            for (idx=0; idx<20; idx++) {
                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
            }
            fprintf(stderr, "\n");
            #endif

            pos += 4;
            line_num++;
            int currentCols = *pos++;
            for (i=0; i<currentCols; i++) {
                #if TNS_BINARY_DEBUG
                fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
                int idx;
                for (idx=0; idx<20; idx++) {
                    fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
                }
                fprintf(stderr, "\n");
                #endif

                if (i>=colNum) {
                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                if (!isInBitmap(bitmap, i)) {
                    if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                    resPos += prevRow.colSizes[i];

                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                prevRow.colValues[i] = resPos;
                if (0xff == *pos) {
                    pos++; /* 0xff��ʾ���ֶΣ�������б�ʾΪ�մ� */
                } else {
                    leftlen = datalen - (pos - data);
                    tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                    pos += leftlen;
                    resPos += tmp;
                }
                prevRow.colSizes[i] = resPos - prevRow.colValues[i];
                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
            }
            if (currentCols < colNum) {
                for (i=0; i<(colNum-currentCols-1); i++) {
                    strcpy(resPos, ";:s");
                    resPos+=3;
                }
                strcpy(resPos, ";:n");
                resPos+=3;
            }

            /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ�
            if (0x07 == *pos) {
                int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
                pos += 3;
                memcpy(bitmap, pos, bitmapSize);
                pos += bitmapSize;
            } else if (0x0d == *pos){
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            } else {
                break;
            } */
            if (0x07 == *pos) {
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            }
        }
    } else if (CLI_ORACLE_SQL_DEVELOPER==clientType) {
        pos = data + 2;
        colNum = 0;
        for (i=0; i<pos[0]; i++) {
            colNum = colNum * 256 + pos[i+1];
        }
        pos += (pos[0] + 1);
        pos += 6;

        if (0 == totalColInfoNum) {
            getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
        }
        if (colNum != totalColInfoNum) {
            logMsg("(colNum(%d) != totalColInfoNum(%d))\n", colNum, totalColInfoNum);
            return TNS_SAVE;
        }

        memset(&prevRow, 0, sizeof(PREV_ROW_INFO));
        int bb;
        for (bb=0; bb<(colNum/8+1); bb++) {
            bitmap[bb] = 0xff;
        }
        while (0x07 == *pos) {
            #if TNS_BINARY_DEBUG
            fprintf(stderr, "035e-0602--------------------------------------------\n");
            int idx;
            for (idx=0; idx<20; idx++) {
                fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
            }
            fprintf(stderr, "\n");
            #endif
            pos++;
            line_num++;
            for (i=0; i<colNum; i++) {
                #if TNS_BINARY_DEBUG
		        fprintf(stderr, "035e-0602-single type %d--------------------------------------------\n", i+1);
                int idx;
                for (idx=0; idx<20; idx++) {
                    fprintf(stderr, "%02x ", (unsigned char)pos[idx]);
                }
                fprintf(stderr, "\n");
                #endif

                if (!isInBitmap(bitmap, i)) {
                    if (prevRow.colSizes[i]!=0) memcpy(resPos, prevRow.colValues[i], prevRow.colSizes[i]);
                    resPos += prevRow.colSizes[i];

                    if (i != colNum-1) strcpy(resPos, ";:s");
                    else strcpy(resPos, ";:n");
                    resPos += 3;
                    continue;
                }

                prevRow.colValues[i] = resPos;
                if (0x00 == *pos) {
                    pos++;/* 0x00��ʾ���ֶΣ�������б�ʾΪ�մ� */
                } else {
                    leftlen = datalen - (pos - data);
                    tmp = parseColValue(pos, &leftlen, colInfos+i, resPos, resMaxSize-(resPos-res));
                    pos += leftlen;
                    resPos += strlen(resPos);
                }
                prevRow.colSizes[i] = resPos - prevRow.colValues[i];
                if (i != colNum-1) strcpy(resPos, ";:s");
                else strcpy(resPos, ";:n");
                resPos += 3;
            }

            /* 0x15�ֶΣ�����һ�е����ݺͱ��ν����У�������ֵͬ����, ��������һ���о�ȱʧ�ˡ� */
            if (0x15 == *pos) {
                int bitmapSize = colNum%8==0 ? colNum/8 : colNum/8+1;
                pos += 3;
                memcpy(bitmap, pos, bitmapSize);
                pos += bitmapSize;
            } else if (0x07 == *pos){
                for (bb=0; bb<(colNum/8+1); bb++) {
                    bitmap[bb] = 0xff;
                }
            } else {
                break;
            }
        }
    } else {
        fprintf(stderr, "home32bit11gr2_response_035e_0602: unknown client\n");
        return -1;
    }


    if (datalen-(pos-data) > 0 && memfind(pos, "ORA-01403", datalen-(pos-data), 9)) {
        isLast = 1;
    }
    return TNS_NORMAL;
}

/* 0376���������username��Ӧ���� */
int home32bit11gr2_request_0376(unsigned char *data, long len, char *res, int resMaxSize) {
    unsigned char *pos;
    char username[512] = {0};
    char key[128] = {0};

    if (memfind(data, "SQL Developer", len, 13))
        clientType = CLI_ORACLE_SQL_DEVELOPER;
    else if (memfind(data, "plsqldev.exe", len, 12))
        clientType = CLI_PL_SQL_DEVELOPER;
    else if (memfind(data, "sqlplus.exe", len, 11))
        clientType = CLI_SQLPLUS;
    else clientType = CLI_PL_SQL_DEVELOPER;

    if (CLI_PL_SQL_DEVELOPER == clientType || CLI_SQLPLUS == clientType) {
        pos = data+ 31;
        memcpy(username, pos+1, *pos);
        username[*pos] = '\0';
    } else if (CLI_ORACLE_SQL_DEVELOPER == clientType) {
        pos = data+ 13;
        memcpy(username, pos+1, *pos);
        username[*pos] = '\0';
    }
    saveUsernameAndClientType(username, clientType);

    return 0;
}


int requestParser(unsigned char *data, long datalen, char *res, int resMaxSize) {
    int len;
    TNS_HEADER *tnsh = (TNS_HEADER *)data;

    if (ntohs(tnsh->length) >= datalen) {
        return request_035e(data+10, datalen-10, res, resMaxSize);
    }

    len = tnsRebuild(data, datalen, bufForTnsRebuid, sizeof(bufForTnsRebuid)-1);
    return request_035e(bufForTnsRebuid, len, res, resMaxSize);
}

/*======================================================================================*/
/*
void sigfun(int sig){
    switch(sig){
        case SIGINT:
        case SIGTERM:
        case SIGSEGV:
        case SIGBUS:
            exit(0);
            break;
        default:
            exit(0);
            break;
    }
}
*/
/* ����'sqlStatement'��ĵ�һ�γ��ֵģ���������ķֺš� */
char *findNextSemicolon(char *sqlStatement) {
    char *pos;
    int singleQuoteFlag, doubleQuoteFlag;

    pos = sqlStatement;
    singleQuoteFlag = doubleQuoteFlag = 0;
    while (*pos != '\0') {
        if (';'==*pos && 0==singleQuoteFlag && 0==doubleQuoteFlag) {
            return pos;
        }

        /* ˫������ĵ����� */
        if ('\'' == *pos && 0==doubleQuoteFlag) {
            singleQuoteFlag = (1 == singleQuoteFlag) ? 0 : 1;
            pos ++; continue;
        }

        /* ���������˫���� */
        if ('"' == *pos && 0==singleQuoteFlag) {
            doubleQuoteFlag = (1 == doubleQuoteFlag) ? 0 : 1;
            pos ++; continue;
        }

        /* �����ڵ����� */
        if ('\\' == *pos && (1==doubleQuoteFlag || 1==singleQuoteFlag)) {
            if ('\'' == *(pos+1) || '"'==*(pos+1)) {
                pos += 2;
            } else {
                pos ++;
            }
            continue;
        }
        pos ++;
    }
    return NULL;
}

/* <1>���sql��ϣ�ÿ����÷ֺŷָ�
   <2>sql�����������(", ''"), ����һ��ʱҪ����
   <3>��ͷ��sql���䣬��β��ע�ͣ�Ҫ����ע��*/
int getHighestLevelSqlAction(char *sqlStatement) {
    char *start, *pos, actionWord[64];
    int tocopy, sqlLen, returnFlag;
    int sqlLevel = 0;

    sqlLen = strlen(sqlStatement);
    pos = sqlStatement;
    returnFlag = 19;
    while (pos < sqlStatement + sqlLen) {
        /* �ҵ�1�γ��ֵ���ĸ. �ù�����Ҫ����ע�� */
        while (!isalpha((int)*pos) && (pos < (sqlStatement+sqlLen))) {
            /* ����ע��. sqlserverע����2��/ * * /��ע�ͣ��Լ�--��ע�� */
            if ('/'==pos[0] && '*'==pos[1]) {
                pos = strstr(pos, "*/");
                if (!pos) return returnFlag;
                pos += 2;
            } else if ('-'==pos[0] && '-'==pos[1]) {
                pos = strchr(pos, '\n');
                if (!pos) return returnFlag;
                pos++;
            } else {
                pos++;
            }
        }
        start = pos;

        //�Ҹ���ĸ�����1�γ��ֵĿհ��ַ�, ��sql���ָ��(�ֺ�;)
        pos = strpbrk(start, " \r\n\t\f;");
        if (NULL == pos) {
            tocopy = (strlen(start) > (sizeof(actionWord)-1)) ? (sizeof(actionWord)-1) : strlen(start);
        } else {
            tocopy = ((pos-start) > (sizeof(actionWord)-1)) ? (sizeof(actionWord)-1) : (pos-start);
        }

        memcpy(actionWord, start, tocopy);
        actionWord[tocopy] = '\0';

        /* delete, insert, update, select ������4,3,2,1��������0 */
        if (strncasecmp(actionWord, "delete", 6) == 0) return 8;
        if (strncasecmp(actionWord, "insert", 6) == 0) {
            if (sqlLevel < 3) sqlLevel = 3;
        } else if (strncasecmp(actionWord, "update", 6) == 0) {
            if (sqlLevel < 2) sqlLevel = 2;
        } else if (strncasecmp(actionWord, "select", 6) == 0){
            if (sqlLevel < 1) sqlLevel = 1;
        }

        pos = findNextSemicolon(start);
        if (NULL == pos) break;
        pos++; /* �����ֺ�(;),������һ��sql */
    }

    switch (sqlLevel) {
        case 0: return 20;
        case 1: return 6;
        case 2: return 7;
        case 3: return 9;
        case 4: return 8;
    }
    return 20; /* ����������� */
}

#define GET_TYPE(request, response, type)                                   \
do {                                                                        \
    if (0x11 == request[10]                                                  \
        || (0x03==request[10]  && 0x5e==request[11])                          \
        || (0x03==request[10]  && 0x05==request[11]))                         \
    {                                                                       \
        type = PKT_035E__XXXX;                                              \
        if (0x10==response[10] && 0x17==response[11]) type = PKT_035E__1017;  \
        if (0x06==response[10] && (0x02==response[11] || 0x01==response[11] || 0x22==response[11] || 0x1a==response[11])) type = PKT_035E__0602;  \
        if (0x04==response[10]) type = PKT_035E__04XX;                       \
    } else if (0x03==request[10] && 0x76==request[11]) {                      \
        type = PKT_0376;                                                    \
    } else {                                                                \
        /* ODBC                                                                 \
        if (0x03==request[10]  && 0x4a==request[11]) {                            \
            type = PKT_034A__XXXX;                                              \
        }                                                                       \
        if (0x03==request[10]  && 0x2b==request[11] && 0x08==response[10]) {       \
            type = PKT_032B__08XX;                                              \
        }                                                                       \
        if (0x03==request[10]  && 0x47==request[11]) {                            \
            if (0x06==response[10] && 0x02==response[11]) type = PKT_0347__0602;  \
            if (0x04==response[10]) type = PKT_0347__04XX;                       \
        }     */                                                                \
        type = PKT_OTHER;                                                       \
    }                                                                           \
}while(0)

/* read()�ķ�װ���ɹ����ض�ȡ���ֽ�����ʧ�ܷ���-1 */
ssize_t readBytes(int fd, void *buf, size_t n) {
    char *pos = buf;
    ssize_t nrd, nLeft=n;

    while (nLeft > 0) {
        if ( (nrd = read(fd, pos, nLeft)) < 0) {
            if (EINTR == errno) {
                nrd = 0;  /* �ٴε���read */
            } else {
                fprintf(stderr, "read() failed");
                return -1;
            }
        } else if (0 == nrd) {
            break; /* EOF */
        }
        nLeft -= nrd;
        pos += nrd;
    }
    return (n-nLeft);
}

/* ���ļ�'fileName'��'len'�ֽڵ�'buf'.
   ���سɹ������ֽ���, ʧ�ܷ���-1 */
ssize_t readFile(char *fileName, void *buf, size_t n) {
    char *pos = buf;
    int fd, nrd;

    fd = open(fileName, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "Fails to open the file(%s)", fileName); return -1; }
    if ((nrd = readBytes(fd, pos, n)) < 0) { fprintf(stderr, "Fails to read the file(%s)", fileName); return -1; }
    close(fd);
    return nrd;
}

/* ��1��select������ʾ�»غϿ�ʼ, ��hash��������½��, ���ѽ������ϻ���. ��hash�����Ѵ��ڸ���Ԫ��, �������
��1������Ϣ��, ��hash�����ҵ���Ӧ, ������Ϣд��redis
��1����ֵ��, ��hash�����ҵ���Ӧ, ����ֵд��redis

hashֵ, ip, port, mac, capture_time, interval_time, line_num�浽�����ڴ�
sql��䱣����src_ip-src_port-dst_ip-dst_port-sql��
��ֵ������src_ip-src_port-dst_ip-dst_port-rows��
�и�ʽ������src_ip-src_port-dst_ip-dst_port-cloInfos�� */
int mainoracle(char *filePath, ORACLE_SHM_MEM *sharedm){
    unsigned long capture_time;
    int requestLen, responseLen, len, dir_id, app_id, version, seqid, tmpret, i;
    unsigned int hash;
    int app_type;
    char *resPos, src_mac[24], dst_mac[24];
    char requestFile[512], responseFile[512];
    int pktType;

    serverVersion = ORACLE_10G;
    isLast = 0;
    line_num = 0;
    totalColInfoNum = 0;
    clientType = CLI_ORACLE_SQL_DEVELOPER;

    /* ��request�ļ��������response�ļ��� */
    strcpy(requestFile, filePath);
    len = strlen(requestFile);
    memcpy(responseFile, requestFile, len-7);
    strcpy(responseFile+len-7, "response");
    responseFile[len+1] = '\0';

    /* ���ļ�, request��response�����ڲŽ��� */
    requestLen = readFile(requestFile, requestBuf, sizeof(requestBuf)-1);
    if (requestLen < 0) {
        deleteFile(requestFile, responseFile);
        logMsg( "read request failed\n");
        return -1;
    }
    if (requestBuf[4] != PKT_TYPE_DATA) {
        deleteFile(requestFile,responseFile);
        logMsg( "not 06 packet\n");
        return 0;
    }

    responseLen = readFile(responseFile, responseBuf, sizeof(responseBuf)-1);
    if (requestLen < 0) {
        deleteFile(requestFile, responseFile);
        logMsg( "read response failed\n");
        return -1;
    }

    /* ����ִ��ʱ�� */
    long requestCaptureTime = getFileTime(requestFile);
    long responseCaptureTime = getFileTime(responseFile);
    long tmpdif = requestCaptureTime - responseCaptureTime;
    unsigned long interval_time = (tmpdif < 0) ? (-tmpdif) : tmpdif;

    /* ɾ���ļ�: ������ڶ��ļ�֮�󣬽���֮ǰ����ֹ�δ�����ɾ������, �������³���ס */
    deleteFile(requestFile, responseFile);

    logMsg("\n\n=========================================================\n"
           "request = %02x %02x, response = %02x %02x\n",
           requestBuf[10], requestBuf[11], responseBuf[10], responseBuf[11]);

    tmpret=sscanf(requestFile,
                  "/dev/shm/oracle/%d/%lu_%u_%d_%u_%12[^_]_%u_%u_%12[^_]_%u_%d_%d_request",
                  &dir_id, &capture_time, &hash, &app_type,
                  &src_ip, src_mac, &src_port, &dst_ip, dst_mac, &dst_port, &version, &seqid);

    if (sharedm->count > 0) checkOldList(sharedm);

    /* version=30_..��10g�汾�� 31_...��11g�汾 */
    switch(version) {
        case 30: serverVersion = ORACLE_10G; break;
        case 31: serverVersion = ORACLE_11G; break;
        default:
            fprintf(stderr, "unknown version (%d)\n", version);
            return -1;
    }

    /* =========================================================================== */
    if (ORACLE_11G == serverVersion) {
        request_035e = home32bit11gr2_request;
        response_035e_0602 = home32bit11gr2_response_035e_0602;
        response_035e_1017 = home32bit11gr2_response_035e_1017;
        request_0376 = home32bit11gr2_request_0376;
    } else if (ORACLE_10G == serverVersion) {
        request_035e = home64bit10g_plsql_request;
        response_035e_0602 = home64bit10g_response_035e_0602;
        response_035e_1017 = home64bit10g_response_035e_1017;
        request_0376 = home64bit10g_request_0376;
    }

    /* ���濪ʼ����� */
    GET_TYPE(requestBuf, responseBuf, pktType);
    switch (pktType) {
        case PKT_0376:/* �û��� */
            logMsg("clean0376\n");
            if (sharedm->count > 0) shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
            request_0376(requestBuf+10, requestLen-10, NULL, 0);
            return 0;
        case PKT_OTHER:
            logMsg( "PKT_OTHER\n");
            return 0;
        case PKT_035E__04XX: /* ��������Ϊ�� */
            logMsg("clean22222\n");
            getUsernameAndClientType(username, sizeof(username)-1, &clientType);
            if (sharedm->count > 0) shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
            return 0;
        default: break;
    }
    getUsernameAndClientType(username, sizeof(username)-1, &clientType);

    /* ����request */
    /* ��sql���ģ���ͷ������1169��116b��, ���س���select���ص�1017��0602�⣬insert�Ȼ��������������Ͱ� */
    sql_str_main[0] = '\0';
    if (0x11 == requestBuf[10] || (0x03 == requestBuf[10] && 0x5e == requestBuf[11])) {
        tmpret = requestParser(requestBuf, requestLen, sql_str_main, sizeof(sql_str_main)-1);
        if (tmpret < 0) {
            unprintToSpace(requestBuf+10, requestLen-10, sql_str_main, sizeof(sql_str_main)-1);
        }
    } else {
        sql_str_main[0] = '\0';
    }
    app_id = getHighestLevelSqlAction(sql_str_main);

    #if TNS_DEBUG
    if (app_id != 6 && PKT_035E__1017==pktType) {
        fprintf(stderr, "(app_id != 6 && PKT_035E__1017==pktType) : \n");
        fprintf(stderr, "request = ");
        int zz;
        for (zz=10; zz<requestLen; zz++)
            fprintf(stderr, "%02x ", (unsigned char)requestBuf[zz]);
        fprintf(stderr, "\n");

        fprintf(stderr, "response = ");
        for (zz=10; zz<requestLen; zz++)
            fprintf(stderr, "%02x ", (unsigned char)responseBuf[zz]);
        fprintf(stderr, "\n");
    }
    #endif

    /* ����response */
    /* ֻ����select���ص�1017��0602�� */
    response_str[0] = '\0';
    if (PKT_035E__1017==pktType || PKT_035E__0602==pktType) {
        tmpret = responseParser(responseBuf, responseLen, response_str, sizeof(response_str)-1, pktType);
        if (tmpret < 0) {
            unprintToSpace(responseBuf+10, responseLen-10, response_str, sizeof(response_str)-1);
        }
    } else {
        response_str[0] = '\0';
    }

    //===========================================================================
    /* ����select�׻غ� */
    if (PKT_035E__1017==pktType || PKT_035E__0602==pktType) {
        /* ���γ�������, �����ֵ�� */
        resPos = bufForColValuesmain;
        for (i=0; i<totalColInfoNum; i++) {
            if (i != totalColInfoNum-1) sprintf(resPos, "%s;:s", colInfos[i].colName);
            else sprintf(resPos, "%s;:n", colInfos[i].colName);
            resPos += (strlen(colInfos[i].colName) + 3);
        }
        strcpy(resPos, response_str);
    }

    /* ע: ��pl sql developerʱ:
           �˴�ֻ�ж�11xx����+1017�ظ���ϵ�select, plsql developer�ȵ�����غ��У���һ��11xx+1017����
           ��󣬻�������ŷ���035e+1017������Ļغ��ط�һ��, ��Ҫ�������������ѱ���select �ĺ�������
           �Ͽ���, ��������ʧ��.
           ����һ��select�غϵģ��а�β����ORA-01403����������һ��11xx+1017��select����, ��ʱ*/
    if ((6 == app_id && 0x11 == requestBuf[10] && 0x10==responseBuf[10] && 0x17==responseBuf[11] && CLI_PL_SQL_DEVELOPER==clientType)
    || (6 == app_id && 0x10==responseBuf[10] && 0x17==responseBuf[11] && CLI_PL_SQL_DEVELOPER!=clientType)) {
        logMsg("clean-select+1017\n");
        if (sharedm->count > 0) shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
        if (isLast) {
            logMsg("ruku1017-islast\n");

            /* select�׻غϴ�����, ��дredis�� */
            writeDBfile(dir_id, capture_time, app_id, src_ip, src_port, src_mac, dst_ip, dst_port, dst_mac, interval_time, line_num, sql_str_main, bufForColValuesmain);
        } else {
            /* select, ����Ϣ, ������, ��ֵ��(�еĻ�)����д�� */
            logMsg("save1017  request = %s\nresponse=%s\n", sql_str_main, bufForColValuesmain);
            saveSelectColInfosColValues(sharedm,
                hash, src_ip, src_port, src_mac, dst_ip, dst_port, dst_mac, capture_time, requestCaptureTime, responseCaptureTime, line_num, dir_id,
                sql_str_main,
                colInfos, totalColInfoNum, totalColInfoNum,
                bufForColValuesmain);
        }
        return 0;
    } else if (0x06==responseBuf[10] && (0x02==responseBuf[11] || 0x01==responseBuf[11] || 0x22==responseBuf[11] || 0x1a==responseBuf[11])){
        if (exists(sharedm, hash, src_ip, src_port, dst_ip, dst_port)) {
            if (isLast) {
                logMsg("rukuisLast: 0602\n");
                if (sharedm->count > 0) shmDelOneRecodWithLastColValues(sharedm, hash, src_ip, src_port, dst_ip, dst_port, response_str, line_num, capture_time, responseCaptureTime);
            } else {
                logMsg("save0602 request=%s\nrespones=%s\n", sql_str_main, response_str);

                saveColValues(sharedm, hash, src_ip, src_port, dst_ip, dst_port, response_str, line_num, capture_time, responseCaptureTime);
            }
            return 0;
        } else {
            logMsg("ruku333 0602=%s\n", response_str);
            writeDBfile(dir_id, capture_time, 6,
                        src_ip, src_port, src_mac,
                        dst_ip, dst_port, dst_mac,
                        interval_time, line_num, sql_str_main, bufForColValuesmain);
            return 0;
        }
    } else {
        ;
        //�����кܶ���ʱ, select���ͺ����plsql��������һҳ�İ��м��������������
        //����ֻ����ͬ��������select�غϣ����ϻ��������һ�����ذ�(ORA-01403)�ܽ���һ��select�غ�
        //shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
    }

    //===========================================================================
    if (20 == app_id) {
        trim(response_str);
        trim(sql_str_main);
        if (strlen(response_str)<20 && strlen(sql_str_main)<20) {
            logMsg("other not ruku: short data\n");
            return 0;
        }
    }

    /* ODBC */
    if (31 == app_type) {
        app_id = 31;
    }

/* ���ڲ���ƥ��ĵ�writeFile()���ˣ��˴�����Ҫ�� */
#if 0
    /* ����ƥ��
       ע: ����ƥ�����ŵ�response�������, ��Ϊresponse��ı�app_id��ֵ */
    CACHE_POLICY_CONF *policy = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm();
    if(NULL == policy){
        printf("Fails to get CACHE_POLICY_CONF");
        return 0;
    }
    sprintf(zinfo.cspHead.policytime, "%d", getPolicyTime(capture_time));
    struct in_addr in;
    char src_ip_str[30] = {0};
    in.s_addr=src_ip;
    strcpy(src_ip_str, inet_ntoa(in));
    strcpy(zinfo.cspHead.userip, src_ip_str);
    zinfo.type = app_id;
    if (0 == policy_match(&zinfo, policy)) {
        return 0;
    }
#endif

    /* д����ļ� */
    logMsg("ruku other(%d): request=%s\n respnose=%s\n", app_id, sql_str_main, response_str);
    writeDBfile(dir_id, capture_time, app_id, src_ip, src_port, src_mac, dst_ip, dst_port, dst_mac, interval_time, line_num, sql_str_main, response_str);
    return 0;
}

