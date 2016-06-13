/*
目前的情况:
ODBC:
032b_08xx
0347_0602

qingyang:
11xx_1017
035e_0602

home:
11xx_1017
0305_0602
后两个要有2个单独的版本，且保留现有的混合版本
以后测试用单独的版本, 若还遇到青羊的情况，则用混合版本

1. request和response还是要分开解析, 因为request解析完，判断app_id，若不通过, 则省去response解析.
而合在一起, 仅仅少1次函数调用, 得不偿失。
2. 混合版本应该基本用不到, 但可以保留做参考
3. 设置版本号和配置号, 以便调用不同的解析函数

修改:
================================================================================
2015-12-11
mainoracle()处
1. requet文件名, 生成response文件名, 简化过程
2. 把request,response文件名变量，由全局改为局部变量, 变量名剪短,requestFile, responseFile
3. 把删除命令数组cmdRmRequest, cmdRmResponse改为局部变量, 因为暂时用不到
4. sscanf后的checkOldList前加个条件判断, 减少进入函数的次数
5. sql_str的清零动作取消, 解析函数中要保证字符串末尾填\0
6. pktType由全局变量, 改为局部变量, response解析函数responseParser(), responseParserHelper()参数列表末尾加参数int type.
7. mainoracle()里面包类型判断一律有pktType
8. 用户名包, 后续内容为空的包, 其他非sql的数据包, 在一个switch中处理, 取代if-else结构
9. 把request的青羊中医院的格式解析，和家里的64解析分成2个独立函数:request_qingyang64bit10g_plsql(),request_home64bit10g_plsql
10. 只对11xx和035e开头的request调用解析函数, 其他调用不可显字符处理函数
11. 去掉response_str的memset清理操作, response解析函数中保证字符串末尾填\0
12. 去掉bufForColValuestmp的memset清零
13. 去掉select的返回行生成部分，bufForColValuesmain的memset清零
14. 把select的返回行生成部分的resPos += strlen(resPos);
    改为resPos += (strlen(colInfos[i].colName) + 3);
    因为bufForColValuesmain不保证全是\0了
15. 把select的返回行生成部分, 改为只在1017或0602返回包时才生成

1017和0602的解析函数:
1. 把1017和0602的解析函数，都拆成2个独立的函数，分别对应青羊医院的64位10g和家里的64位10g版本。且原来的混合版本保留。
2. 把1017解析函数(2个版本), 去掉无关紧要的值的读取，只取类型,字符宽,列名. 简化列信息解析过程
3. 删除1017和0602解析函数开头，对长度的甄别，保证在调用前过滤掉过短的数据
4. 增加2个全局的函数指针response_035e_0602, response_035e_1017，根据不同版本，设置不同的解析函数
5. 把response_035e_1017_qingyang和response_035e_1017_home64bit10g函数末尾的0602解析函数，改为对应版本的函数
6. 把response_035e_0602_qingyang的获取列信息的部分从循环中的seq==0部分，移动到函数最开始处
7. 0602解析函数的调用，存在2种情况，1是获取后序包时返回的0602，2是在1017包后追加上0602包(家里的sqllus是这种)
   因此读redis获取列信息要判断下，否则第2种情况时，列信息还没写redis了，就会返回空的列信息。解析出错
8. 去掉requestParser(),responseParser()里对bufForTnsRebuid的memset清零，因为tnsRebuild
   重组的数据不是字符串，且一直有长度参数，没必要都设为\0
9. 在request_home64bit10g_plsql(), request_qingyang64bit10g_plsql()和requestParserHelper_64bit10g_plsql_homeAndqingyang()最开始，先把res初始化为空串

segmentsReform()
1. 重组完后，在结果串末尾添加\0

parseColValue()
1. 去掉调用parseCHAR()之前，bufForBigCharRebuid的memset清零
2. 把第1部分的strlen(res), 改为resPos-res, 因为res不保证清零

parseCHAR()
1. 在第3种情况处，向结果串末尾填\0，防止没清零的情况

saveColInfos
1. 去掉tmpColInfos的memset清零，只将第1个字节设为\0
2. 去掉redisColInfos的memset清零
3. 在读redis里的列信息时，pos没初始化, 修改了该bug

saveSelectColInfosColValues
1. checkold后，free还是空的，那么有问题，直接exit，取代return -1
2. 所有的写redis的EX超时的值，都改为120s，取代1200s
3. 缓存rows键处, bufForColValuestmp的memset清零，完全没必要
4. 取消该函数的colName参数, 并在缓存rows键处，取消bufForColValuestmp的应用，直接用colValues
5. 缓存colInfos键处, redisColInfos在后面已经保证最后加\0了，所以去掉memset清零

saveColValues
1. 去掉colNames参数
2. 在读redis前，去掉bufForColValuestmp的memset清零

shmDelOneRecodWithLastColValues
1. 去掉调用getSelectAndResponse()函数前, bufForColValuestmp的memset清零
2. while中不能return，改为break
3. 在函数最后，要把dummy.next重新挂回到hash上

shmDelOneRecod
1. 去掉调用getSelectAndResponse()函数前, bufForColValuestmp的memset清零(和shmDelOneRecodWithLastColValues()的问题一样)
2. while中不能return，改为break(和shmDelOneRecodWithLastColValues()的问题一样)
3. 在函数最后，要把dummy.next重新挂回到hash上(和shmDelOneRecodWithLastColValues()的问题一样)

getSelectAndResponse
1. 在开头把sqlbuf和resultbuf的第1个字节设为\0，防止意外return

checkOldList
1. 去掉调用getSelectAndResponse()函数前, bufForColValuestmp的memset清零

getHighestLevelSqlAction
1. 把delete等关键字的strlen, 改为常数, 省去多次函数调用

================================================================================
2015-12-14
mainoracle():
1, 修改了requset解析函数调用的条件, 原先是pktType==PKT_035E__XXXX比较，这个把select的都漏掉了.
   改为(0x11 == requestBuf[10])
2. 修改了response解析函数调用，原先是比较responseBuf[10]/[11]，改为(PKT_035E__1017==pktType || PKT_035E__0602==pktType)
   简洁了些
3. 把colInfos和totalColInfoNum的清零，从mainoracle()的开头未知，移动到responseParser()中，省去非select返回包时的动作
4. 在没有打印的return前，加上打印
5. 把response_str和sql_str在解析之前置为空串，因为1017返回包解析时，若不包含0602分包，那么不对
   response_str做任何操作。所以要是置空串，那么他的内容未知，在生成select结果行时，会出错


GET_TYPE():
1. 参数request和response都是没有偏移10字节的，比较的字节改为10和11

checkOldList():
1.在检查isold前后加上两个打印, 查看isold的识别情况
2.修改isold传入的参数, 原来是capture_time/1000000, 但是若解析停了一段时间后, 再开启, 那么所有的包都将是isold了
  所以要把识别的时间改为入链的时间.

setUsername_0376()
1.把setUsername_0376()函数分成两个版本, 青羊区中医院的版本，和家里的64位10g的版本

responseParser()
1. 在开头，加入res[0]='\0';
2. 在置res为空串后，加入长度判断，跳过长度过小的包

================================================================================
2015-12-15
1.增加了LONG, LONG RAW, RAW类型的识别函数，并在parseColValue()添加相应代码
2. parseColValue()里的if-else结构，改为switch-case结构
3. writeDBfile()函数中调用system()来移动函数，费时1500微秒以上，改成用rename()函数, 耗时130微秒左右

2015-12-23:
1.在writeFile()里增加了策略匹配代码
2. 修改字符集转换部分代码，NCHAR,NVARCHAR的UCS-2编码，都转为为和CHAR,VARCHAR等相同的编码，
   增加一个宏TNS_CHARSET，用于指定CHAR,VARCHAR的编码。
   删除CHAR,VARCHAR等部分的codeConv()调用，改为直接memcpy()

================================================================================
2016-01-28
1, 在0376解析函数里，增加客户端类型的判定。
   增加全局标记变量clientType,
   增加表示客户端类型的宏:
#define CLI_MINI 11
#define CLI_PL_SQL_DEVELOPER 11
#define CLI_SQLPLUS 12
#define CLI_ORACLE_SQL_DEVELOPER 13
#define CLI_OTHER 14
#define CLI_MAX 14
#define IS_VALID_CLIENT(n) ((n)>=CLI_MINI && (n)<=CLI_MAX)
2, setUsername_0376_home64bit10g函数名改为request_0376_home64bit10g
   setUsername_0376_qingyang64bit10g_plsql函数名改为request_0376_qingyang64bit10g_plsql
3, request_0376_home64bit10g里的几个if-else改为?:表达式实现。
4, response_035e_0602全局变量类型由REQUEST_PARSER改为RESPONSE_PARSER.
   增加REQUEST_PARSER类型的全局变量request_035e
5, 在函数getUsername()中，增加读取clientType值的动作，并把函数名改为getUsernameAndClientType()

2016-02-02:
1,增加宏
#define ORACLE_10G 10
#define ORACLE_11G 11
2，把全局变量oracleVersion改为serverVersion,并把所有的值改为使用上面定义的宏，取消硬编码
3, 添加logMsg()日志函数，用于调试，替代
#if TNS_DEBUG
fprintf(...)
#endif
这种调试语句，试代码更加紧凑。(缺点是当TNS_DEBUG为0时, 比以前引入了许多空函数调用)
4, requestParser()里的请求解析函数，改为调用全局变量request_035e()。
5，去掉mainoracle()里，处理request部分的unprintToSpace()，使之只处理11, 035e两种回复，
   其他类型回复的解析结果都设为空串。

2016-02-03:
1, 把response解析部分，非1017，0602的包，不用unprintToSpace()，直接把结果设为空串

2016-02-23:
1, 今天开始调试oracle 11g r2 32位版本。
2，把本文件结构改为每个oracle版本一套函数，不要在同一个函数里集合多个版本，容易混乱，且不易修改
3，其他解析函数的名称，也把版本字段改到开头, 并把每个版本的解析函数放到一起
   home64bit10g_request_0376()
   qingyang64bit10g_plsql_request_0376()
4，新增函数:
   home32bit11gr2_request(),
   home32bit11gr2_response_035e_1017(),
   home32bit11gr2_response_035e_0602().
5，GET_TYPE()和mainoracle()在0602处，增加0601的识别
6，getUsernameAndClientType()增加一个参数ct，用于返回clientType，取消以前直接把
   clientType直接放到全局变量的做法

2016-02-25:
把所有的类型包都分成不同的client分别处理。

2016-20-26
发现一个bug，mainoracle()里解析出的sql语句保存在全局变量sql_str, 但在将其入库之前，若要结束上一条sql
请求，shmDelOneRecod()中还用sql_str保存上一条sql，就会把当前的sql覆盖掉了。
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
#include "/usr/src/inp/audit_new/redis_new_api.h" /* 2015-11-16,新增snmp */
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

/* 按照实际的oracle字符集设置 */
#define TNS_CHARSET "GBK"

/* client类型 */
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

/* redis相关 */
#define REDISSERVERHOST      "127.0.0.1"
#define REDISSERVERPORT      6379
#define REDIS_ORACLE_TABLE  14

#define TEMP_SQL_DIR "/data/audit/sql_tmp"
#define SQL_DIR "/data/audit/sql"

#define MAX_COLUMN 1024

/* TNS头type字段常见取值, 只有0x06(DATA类型)有用到，都列出来是防止以后用到 */
#define PKT_TYPE_CONNECT 0x01
#define PKT_TYPE_ACCEPT 0x02
#define PKT_TYPE_ACK 0x03
#define PKT_TYPE_REFUSE 0x04
#define PKT_TYPE_REDIRECT 0x05
#define PKT_TYPE_DATA 0x06 /* 数据(最重要的类型,sql请求和返回都该类型) */
#define PKT_TYPE_NULL 0x07
#define PKT_TYPE_ABORT 0x09
#define PKT_TYPE_RESEND 0x0b
#define PKT_TYPE_MARKER 0x0c
#define PKT_TYPE_ATTENTION 0x0d
#define PKT_TYPE_CONTROL 0x0e

#define TNS_NORMAL 0 /* 是0x06包，且解析正常 */
#define TNS_DEL -1   /* 非0x06包 */
#define TNS_SAVE -2  /* 是0x06包，解析异常 */

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
    unsigned short length; /* TNS包长度，包括TNS头 */
    unsigned short packetCheckSum; /* 包的校验和 */
    unsigned char type; /* TNS包类型 */
    unsigned char flag; /* 状态 */
    unsigned short headerCheckSum; /* TNS头的校验和 */
} TNS_HEADER;

typedef struct {
    char colName[1024];
    unsigned char type;
    unsigned char charWidth; /* 当CHAR,VARCHAR时是1，NCHAR,NVARCHAR时是2 */

    /* 用于NUMBER,FLOAT. 在NUMBER(p,s)中，
    p是精度，表示有效数字的位数,
    s为正数时，表示从小数点到最低有效数字的位数，它为负数时，表示从最大有效数字到小数点的位数 */
    unsigned char precision;
    unsigned char scale;
    unsigned long maxByteSize; /* 所占最大空间 */
    unsigned char nullbit;
    unsigned int seq;
}COLUMN_INFO;

typedef struct {
    char *colValues[MAX_COLUMN+1];
    int colSizes[MAX_COLUMN+1];
}PREV_ROW_INFO;

/* 客户PL/sql developer 7访问oracle 10g 64bits */
#define PKT_11XX_035E__1017 0 /* 03 5E请求包含sql语句, 10 17包含列信息 */
#define PKT_035E__1017 1
#define PKT_035E__0602 2 /* 03 5E请求后序包, 06 02包含列值 */
#define PKT_035E__04XX 3 /* 03 5E请求后序包, 04 01表示请求结果为空 */
#define PKT_035E__XXXX 4 /* 03 5E请求, 回复其他非1017,0602,04XX的命令包 */

#define PKT_0305__0602 5 /* 03 05请求后序包, 06 02回复列值 */
#define PKT_0305__04XX 6 /* 03 05请求后序包, 04 01表示请求结果为空 */

/* 客户odbc访问oracle 10g 64bits */
#define PKT_034A__XXXX 7 /* 03 4A包含sql语句, 04XX包含统计信息 */
#define PKT_032B__08XX 8 /* 06 2B请求列信息, 08XX包含列信息 */
#define PKT_0347__0602 9 /* 06 02包含列值, 06 02回复列值 */
#define PKT_0347__04XX 10 /* 03 47请求后序列值, 04xx返回没有数据 */
#define PKT_0376 11

#define PKT_OTHER 100

char pre_sql_str[5*1024] = {0};       /* 存储request解析结果 */
char sql_str_main[5*1024] = {0};           /* 存储request解析结果 */
char response_str[1024*1024] = {0}; /* 存储response解析结果 */
unsigned char bufForTnsRebuid[2*1024*1024]; /* 保存TNS重组后的数据, 需要unsigned */
unsigned char requestBuf[2*1024*1024], responseBuf[2*1024*1024]; /* 保存读文件的数据 */

char bufForBigCharRebuid[5*1024];
char bufForColValuesmain[10*1024*1024];
char bufForColValuestmp[10*1024*1024];
COLUMN_INFO colInfos[MAX_COLUMN];
unsigned char redisColInfos[10240]; /* 需要unsigned, 有字节比较 */
unsigned char tmpColInfos[10240];   /* 需要unsigned, 有字节比较 */
PREV_ROW_INFO prevRow;
int isLast = 0; /* 1表示select最后一个回合 */
int totalColInfoNum = 0;
int line_num = 0; /* 06 02里的行数 */

CSP_FILE_INFO zinfo;

unsigned int src_ip, src_port, dst_ip, dst_port; /*255.255.255.255(0xffffffff)是ip最大值, 可用unsigned int表示*/
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

/* 日志函数。成功返回0，出错或没初始化返回-1。*/
char outputlogMsg[10240];
int logMsg(char *format, ...) {
    va_list ptr;
    char *lineend;

#if TNS_DEBUG
    va_start(ptr, format);

    /* 并不是所有的系统都支持vsnprintf() */
#ifdef NO_SNPRINTF
    vsprintf(outputlogMsg, format, ptr);
#else
    vsnprintf(outputlogMsg, sizeof(outputlogMsg)-1, format, ptr);
#endif

    /* 保证最后一个字符是换行 */
    lineend = ('\n' == outputlogMsg[strlen(outputlogMsg)-1]) ? "" : "\n";

    fprintf(stderr, "%s%s", outputlogMsg, lineend);
#endif
    return 0;
}

/* 超过20s就老化 */
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

/* 从redis里读取username和clientType值，合在一起少掉一次连接过程
   "username"是长"len"的缓存，clientType保存在"ct"中*/
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

    /* 2015-11-16, 新增snmp
    char tmpSrcmac[64]={0}, tmpDstmac[64]={0};
    get_mac_str(src_ip_str, tmpSrcmac);
    get_mac_str(dst_ip_str, tmpDstmac);
    if (tmpSrcmac[0] != '\0' && tmpDstmac[0] != '\0') {
        strcpy(src_mac, tmpSrcmac);
        strcpy(dst_mac, tmpDstmac);
    }*/


    /* 策略匹配 */
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


/* 1,从redis里读select语句
   2,从redis里读select返回 */
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

    //获取并删除select
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

    //获取并删除列值
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

    //列信息不删除, 靠redis自己计时删除, 列信息在每次保存select语句时更新
    //select * from help之类的查询, PL/sql developer不会一次全部传输, 要点
    //下一页才传, 这就要求列信息比select语句要留存更长的时间.而select语句会
    //由老化链来入库.
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

/* 从开始遍历, 把超过10s的入库
   注意: 判断的时间，应该是生成入库文件的当前时间，千万不能用包文件的生成时间
   因为若处理的延迟过大的话，会把组合功能顶掉 */
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
            cur = cur->oldnext; /* cur向前一步 */

            /* 下面这段, 只操作node, 不要动cur了 */
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

/* 查hash表, 若有, 则写入库文件, 否则不做动作， 没有最后列值，redis里已经是最后的列值了  */
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

/* 查hash表, 若有, 则写入库文件, 否则不做动作, 需要追加最后一部分列值*/
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

    /* 清理老化链 */
    if (!(shm->free)) {
        if (shm->count > 0) checkOldList(shm);
        if (!(shm->free)) {
            printf("(!(shm->free))\n");
            exit(-1);
        }
    }

    /* 取得1个free结构, 将数据填入结构 */
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

    /* 清理同一四元组的记录 */
    if (shm->count > 0) shmDelOneRecod(shm, hash, src_ip, src_port, dst_ip, dst_port);

    /* 挂到hash表 */
    node->next = shm->hashTable[hash % HASH_TABLE_SIZE];
    shm->hashTable[hash % HASH_TABLE_SIZE] = node;

    /* 挂到老化链 */
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

    /* 缓存rows键 */
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

    /* 缓存colInfos键 */
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
            *pos++ = (0x00 == res[i].charWidth) ? 0x01 : (res[i].charWidth); /* 防止charWidth取值错误, 使串内带有\0 */
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

    /* sql语句入redis */
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

/* 1, 读取已有的
   2, 追加在已有的后面
   3, 写redis */
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

    /* 列名已经写入redis了, 先读出已有的, 在追加在后面 */
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

/* 格式:
   意义     字节数
   总列数     首字节是该字段长度
   当前列数     首字节是该字段长度

   列1类型  1字节
   字符宽度 1字节(只有当类型是0x01时才有)
   列名     首字节是该字段长度
   后面重复列1的格式
   */
int saveColInfos(char *key, COLUMN_INFO *res, unsigned int num, unsigned int totalNum) {
    unsigned char *pos;
    int i;
    unsigned int len, tmpNum, tmpTotal;
    redisContext * conn;
    redisReply *reply =NULL;

    /* 连接redis */
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

    /* 重要: 若列数不对, 那直接覆盖 */
    /* tmpColInfos保存redis里已保存的列信息(ODBC时，列信息是多个回合传的) */
    tmpColInfos[0] = '\0';
    if (num == totalNum) {
        tmpNum = 0;
    } else {
        /* 先get */
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

    /* 续接 */
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
            *pos++ = (0x00 == res[i].charWidth) ? 0x01 : (res[i].charWidth);/* 防止charWidth取值错误, 使串内带有\0 */
        }
        len = strlen(res[i].colName);
        *pos++ = len;
        strcpy(pos, res[i].colName);
        pos += len;
    }
    *pos = '\0';

    /* 再set */
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

/* 删除'str'开头和末尾连续的空白字符. 原址进行
   返回结果串的长度 */
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

/* 把1个或连续的多个不可打印字符变成一个空格，且忽略\0
   返回结果串长度 */
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


/* 字符集转换。成功则返回结果串长度，失败返回-1
   注意 - GBK/GB2312等，属于一种，所以他们之间转换会在iconv()上出错 */
int codeConv(char* srcCharSet, char *dstCharSet, char *src, size_t srcLen, char *dst, size_t dstMaxSize) {
    iconv_t cd;
    size_t tmpLen = dstMaxSize;
    char *tmppos = dst;

    if (NULL==srcCharSet || NULL==dstCharSet || NULL==src || NULL==dst || srcLen<0 || dstMaxSize<0 ) {
        perror("Incorrect parameter\n");
        return -1;
    }

    /* 当二者都是GBK的一种时，不需要转换 */
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
   二进制 = 0100 0000 0101 1011 1100 0111 0000 1010 0011 1101 0111 0000 1010 0011
   符号位F(1位) = 0(+)
   指数位Z(11位) = (100 0000 0101)2 = (1029)10
   尾数W(23位) = 0.1011 1100 0111 0000 1010 0011 1101 0111 0000 1010 0011 = 0.73609374999995225152815692126751
   该IEEE754数表示的十进制数 = (-1)^F * 2^(Z-1023) * (1+W) = 111.10999999999694409780204296112

   若Z = 0, 结果是(-1)^F * 2^(-126) * W
   若Z非0和0xff, 结果是(-1)^F * 2^(Z-127) * (1+W)
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

/* data[] = {0xb6, 0x33, 0x24, 0x40},最高位是左边
   表示IEEE754数 = 0x402433b6
   二进制 = 0100 0000 0010 0100 0011 0011 1011 0110
   符号位F(1位) = 0(+)
   指数位Z(8位) = (100 0000 0)2 = (128)10
   尾数W(23位) = 0.0100100001100111011010100111001
   该IEEE754数表示的十进制数 = (-1)^F * 2^(Z-127) * (1+W)
   应该是2.565656

   若Z = 0, 结果是(-1)^F * 2^(-126) * W
   若Z非0和0xff, 结果是(-1)^F * 2^(Z-127) * (1+W)

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

/* 返回解析了data多少字节
   @resMaxSize传进@res最大空间，并最后返回存入res的字节数
   注: 在结果串末尾添加\0 */
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
    return parsedLen+1; /* 别忘了最后结尾的0x0 */
}

/* 返回解析结果的长度，@datalen返回解析的data的长度 */
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
        *datalen = len + 1; /* 别忘了开头的0xfe */
        return tmplen;
    }

    len = *pos++;
    memcpy(res, pos, len);
    res[len] = '\0';
    *datalen = len+1;
    return len;
}

/* 返回解析结果的长度，@datalen返回解析的data的长度 */
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
        /* 正数 */
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
        /* 负数 */
        intLen = 0x3f - (unsigned int)*pos;
        *resPos++ = '-';
        if (intLen >= len-1-1) {
            padLen = intLen - (len - 1 - 1);
            intLen = len - 1 - 1;
            docLen = 0;
        } else {
            padLen = 0;
            docLen = len - intLen - 1 - 1; /* 第1个字符和最后1个字符0x66(0x66是负数仅有) */
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
        pos++; /* 跳过末尾的0x66 */
    }

    *datalen = pos-data;
    return resPos-res;
}

/* 返回解析结果的长度，@datalen返回解析的data的长度 */
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
        /* *datalen不变，就跳过后面的所有字段 */
        return 0;
    }
    *datalen = (data[0]+1);
    return strlen(res);
}

/* 返回解析结果的长度，@datalen返回解析的data的长度 */
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
        /* *datalen不变，就跳过后面的所有字段 */
        return 0;
    }

    *datalen = (data[0]+1);
    return strlen(res);
}

#define parseLONG_RAW(data, datelen, res, resSize) parseLONG(data, datelen, res, resSize)

/* 返回解析结果的长度，@datalen返回解析的data的长度 */
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
        *datalen = len + 5; /* 开头的0xfe, 末尾的4字节0x00 */
        return tmplen;
    }else {
        printf("not LONG type\n");
        res[0] = '\0';
        /* *datalen不变，就跳过后面的所有字段 */
        return 0;
    }

    return 0;
}

/* 返回解析结果的长度，@datalen返回解析的data的长度 */
/* RAW类型将数据以16进制形式显示, 保存形式和char相似 */
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
    /* 原样 */
    if (0xfe == *pos) {
        pos++;
        len = segmentsReform(pos, res, &tmplen);
        *datalen = len+1; /* 别忘了开头的0xfe */
        return tmplen;
    }

    len = *pos++;
    memcpy(res, pos, len);
    res[len] = '\0';
    *datalen = len + 1;
    return len;
#else
    /* 十六进制格式 */
    if (0xfe == *pos) {
        pos++;
        len = segmentsReform(pos, bufForBigCharRebuid, &tmplen);
        *datalen = len+1; /* 别忘了开头的0xfe */
        return getHexStr2(bufForBigCharRebuid, tmplen, res, resSize);
    }

    len = *pos++;
    memcpy(bufForBigCharRebuid, pos, len);
    bufForBigCharRebuid[len] = '\0';
    *datalen = len + 1;
    return getHexStr2(bufForBigCharRebuid, len, res, resSize);
#endif
}

/* 解析列值，首字节是长度
   @info是列信息, @res保存结果串, @datalen传入data长度, 返回解析的列值部分长度.
   函数返回结果串长度. */
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
/* request = 03 2b, response = 08 0xnn, 回复列信息
环境 : 10g server, ODBC(成都青羊区中医院, 终端访问)
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

        /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
        if (0x60 == colInfos[i].type || 0x01 == colInfos[i].type) {
            colInfos[i].charWidth = *pos;
        }
        pos += 15;
    }
    len = pos[0] + pos[1]*256;

    /* 包不完整 */
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

/* request = 03 47, response = 06 02, 回复列值
环境 : ODBC 访问 64bits oracle 10g server(成都青羊区中医院, 终端访问)
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

        /* 本06 02包的列数, 当和上一行相同时会缺项 */
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

/* request = 03 5e, response = 06 02, 回复列值
环境 : 10g server, PL/sql developer(成都青羊区中医院)
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

        /* 本06 02包的列数, 位图不满时, 该数小于查询列数 */
        curColNum = pos[0] + pos[1]*256;
        pos += 2;

        seq = pos[0] + pos[1]*256;
        pos += 2;

        if (0 == seq) {
            pos += 12;

            /* 读取列格式, 获得总列数 */
            if (0 == totalColInfoNum) {
                totalColInfoNum = MAX_COLUMN;
                getColInfos(src_ip, src_port, dst_ip, dst_port, colInfos, &totalColInfoNum);
                if (totalColInfoNum != curColNum) {
                    return TNS_SAVE;
                }
            }

            /* 初设位图 */
            for (bb=0; bb<(totalColInfoNum/8+1); bb++) {
                bitmap[bb] = 0xff;
            }

            /* 初始化上一行内容 */
            memset(&prevRow, 0, sizeof(PREV_ROW_INFO));

            /* 生成表头 */
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

/* request = 03 5e, response = 10 17, sql语句请求包, 和回复列信息
有时03 5e包前叠加1个11 6b包, 需要向后偏移到03 5e来解析
环境 : 10g server, PL/sql developer(成都青羊区中医院)
若后跟06 02, 则接着解析，否则退出
*/
int qingyang64bit10g_plsql_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';

    pos = data + 29; /* 跳过0x10 0x17 */

    /* 列数 */
    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
    if (colNum > MAX_COLUMN) {
        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
        return TNS_SAVE;
    }
    totalColInfoNum = colNum;

    pos += 5;

    /* 列属性 */
    for (i=0; i<colNum && pos<data+datalen; i++) {
        #if TNS_BINARY_DEBUG
        int iii;
        fprintf(stderr, "%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
        }
        fprintf(stderr, "\n");
        #endif

        colInfos[i].type = *pos++; /* 列类型 */
        pos += 23;
        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
        pos += 6;

        /* 列名 */
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

    /* 先初始化为空串, 防止没找到sql语句直接返回 */
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



/* request = 03 5e, response = 10 17, sql语句请求包, 和回复列信息
环境 : 10g server, PL/sql developer(成都青羊区中医院)或家里的64位10g版本的混合解析
若后跟06 02, 则接着解析，否则退出
*/
int response_035e_1017_reserve(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;
    int flagLocation = UNMEET_PKT; /* 标记哪里的访问数据 */

    res[0] = '\0';

    if (datalen < 34) return TNS_DEL;

    pos = data + (2 + 16 + 7 + 4); /* 跳过0x10 0x17 */

    /* 列数 */
    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
    pos += 4;
    if (colNum > MAX_COLUMN) {
        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
        return TNS_SAVE;
    }
    totalColInfoNum = colNum;

    pos++;

    /* 列属性 */
    for (i=0; i<colNum && pos<data+datalen; i++) {
        #if TNS_BINARY_DEBUG
        int iii;
        fprintf(stderr, "%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
        }
        fprintf(stderr, "\n");
        #endif

        /* 列类型 */
        colInfos[i].type = *pos++;
        pos++; /* 未知, 目前仅在varchar2、char、clob、long类型中出现，值为0x80 */

        /* NUMBER,FLOAT的precision和scale */
        colInfos[i].precision = *pos++;
        colInfos[i].scale = *pos++;
        colInfos[i].maxByteSize = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
        pos += 4;

        pos += 16;

        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */

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

        /* 列名 */
        memcpy(colInfos[i].colName, pos+1, pos[0]);
        colInfos[i].colName[pos[0]] = '\0';
        pos += (1 + pos[0]); /* 跳过colName */

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
    /* PL/sql developer7 访问64bit Oracle 10g */
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
/* oracle10g或oracle11g,
   Oracle sql developer 11g*/
int requestParserHelper(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos047fffffff, *pos;
    int sqlLen, tocopy;
    int offset, i, n;

    if (ORACLE_11G == serverVersion) offset = 15;
    else if (ORACLE_10G == serverVersion) offset = 10;
    else offset = 10;

    /* 找04 7F FF FF FF */
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
        /* 03 4A有这种情况:
03 4A FD 01 00 00
00 3A 00 00 00 01 CF 00  00 00 00 00 00 00 00 01
01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 (后面是0xcf字节的内容)73 65 6C 65  63 74 ...*/
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

    /* 长度字段 */
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

    /* 先初始化为空串, 防止没找到sql语句直接返回 */
    res[0] = '\0';

    if (ORACLE_11G == serverVersion) offset = 15;
    else if (ORACLE_10G == serverVersion) offset = 10;
    else offset = 10;

    /* 找04 7F FF FF FF */
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
        /* 长度字段 */
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
        /* 03 4A有这种情况:
03 4A FD 01 00 00
00 3A 00 00 00 01 CF 00  00 00 00 00 00 00 00 01
01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 (后面是0xcf字节的内容)73 65 6C 65  63 74 ...*/
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

    /* 先初始化为空串, 防止没找到035e直接返回 */
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

/* oracle10g或oracle11g,
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

        /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。 */
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


/* request = 03 5e, response = 10 17, sql语句请求包, 和回复列信息
环境 : 10g server, PL/sql developer(家)
若后跟06 02, 则接着解析，否则退出 */
int home64bit10g_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';

    pos = data + 29;

    /* 列数 */
    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
    if (colNum > MAX_COLUMN) {
        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
        return TNS_SAVE;
    }
    totalColInfoNum = colNum;

    pos += 5;

    /* 列属性 */
    for (i=0; i<colNum && pos<data+datalen; i++) {
        #if TNS_BINARY_DEBUG
        int iii;
        fprintf(stderr, "%d: ------------------------------------\n", i);
        for (iii=0; iii<50; iii++) {
            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
        }
        fprintf(stderr, "\n");
        #endif

        colInfos[i].type = *pos++; /* 列类型 */
        pos += 23;
        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
        pos += 10;

        /* 列名 */
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

/* 0376请求包含有username，应用名 */
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

    /* 先初始化为空串, 防止没找到035e直接返回 */
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

/* request = 03 5e, response = 10 17, sql语句请求包, 和回复列信息
环境 : 10g server, PL/sql developer(家)
若后跟06 02, 则接着解析，否则退出 */
int home64bit11gr2_response_035e_1017(unsigned char *data, long datalen, char *res, int resMaxSize) {
    unsigned char *pos;
    int colNum, i;

    res[0] = '\0';
    if (CLI_PL_SQL_DEVELOPER==clientType) {
        pos = data + 29;
	    /* 列数 */
	    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 5;

	    /* 列属性 */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* 列类型 */
	        pos += 23;
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
	        pos += 10;

	        /* 列名 */
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
	    /* 列数 */
	    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 5;

	    /* 列属性 */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* 列类型 */
	        pos += 23;
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
	        pos += 10;

	        /* 列名 */
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

	    /* 列数 */
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

	    /* 列属性 */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* 列类型 */
	        pos += 2;
            if (pos[0] > 0x01) pos += 2;
            else pos += (1 + pos[0]);

	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
	        pos += (1 + pos[0]);
	        pos += 4;

	        /* 列名 */
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


/* oracle10g或oracle11g,
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
                    pos++;/* 0x00表示空字段，结果串中表示为空串 */
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

            /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。 */
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
                    pos++; /* 0xff表示空字段，结果串中表示为空串 */
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

            /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。
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
                    pos++;/* 0x00表示空字段，结果串中表示为空串 */
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

            /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。 */
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

/* 0376请求包含有username，应用名 */
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

    /* 先初始化为空串, 防止没找到035e直接返回 */
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

        /* 列数 */
        colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
        if (colNum > MAX_COLUMN) {
            fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
            return TNS_SAVE;
        }
        totalColInfoNum = colNum;

        pos += 5;

        /* 列属性 */
        for (i=0; i<colNum && pos<data+datalen; i++) {
            #if TNS_BINARY_DEBUG
            int iii;
            fprintf(stderr, "%d: ------------------------------------\n", i);
            for (iii=0; iii<50; iii++) {
                fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
            }
            fprintf(stderr, "\n");
            #endif

            colInfos[i].type = pos[1]; /* 列类型 */
            pos += 27;
            colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
            pos += 11;

            /* 列名 */
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
	    /* 列数 */
	    colNum = pos[0] + pos[1]*256 + pos[2]*256*256 + pos[3]*256*256*256;
	    if (colNum > MAX_COLUMN) {
	        fprintf(stderr, "%d:too many column (%d)\n", __LINE__, colNum);
	        return TNS_SAVE;
	    }
	    totalColInfoNum = colNum;

	    pos += 5;

	    /* 列属性 */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
            #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
            #endif

	        colInfos[i].type = pos[1]; /* 列类型 */
	        pos += 27;
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
	        pos += 11;

	        /* 列名 */
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

	    /* 列数 */
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

	    /* 列属性 */
	    for (i=0; i<colNum && pos<data+datalen; i++) {
	        #if TNS_BINARY_DEBUG
	        int iii;
	        fprintf(stderr, "%d: ------------------------------------\n", i);
	        for (iii=0; iii<50; iii++) {
	            fprintf(stderr, "0x%02x ", (unsigned char)pos[iii]);
	        }
	        fprintf(stderr, "\n");
	        #endif

	        colInfos[i].type = *pos++; /* 列类型 */
	        pos += 2;
            if (pos[0] > 0x01) pos += 2;
            else pos += (1 + pos[0]);

	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        pos += (1 + pos[0]);
	        colInfos[i].charWidth = *pos++; /* CHAR,NCHAR,VARCHAR2,NVARCHAR的字符宽(1/2) */
	        pos += (1 + pos[0]);
	        pos += 4;

	        /* 列名 */
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

/* oracle10g或oracle11g,
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

	        /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。 */
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
                    pos++; /* 0xff表示空字段，结果串中表示为空串 */
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

            /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。
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
                    pos++;/* 0x00表示空字段，结果串中表示为空串 */
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

            /* 0x15字段，当下一行的数据和本次解析行，存在相同值的列, 该项在下一行中就缺失了。 */
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

/* 0376请求包含有username，应用名 */
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
/* 查找'sqlStatement'里的第一次出现的，引号外面的分号。 */
char *findNextSemicolon(char *sqlStatement) {
    char *pos;
    int singleQuoteFlag, doubleQuoteFlag;

    pos = sqlStatement;
    singleQuoteFlag = doubleQuoteFlag = 0;
    while (*pos != '\0') {
        if (';'==*pos && 0==singleQuoteFlag && 0==doubleQuoteFlag) {
            return pos;
        }

        /* 双引号外的单引号 */
        if ('\'' == *pos && 0==doubleQuoteFlag) {
            singleQuoteFlag = (1 == singleQuoteFlag) ? 0 : 1;
            pos ++; continue;
        }

        /* 单引号外的双引号 */
        if ('"' == *pos && 0==singleQuoteFlag) {
            doubleQuoteFlag = (1 == doubleQuoteFlag) ? 0 : 1;
            pos ++; continue;
        }

        /* 引号内的引号 */
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

/* <1>多句sql组合，每句间用分号分割
   <2>sql语句中有引号(", ''"), 找下一句时要跳过
   <3>开头，sql语句间，结尾有注释，要跳过注释*/
int getHighestLevelSqlAction(char *sqlStatement) {
    char *start, *pos, actionWord[64];
    int tocopy, sqlLen, returnFlag;
    int sqlLevel = 0;

    sqlLen = strlen(sqlStatement);
    pos = sqlStatement;
    returnFlag = 19;
    while (pos < sqlStatement + sqlLen) {
        /* 找第1次出现的字母. 该过程中要跳过注释 */
        while (!isalpha((int)*pos) && (pos < (sqlStatement+sqlLen))) {
            /* 跳过注释. sqlserver注释有2种/ * * /块注释，以及--行注释 */
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

        //找该字母后面第1次出现的空白字符, 或sql语句分割符(分号;)
        pos = strpbrk(start, " \r\n\t\f;");
        if (NULL == pos) {
            tocopy = (strlen(start) > (sizeof(actionWord)-1)) ? (sizeof(actionWord)-1) : strlen(start);
        } else {
            tocopy = ((pos-start) > (sizeof(actionWord)-1)) ? (sizeof(actionWord)-1) : (pos-start);
        }

        memcpy(actionWord, start, tocopy);
        actionWord[tocopy] = '\0';

        /* delete, insert, update, select 依次是4,3,2,1，其他是0 */
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
        pos++; /* 跳过分号(;),进入下一句sql */
    }

    switch (sqlLevel) {
        case 0: return 20;
        case 1: return 6;
        case 2: return 7;
        case 3: return 9;
        case 4: return 8;
    }
    return 20; /* 其他操作语句 */
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

/* read()的封装。成功返回读取的字节数，失败返回-1 */
ssize_t readBytes(int fd, void *buf, size_t n) {
    char *pos = buf;
    ssize_t nrd, nLeft=n;

    while (nLeft > 0) {
        if ( (nrd = read(fd, pos, nLeft)) < 0) {
            if (EINTR == errno) {
                nrd = 0;  /* 再次调用read */
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

/* 从文件'fileName'读'len'字节到'buf'.
   返回成功读的字节数, 失败返回-1 */
ssize_t readFile(char *fileName, void *buf, size_t n) {
    char *pos = buf;
    int fd, nrd;

    fd = open(fileName, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "Fails to open the file(%s)", fileName); return -1; }
    if ((nrd = readBytes(fd, pos, n)) < 0) { fprintf(stderr, "Fails to read the file(%s)", fileName); return -1; }
    close(fd);
    return nrd;
}

/* 来1个select包，表示新回合开始, 在hash表中添加新结点, 并把结点加入老化链. 若hash表中已存在该四元组, 则将其入库
来1个列信息包, 在hash表中找到对应, 将列信息写入redis
来1个列值包, 在hash表中找到对应, 将列值写入redis

hash值, ip, port, mac, capture_time, interval_time, line_num存到共享内存
sql语句保存在src_ip-src_port-dst_ip-dst_port-sql中
列值保存在src_ip-src_port-dst_ip-dst_port-rows中
列格式保存在src_ip-src_port-dst_ip-dst_port-cloInfos中 */
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

    /* 由request文件名，获得response文件名 */
    strcpy(requestFile, filePath);
    len = strlen(requestFile);
    memcpy(responseFile, requestFile, len-7);
    strcpy(responseFile+len-7, "response");
    responseFile[len+1] = '\0';

    /* 读文件, request和response都存在才解析 */
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

    /* 请求执行时间 */
    long requestCaptureTime = getFileTime(requestFile);
    long responseCaptureTime = getFileTime(responseFile);
    long tmpdif = requestCaptureTime - responseCaptureTime;
    unsigned long interval_time = (tmpdif < 0) ? (-tmpdif) : tmpdif;

    /* 删除文件: 必须放在读文件之后，解析之前，防止段错误导致删除不了, 进而导致程序卡住 */
    deleteFile(requestFile, responseFile);

    logMsg("\n\n=========================================================\n"
           "request = %02x %02x, response = %02x %02x\n",
           requestBuf[10], requestBuf[11], responseBuf[10], responseBuf[11]);

    tmpret=sscanf(requestFile,
                  "/dev/shm/oracle/%d/%lu_%u_%d_%u_%12[^_]_%u_%u_%12[^_]_%u_%d_%d_request",
                  &dir_id, &capture_time, &hash, &app_type,
                  &src_ip, src_mac, &src_port, &dst_ip, dst_mac, &dst_port, &version, &seqid);

    if (sharedm->count > 0) checkOldList(sharedm);

    /* version=30_..是10g版本， 31_...是11g版本 */
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

    /* 下面开始处理包 */
    GET_TYPE(requestBuf, responseBuf, pktType);
    switch (pktType) {
        case PKT_0376:/* 用户名 */
            logMsg("clean0376\n");
            if (sharedm->count > 0) shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
            request_0376(requestBuf+10, requestLen-10, NULL, 0);
            return 0;
        case PKT_OTHER:
            logMsg( "PKT_OTHER\n");
            return 0;
        case PKT_035E__04XX: /* 后序内容为空 */
            logMsg("clean22222\n");
            getUsernameAndClientType(username, sizeof(username)-1, &clientType);
            if (sharedm->count > 0) shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
            return 0;
        default: break;
    }
    getUsernameAndClientType(username, sizeof(username)-1, &clientType);

    /* 处理request */
    /* 带sql语句的，开头命令是1169，116b等, 返回除了select返回的1017，0602外，insert等还有其他返回类型包 */
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

    /* 处理response */
    /* 只处理select返回的1017和0602包 */
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
    /* 若是select首回合 */
    if (PKT_035E__1017==pktType || PKT_035E__0602==pktType) {
        /* 先形成列名行, 后跟列值行 */
        resPos = bufForColValuesmain;
        for (i=0; i<totalColInfoNum; i++) {
            if (i != totalColInfoNum-1) sprintf(resPos, "%s;:s", colInfos[i].colName);
            else sprintf(resPos, "%s;:n", colInfos[i].colName);
            resPos += (strlen(colInfos[i].colName) + 3);
        }
        strcpy(resPos, response_str);
    }

    /* 注: 当pl sql developer时:
           此处只判断11xx请求+1017回复组合的select, plsql developer等的请求回合中，第一次11xx+1017发送
           完后，还会紧跟着发送035e+1017包上面的回合重发一遍, 不要处理它，否则会把本次select 的后序内容
           断开掉, 导致重组失败.
           结束一个select回合的，有包尾部有ORA-01403串，或发现另一个11xx+1017的select请求, 或超时*/
    if ((6 == app_id && 0x11 == requestBuf[10] && 0x10==responseBuf[10] && 0x17==responseBuf[11] && CLI_PL_SQL_DEVELOPER==clientType)
    || (6 == app_id && 0x10==responseBuf[10] && 0x17==responseBuf[11] && CLI_PL_SQL_DEVELOPER!=clientType)) {
        logMsg("clean-select+1017\n");
        if (sharedm->count > 0) shmDelOneRecod(sharedm, hash, src_ip, src_port, dst_ip, dst_port);
        if (isLast) {
            logMsg("ruku1017-islast\n");

            /* select首回合处结束, 不写redis了 */
            writeDBfile(dir_id, capture_time, app_id, src_ip, src_port, src_mac, dst_ip, dst_port, dst_mac, interval_time, line_num, sql_str_main, bufForColValuesmain);
        } else {
            /* select, 列信息, 列名行, 列值行(有的话)缓存写来 */
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
        //当表有很多行时, select包和后面的plsql的请求下一页的包中间可能有其他包，
        //所以只有相同流的其他select回合，或老化链，最后一个返回包(ORA-01403)能结束一个select回合
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

/* 现在策略匹配改到writeFile()里了，此处不需要了 */
#if 0
    /* 策略匹配
       注: 策略匹配必须放到response解析完后, 因为response会改变app_id的值 */
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

    /* 写入库文件 */
    logMsg("ruku other(%d): request=%s\n respnose=%s\n", app_id, sql_str_main, response_str);
    writeDBfile(dir_id, capture_time, app_id, src_ip, src_port, src_mac, dst_ip, dst_port, dst_mac, interval_time, line_num, sql_str_main, response_str);
    return 0;
}

