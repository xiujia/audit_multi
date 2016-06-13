/*
2015-05-06:
重新写了个版本的audit_mongo_insert.c, 去掉用缓存保存，而直接用指针解决。
MongoDB数据库的接口封装
URI的格式(http://docs.mongodb.org/manual/reference/connection-string/)
mongodb://[username:password@]host1[:port1][,host2[:port2],...[,hostN[:portN]]][/[database][?options]]
gcc -o new_insert new_insert_multiple_threads.c $(pkg-config --cflags --libs libmongoc-1.0) -Wall -g -lpthread
./new_insert -D ./debug -f ./Sql_csp_1463973445971954_0 -d test -k
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <bson.h>
#include <bcon.h>
#include <mongoc.h>
#include <stdio.h>
#include <sched.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#define HOST "localhost"
#define POST "27017"
#define URI "mongodb://localhost:27017/"

#define METHOD_DEL 0
#define METHOD_REMAIN 1
#define METHOD_COPY 2
#define METHOD_MOVE 3

typedef struct ConfigureInfo{
    char dirHandle[64];    /* 要遍历的目录, 默认"/media/data/sql/" */
    char databaseName[64]; /* 要入库的mongo数据库名, 默认mongoDB */
    int debugOn;           /* 0关闭debug模式, 1开启debug模式, 默认0 */
    char debugFile[64];    /* debug信息文件 */
    FILE *fp;              /* debug文件句柄 */
    int cacheSecond;       /* cache的rowkey时间加上的秒数, 默认120 */
    int oracleSecond;      /* oracle的rowkey时间加上的秒数, 默认120 */
    int sqlserverSecond;   /* sqlserver的rowkey时间加上的秒数, 默认120 */
    int deamonOn;          /* 0前台运行, 1后台运行, 默认0 */
    char singleFile[64];   /* 处理一个文件时的文件名 */
    int cpuNO;             /* 要绑定的CPU号, 负数是不绑定,默认是0 */
    int handleMethod;      /* 对文件的处置, 0删除, 1保留, 2复制到指定目录, 3移动到指定目录. 默认0 */
    char toPath[64];       /* 当handleMethod取值2/3时, 目标目录 */
    int timing;            /* 0=关闭计时模式, 1=开启, 默认0 */
    int pthreadNum;        /* 线程个数[0-10],默认0 */
    char processDir[128];  /* 多线程时, 临时文件路径, 默认"/media/data/processDir/" */
} ConfigureInfo;

ConfigureInfo cfg = {
    "/data/audit/sql/",
    "mongoDB",
    0,
    "-",
    NULL,
    120,
    120,
    120,
    0,
    "",
    0,
    METHOD_DEL,
    "/home/debugInfo/",
    0,
    0,
    "/media/data/processDir/"
};

char output[1024*1024];
int logger( char *format, ...) {
    if (!cfg.debugOn) return 0;

    va_list ptr;
    va_start(ptr, format);
    vsnprintf(output, sizeof(output)-10, format, ptr);
    fprintf(cfg.fp, "%s", output);
    fflush(cfg.fp);
    return 0;
}

#define DEBUG_TIME_INIT() struct timeval timeval1, timeval2
#define DEBUG_TIME_START() gettimeofday(&timeval1, NULL)
#define DEBUG_TIME_STOP(msg) \
do {\
    gettimeofday(&timeval2, NULL);\
    long diff;\
    if (timeval1.tv_sec == timeval2.tv_sec) \
        diff = timeval2.tv_usec - timeval1.tv_usec;\
    else \
        diff = (timeval2.tv_sec - timeval1.tv_sec - 1)*1000000 + (1000000 - timeval1.tv_usec) + timeval2.tv_usec; \
    fprintf(stderr, "%s:diff = %lu\n", (msg), diff);\
    exit(0);\
}while(0)

#define HIS_TABLE_NAME_PREFIX        "web_monitor_data_"
#define CACHE_TABLE_NAME_PREFIX        "cache_monitor_data_"
#define CACHE_PORTAL_TELNET_PREFIX    "cache_portal_telnet_data_"
#define SQLSERVER_TABLE_NAME_PREFIX "sqlserver_monitor_data_"
#define ORACLE_TABLE_NAME_PREFIX    "oracle_monitor_data_"

#define APP_ID_HIS 1
#define APP_ID_congous 30
#define APP_ID_SUPERSERVER 2
#define APP_ID_TELNET 3
#define APP_ID_PORTAL 4
#define APP_ID_FTP 5
#define APP_ID_SQLSERVER_SELECT 6
#define APP_ID_SQLSERVER_UPDATE 7
#define APP_ID_SQLSERVER_DELETE 8
#define APP_ID_SQLSERVER_INSERT 9
#define APP_ID_SQLSERVER_CREATE 10
#define APP_ID_SQLSERVER_DROP 11
#define APP_ID_SQLSERVER_PRE_LOGIN 12
#define APP_ID_SQLSERVER_LOGIN 13
#define APP_ID_SQLSERVER_FAT 14
#define APP_ID_SQLSERVER_BULK_LOAD 15
#define APP_ID_SQLSERVER_RPC 16
#define APP_ID_SQLSERVER_ATTENTION 17
#define APP_ID_SQLSERVER_TMR 18
#define APP_ID_SQLSERVER_OTHER 19
#define APP_ID_SQLSERVER_SSPI 20
#define APP_ID_SQLSERVER_ALTER 21
#define APP_ID_SQLSERVER_USE 22
#define APP_ID_SQLSERVER_SET 23
#define APP_ID_SQLSERVER_GRANT 24
#define APP_ID_SQLSERVER_DENY 25
#define APP_ID_SQLSERVER_REVOKE 26
#define APP_ID_SQLSERVER_COMMIT 27
#define APP_ID_SQLSERVER_ROLLBACK 28
#define APP_ID_SQLSERVER_OTHER_SQL_STATE 29

#define DB_TYPE_CSP 100
#define DB_TYPE_STUDIO 20
#define DB_TYPE_SQLSERVER 10
#define DB_TYPE_ORACLE 30
#define DB_TYPE_TELNET 300

typedef struct {
    mongoc_client_t *client;
    mongoc_collection_t *collection;
    mongoc_cursor_t *cursor;
    bson_error_t error;
    mongoc_bulk_operation_t *bulk;
    mongoc_bulk_operation_t *bulk_PortalTelnet;
    mongoc_collection_t *collection_PortalTelnet;
} MONGO;

static int databaseType = -1;
char fileData[10*1024*1024];
char filenameForSig[100];

#define POOL_SIZE 1024000
typedef struct __JobQueue {
    char fname[POOL_SIZE][128];
}JobQueue;

/*线程池结构*/
#define THREAD_NUM_MAX 10
typedef struct __ThreadPool{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
    //pthread_cond_t queue_full_ready;
    JobQueue jqueue;
    int queue_head;
    int queue_tail;
    int shutdown; /*是否销毁线程池*/
    pthread_t threadid[THREAD_NUM_MAX];
    unsigned short threadNum; /*线程池中允许的活动线程数目*/
    unsigned int curSize; /*当前等待队列的任务数目*/
} ThreadPool;
ThreadPool *pool = NULL;

int mongoConnect(MONGO *mongo) {
    mongoc_init();
    mongo->client = mongoc_client_new (URI);
    if (NULL == mongo->client) { logger("%d:Failed to parse URI.\n", __LINE__); return -1; }
    return 0;
}

/*
   1, 格式: 前三个是rowkey,table,app_id,后面是name=value, 最后1个name=len,数据放最后.
      sqlserver,superserver的response放第2块rowkey=....|colfam1:response_content=len|colfam\n...\n
      但要注意, 这第2块的字段, 除rowkey外不能有和第1块相同的, 否则出现一个文档有重名的字段, 查询时会出错
   sqlserver的:
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
   "|colfam1:interval_time=%lu"
   "|colfam1:operation_command=%d"
   "|colfam\n"
   "....\n"
   "rowkey=%lu"
   "|colfam1:line_num=%d"
   "|colfam1:response_content=%d"
   "|colfam\n"
   "....\n"

   oracle的
   "rowkey=%lu|"
   "colfam1:table=%04d_%02d|"
   "colfam1:user_name=%s|"
   "colfam1:app_id=%d|"
   "colfam1:src_ip=%s|colfam1:dst_ip=%s|"
   "colfam1:src_mac=%s|colfam1:dst_mac=%s|"
   "colfam1:src_port=%u|colfam1:dst_port=%u|"
   "colfam1:interval_time=%lu|"
   "colfam1:line_num=%d|"
   #if GA_TEST
   "colfam1:security_level=%hu|"
   #endif
   "colfam1:operation_command=%s|"
   "colfam1:response_content=%d|colfam\n%s\n"

   ftp/telnet的:

   his的

   superserver的

   2, 放哪个表
   按app_id和数据库类型判断:
   his=1
   superserver=2
   portal=3
   telnet=4
   ftp=5
   IBM的http=30

   HIS放web_monitor_data_表
   portal/telent/ftp/superserver/自定义HTTP,放cache_monitor_data_表, cache_portal_telnet_data_表
   SQL Server放sqlserver_monitor_data_表
   Oracle放oracle_monitor_data_表

   3,字段类型:
   capture_time:INT64
   app_id: 字符串
   sub_app_id: 字符串
   src_ip: 字符串
   src_mac: 字符串
   src_port: 字符串
   dst_ip: 字符串
   dst_mac: 字符串
   dst_port: 字符串
   department: 字符串
   web_session: 字符串
   user_name: 字符串
   web_url: 字符串
   operation_command: 二进制
   web_content: 字符串
   operation_sql: 字符串
   user_id: 字符串
   response_content: 二进制
   tform: 字符串
   level_1: 字符串
   alarm_id: 字符串
   process_state: 字符串
   file_content: 二进制
   saveflag: 字符串
   charset: 字符串
   pass: 字符串
   line_num:INT64
   interval_time:INT64
   security_level:INT32
   update_time: INT64
*/
int parseFormatData(MONGO *conn, char *data, long datalen) {
    char *pos=NULL, *end=NULL, *name=NULL, *value=NULL, *table=NULL;
    char collectionName[256]={0}, *saveflag="";
    int len, app_id_val, isLastKeyValue;
    unsigned long long capture_time;
    bson_t *doc = NULL;
    bson_oid_t oid;

    pos = data;
    while ((pos = strstr(pos, "rowkey"))) {
        end = strstr(pos, "|colfam\n");
        if (!end) { logger("%d:failed to find end of rowkey line\n", __LINE__); return -1; }

        /* caputre_time */
        value = pos + strlen("rowkey=");
        pos = strchr(value, '|');
        if (!pos) { logger("%d:failed to find end of rowkey\n", __LINE__); return -1; }
        *pos = '\0';
        sscanf(value, "%llu", &capture_time);
        capture_time /= 1000;

        /* table, 用于生成collection名 */
        table = pos + strlen("|colfam1:table=");
        pos = strstr(table, "|colfam");
        if (!pos) { logger("%d:failed to find end of table\n", __LINE__); return -1; }
        *pos = '\0';

        /* 生成document */
        doc = bson_new ();
        bson_oid_init (&oid, NULL); /* '_id'自动必须是自动生成的，如果要改程序自己生成，那么程序重启后，计数会重置，会导致'_id'重复 当然可以在文件中保存该值*/
        BSON_APPEND_OID (doc, "_id", &oid);

        /* rowkey和table后,最后一个之前的每个字段都是nam=value结构, 最后一个字段是name=len|colfam\n...\n,要特殊处理 */
        isLastKeyValue = 0;
        while (pos < end) {
            name = pos + strlen("|colfam1:");
            value = strchr(name, '=');
            if (!value) { logger("%d:failed to find %s\n", __LINE__, name); return -1; }
            *value++ = '\0';

            pos = strstr(value, "|colfam");
            if (!pos) { logger("%d:failed to find end of %s's value\n", __LINE__, name); return -1; }
            *pos = '\0';
            logger("%d:name = %s, value = %s\n", __LINE__, name, value);

            /* 最后一个字段格式: name=len|colfam\n...\n */
            if ('\n' == pos[strlen("|colfam")]) {
                len = atoi(value);
                value = pos + strlen("|colfam\n");
                end += (strlen("|colfam\n") + len);
                *end++ = '\0'; /* 把\n设为\0 */
                logger("%d:name = %s, value = %s, reallen = %d\n", __LINE__, name, value, strlen(value));
                isLastKeyValue = 1;
            }

            /* operation_command:
            二进制：saveflag是206，207，208，209，300，1，2
            字符串：其他

            response_content:
            二进制：saveflag是206，207，208，209，300，1，2，app_id是3，database_type是oracle，sqlserver
            字符串：其他

            必须保证saveflag在他们前面*/
            if (!strcmp(name, "line_num")
             || !strcmp(name, "interval_time")) {
                BSON_APPEND_INT64 (doc, name,  atol(value));
            } else if (!strcmp(name, "security_level")) {
                BSON_APPEND_INT32 (doc, name,  atoi(value));
            } else if (!strcmp(name, "operation_command")) {
                if (!strcmp(saveflag, "206") || !strcmp(saveflag, "207")
                 || !strcmp(saveflag, "208") || !strcmp(saveflag, "209")
                 || !strcmp(saveflag, "300") || !strcmp(saveflag, "1")|| !strcmp(saveflag, "2")
                 || (DB_TYPE_ORACLE == databaseType)
                 || (DB_TYPE_SQLSERVER == databaseType)) {
                    if (!isLastKeyValue) len = strlen(value);
                    BSON_APPEND_BINARY (doc, name, BSON_SUBTYPE_BINARY, (uint8_t *)value, len);
                } else {
                    BSON_APPEND_UTF8 (doc, name, value);
                }
            } else if (!strcmp(name, "response_content")) {
                if (!strcmp(saveflag, "206") || !strcmp(saveflag, "207")
                 || !strcmp(saveflag, "208") || !strcmp(saveflag, "209")
                 || !strcmp(saveflag, "300") || (app_id_val == 3)
                 || !strcmp(saveflag, "1")|| !strcmp(saveflag, "2")
                 || (DB_TYPE_ORACLE == databaseType)
                 || (DB_TYPE_SQLSERVER == databaseType)) {
                    if (!isLastKeyValue) len = strlen(value);
                    BSON_APPEND_BINARY (doc, name, BSON_SUBTYPE_BINARY, (uint8_t *)value, len);
                } else {
                    BSON_APPEND_UTF8 (doc, name, value);
                }
            } else if (!strcmp(name, "file_content")) {
                if (!isLastKeyValue) len = strlen(value);
                BSON_APPEND_BINARY (doc, name, BSON_SUBTYPE_BINARY, (uint8_t *)value, len);
            } else {
                if (!strcmp(name, "app_id")) app_id_val = atoi(value);
                if (!strcmp(name, "saveflag")) saveflag = value;
                BSON_APPEND_UTF8 (doc, name, value);
            }

            if (isLastKeyValue) {
                /* sqlserver和cache superserver的格式是后跟第2块rowkey */
                if (DB_TYPE_SQLSERVER==databaseType || APP_ID_SUPERSERVER==app_id_val) {
                    pos = strstr(end, "rowkey=");
                    if (!pos) { logger("%d:failed to find second rowkey\n", __LINE__); return -1; }

                    pos = strstr(pos, "|colfam");
                    if (!pos) { logger("%d:failed to find end of second rowkey\n", __LINE__); return -1; }

                    end = strstr(pos, "|colfam\n");
                    if (!end) { logger("%d:failed to find end of seconde rowkey line\n", __LINE__); return -1; }

                    isLastKeyValue = 0;
                    while (pos < end) {
                        name = pos + strlen("|colfam1:");
                        value = strchr(name, '=');
                        if (!value) { logger("%d:failed to find %s\n", __LINE__, name); return -1; }
                        *value++ = '\0';

                        pos = strstr(value, "|colfam");
                        if (!pos) { logger("%d:failed to find end of %s's value\n", __LINE__, name); return -1; }
                        *pos = '\0';
                        logger("%d:name = %s, value = %s\n", __LINE__, name, value);

                        /* 最后一个字段格式: name=len|colfam\n...\n */
                        if ('\n' == pos[strlen("|colfam")]) {
                            len = atoi(value);
                            value = pos + strlen("|colfam\n");
                            end += (strlen("|colfam\n") + len);
                            *end++ = '\0'; /* 把\n设为\0 */
                            logger("%d:name = %s, value = %s, reallen = %d\n", __LINE__, name, value, strlen(value));
                            isLastKeyValue = 1;
                        }

                        /* operation_command:
                        二进制：saveflag是206，207，208，209，300，1，2
                        字符串：其他

                        response_content:
                        二进制：saveflag是206，207，208，209，300，1，2，app_id是3，database_type是oracle，sqlserver
                        字符串：其他 */
                        if (!strcmp(name, "line_num")
                         || !strcmp(name, "interval_time")) {
                            BSON_APPEND_INT64 (doc, name,  atol(value));
                        } else if (!strcmp(name, "security_level")) {
                            BSON_APPEND_INT32 (doc, name,  atoi(value));
                        } else if (!strcmp(name, "operation_command")) {
                            if (!strcmp(saveflag, "206") || !strcmp(saveflag, "207")
                             || !strcmp(saveflag, "208") || !strcmp(saveflag, "209")
                             || !strcmp(saveflag, "300") || !strcmp(saveflag, "1")|| !strcmp(saveflag, "2")) {
                                if (!isLastKeyValue) len = strlen(value);
                                BSON_APPEND_BINARY (doc, name, BSON_SUBTYPE_BINARY, (uint8_t *)value, len);
                            } else {
                                BSON_APPEND_UTF8 (doc, name, value);
                            }
                        } else if (!strcmp(name, "response_content")) {
                            if (!strcmp(saveflag, "206") || !strcmp(saveflag, "207")
                             || !strcmp(saveflag, "208") || !strcmp(saveflag, "209")
                             || !strcmp(saveflag, "300") || (app_id_val == 3)
                             || !strcmp(saveflag, "1")|| !strcmp(saveflag, "2")
                             || (DB_TYPE_ORACLE == databaseType)
                             || (DB_TYPE_SQLSERVER == databaseType)) {
                                if (!isLastKeyValue) len = strlen(value);
                                BSON_APPEND_BINARY (doc, name, BSON_SUBTYPE_BINARY, (uint8_t *)value, len);
                            } else {
                                BSON_APPEND_UTF8 (doc, name, value);
                            }
                        } else if (!strcmp(name, "file_content")) {
                            if (!isLastKeyValue) len = strlen(value);
                            BSON_APPEND_BINARY (doc, name, BSON_SUBTYPE_BINARY, (uint8_t *)value, len);
                        } else {
                            if (!strcmp(name, "app_id")) app_id_val = atoi(value);
                            if (!strcmp(name, "saveflag")) saveflag = value;
                            BSON_APPEND_UTF8 (doc, name, value);
                        }
                        if (isLastKeyValue) break;
                    }
                }
                break;
            }
        }
        pos = end;

        unsigned long update_time;
        struct timeval tp;
        gettimeofday(&tp, NULL);
        update_time = (unsigned long)tp.tv_sec * 1000 + tp.tv_usec/1000;
        BSON_APPEND_INT64 (doc, "update_time",  update_time);

        logger("%d:caputre_time=%lu(before add extra value)\n", __LINE__, capture_time);
        /* 生成collection名, 连接集合 */
        if (1 == app_id_val) {
            /* cache(his) */
            sprintf(collectionName, "%s%s", HIS_TABLE_NAME_PREFIX, table);
            capture_time += (cfg.cacheSecond*1000);
        } else if ((app_id_val<6 && app_id_val>1) || 30==app_id_val) {
            /* cache(portal/telent/ftp/superserver/自定义HTTP) */
            sprintf(collectionName, "%s%s", CACHE_TABLE_NAME_PREFIX, table);
            capture_time += (cfg.cacheSecond*1000);
        } else if(app_id_val >= 6 && DB_TYPE_SQLSERVER==databaseType){
             /* SQL Server */
            sprintf(collectionName, "%s%s", SQLSERVER_TABLE_NAME_PREFIX, table);
             capture_time += (cfg.sqlserverSecond*1000);
        } else if(app_id_val >= 6 && DB_TYPE_ORACLE==databaseType){
             /* Oracle */
            sprintf(collectionName, "%s%s", ORACLE_TABLE_NAME_PREFIX, table);
             capture_time += (cfg.oracleSecond*1000);
        }
        logger("%d:collectionName=%s\n", __LINE__, collectionName);
        logger("%d:caputre_time=%lu(after add extra value)\n", __LINE__, capture_time);
        BSON_APPEND_INT64 (doc, "capture_time",  capture_time);

        conn->collection = mongoc_client_get_collection (conn->client, cfg.databaseName, collectionName);
        conn->bulk = mongoc_collection_create_bulk_operation (conn->collection, 1, NULL);
        /* Cache的Portal/telnet再额外插一次到缓存表 */
        if((app_id_val<6 && app_id_val>2) || 30==app_id_val){
            sprintf(collectionName, "%s%s", CACHE_PORTAL_TELNET_PREFIX, table);
            conn->collection_PortalTelnet = mongoc_client_get_collection (conn->client, cfg.databaseName, collectionName);
            conn->bulk_PortalTelnet = mongoc_collection_create_bulk_operation (conn->collection_PortalTelnet, 1, NULL);
        }

        /* 缓存1个文档 */
        mongoc_bulk_operation_insert (conn->bulk, doc);
        if((app_id_val<6 && app_id_val>2) || 30==app_id_val){
            mongoc_bulk_operation_insert (conn->bulk_PortalTelnet, doc);
        }
        bson_destroy (doc);
    }

    if (!doc) {logger("data format error!\n"); return -1;}
    /* 入库 */
    bson_t reply;
    bson_error_t error;
    int ret = mongoc_bulk_operation_execute (conn->bulk, &reply, &error);
    if (!ret) { logger("%d:Error: %s\n", __LINE__, error.message); return -1;}

    bson_destroy (&reply);
    mongoc_bulk_operation_destroy (conn->bulk);
    mongoc_collection_destroy(conn->collection);
    if((app_id_val<6 && app_id_val>2) || 30==app_id_val){
        int ret = mongoc_bulk_operation_execute (conn->bulk_PortalTelnet, &reply, &error);
        if (!ret) { logger("%d:Error: %s\n", __LINE__, error.message); }
        bson_destroy (&reply);
        mongoc_bulk_operation_destroy (conn->bulk_PortalTelnet);
        mongoc_collection_destroy(conn->collection_PortalTelnet);
    }
    return 0;
}

int NC_daemon_audit(void) {
    pid_t pid;
    int ret;
    if ((pid = fork()) < 0) {
        return -1;
    } else if (pid != 0) {
        exit(0); /* parent goes bye-bye */
    }
    if((ret=setsid()) < 0) { /* become session leader */
        printf("unable to setsid.\n");
    }
     setpgrp();
     return 0;
}

/* read()的封装。成功返回读取的字节数，失败返回-1 */
ssize_t readBytes(int fd, void *buf, size_t n) {
    char *pos = buf;
    ssize_t nrd, nLeft=n;

    while (nLeft > 0) {
        if ( (nrd = read(fd, pos, nLeft)) < 0) {
            if (EINTR == errno) {
                nrd = 0;  /* 再次调用read */
            } else {
                logger("%d:read() failed\n", __LINE__);
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
    if (fd < 0) { logger("%d:fails to open the file\n", __LINE__); return -1; }
    if ((nrd = readBytes(fd, pos, n)) < 0) { logger("%d:read failed\n", __LINE__); return -1; }
    close(fd);
    return nrd;
}

/* 处理单个文件 */
int handleSingleFile(char *filePath) {
    char cmd[512];
    int retval;
    MONGO conn;
    DEBUG_TIME_INIT();
    if (cfg.timing) DEBUG_TIME_START();

    if (strstr(filePath, "Sql_csp_")) {
        databaseType = DB_TYPE_CSP;
    } else if (strstr(filePath, "Sql_studio_")) {
        databaseType = DB_TYPE_STUDIO;
    } else if (strstr(filePath, "Sql_sqlserver_")) {
        databaseType = DB_TYPE_SQLSERVER;
    } else if (strstr(filePath, "Sql_oracle_")) {
        databaseType = DB_TYPE_ORACLE;
    } else if (strstr(filePath, "Sql_terminal_")) {
        databaseType = DB_TYPE_TELNET;
    } else {
        logger("%d:wrong file name", __LINE__);
        exit(-1);
    }

    retval = mongoConnect(&conn);
    if (retval < 0) {
        return -1;
    }

    retval = readFile(filePath, fileData, sizeof(fileData)-1);
    switch(cfg.handleMethod) {
        case METHOD_DEL:
            unlink(filePath);
            break;
        case METHOD_REMAIN:
            break;
        case METHOD_COPY:
            sprintf(cmd, "cp %s %s", filePath, cfg.toPath);
            system(cmd);
            break;
        case METHOD_MOVE:
            sprintf(cmd, "mv %s %s", filePath, cfg.toPath);
            system(cmd);
            break;
    }
    if (retval <= 0) return 0;

    parseFormatData(&conn, fileData, retval);
    if (conn.client) mongoc_client_destroy (conn.client);
    if (cfg.timing) DEBUG_TIME_STOP("file");

    return 0;
}

void *thread_routine (void *arg) {
    char fname[128], fpath[128], cmd[512];
    int retval;
    MONGO conn;

    retval = mongoConnect(&conn);
    if (retval < 0) {logger("%d:fail to connect to mongo\n", __LINE__); pthread_exit (NULL); }

    while (1) {
        pthread_mutex_lock (&(pool->queue_lock));
        while (pool->curSize == 0 && !pool->shutdown) {
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock)); /* 解文件线程在queue_ready上等 */
        }

        if (pool->curSize == 0 && pool->shutdown) {
            pthread_mutex_unlock (&(pool->queue_lock));
            pthread_exit (NULL);
        }

        if (pool->curSize <= 0) {
            printf("%d:pool->curSize <= 0", __LINE__);
            exit(-1);
        }

        pool->curSize--;
        strcpy(fname, pool->jqueue.fname[pool->queue_head]);
        pool->queue_head = (pool->queue_head == POOL_SIZE-1) ? (0) : (pool->queue_head+1);
        pthread_mutex_unlock (&(pool->queue_lock));
        pthread_cond_signal (&(pool->queue_ready));

        if (!memcmp(fname, "Sql_csp_", strlen("Sql_csp_"))) {
            databaseType = DB_TYPE_CSP;
        } else if (!memcmp(fname, "Sql_studio_", strlen("Sql_studio_"))) {
            databaseType = DB_TYPE_STUDIO;
        } else if (!memcmp(fname, "Sql_sqlserver_", strlen("Sql_sqlserver_"))) {
            databaseType = DB_TYPE_SQLSERVER;
        } else if (!memcmp(fname, "Sql_oracle_", strlen("Sql_oracle_"))) {
            databaseType = DB_TYPE_ORACLE;
        } else if (!memcmp(fname, "Sql_terminal_", strlen("Sql_terminal_"))) {
            databaseType = DB_TYPE_TELNET;
        }

        sprintf(fpath, "%s%s", cfg.processDir, fname);
        retval = readFile(fpath, fileData, sizeof(fileData)-10);
        switch(cfg.handleMethod) {
            case METHOD_DEL:
                unlink(fpath);
                break;
            case METHOD_REMAIN:
                break;
            case METHOD_COPY:
                sprintf(cmd, "cp %s %s", fpath, cfg.toPath);
                system(cmd);
                break;
            case METHOD_MOVE:
                sprintf(cmd, "mv %s %s", fpath, cfg.toPath);
                system(cmd);
                break;
        }
        if (retval <= 0) continue;
        fileData[retval] = '\0';
        if (parseFormatData(&conn, fileData, retval) < 0) {
            logger("%d:%d:\nfiledata = %s\n", __LINE__, fileData);
        }
    }
    if (conn.client) mongoc_client_destroy (conn.client);
    pthread_exit (NULL);
    return NULL;
}

int poolInit (int size) {
    int i;
    if(size>10 || size<1){ printf("only 1-10 threads!\n"); exit(-1); }
    pool = (ThreadPool *)malloc (sizeof (ThreadPool));
    if(!pool){ printf("malloc pool failed!\n");  exit(-1); }
    pthread_mutex_init (&(pool->queue_lock), NULL);
    pthread_cond_init (&(pool->queue_ready), NULL);

    pool->queue_head = pool->queue_tail = 0;
    pool->threadNum = size;
    pool->curSize = 0;
    pool->shutdown = 0;
    for (i = 0; i < size; i++) {
         pthread_create (&(pool->threadid[i]), NULL, thread_routine, NULL);
    }
    return 0;
}

void enqueue(char *fname) {
    pthread_mutex_lock (&(pool->queue_lock));
    while (pool->curSize >= POOL_SIZE && !pool->shutdown) {
        pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock)); /* 等解文件线程唤醒 */
    }

    if (pool->curSize >= POOL_SIZE) {
        printf("%d:pool->curSize >= POOL_SIZE", __LINE__);
        exit(-1);
    }

    pool->curSize++;
    strcpy(pool->jqueue.fname[pool->queue_tail], fname);
    pool->queue_tail = (pool->queue_tail == POOL_SIZE-1) ? (0) : (pool->queue_tail+1);
    pthread_mutex_unlock (&(pool->queue_lock));
    pthread_cond_signal (&(pool->queue_ready));  /* 唤醒任意一个解文件线程 */
}

int poolDestroy () {
    int i;
    if (pool->shutdown) return -1;/*防止两次调用*/
    pool->shutdown = 1;

    /*唤醒所有等待线程，线程池要销毁了*/
    pthread_cond_broadcast (&(pool->queue_ready));

    /*阻塞等待线程退出，否则就成僵尸了*/
    for (i = 0; i < pool->threadNum; i++)
        pthread_join (pool->threadid[i], NULL);

    /*条件变量和互斥量也别忘了销毁*/
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    free (pool);
    pool=NULL;
    return 0;
}

//只有3不能绑定，1U的是4个核0123，2U是8个核01234567
int BindCpu(int toBind){
    cpu_set_t mask;
    CPU_ZERO(&mask);    //置空
    #if U2_DEV
        toBind %= 8;
        if(toBind == 3){
            toBind++;
        }
    #else
        if(toBind == 3){
            toBind--;
        }
    #endif

       CPU_SET(toBind,&mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) { //设置线程CPU亲和力
        //printf("warning: could not set CPU affinity!\n");
        return -1;
    }
    //printf("set CPU affinity success!\n");
    return 0;
}


/*
后缀参数设计:
1. -deamon: 后台运行, 默认前台运行
2. -t n: rowkey时间后延n秒钟, 包括所有的数据库, 若有-D0 n/-D1 n...指定特定数据库, 则按指定的来
   -t0 n: cache的rowkey时间后延n秒钟
   -t1 n: sqlserver
   -t2 n: oracle
   默认所有数据库的rowkey时间后延120秒钟
3. -D file: 开启debug模式, 默认关闭.该模式下打印debug信息, 仍删除文件, 若不删除则用-m选项. debug信息保存到file, 若file是"-",则输出到stdout
4. -f filename: 指定要处理的文件, 不指定则默认遍历目录'dirHandle'下所有文件. 它会取消遍历目录的动作.
5. -p path: 指定要遍历的目录, 默认遍历"/data/audit/sql/". 有-f时此选项无效.
6. -d database_name: 指定数据库名, 默认是"mongoDB"
7. -b n: 绑定第n个CPU. 默认是0
8. -h: 打印帮助信息, 直接退出
9. -k: 保留文件不删除
10. -c path: 把文件复制到指定路径下.
11. -m path: 保留文件并把文件移动到指定路径下
12. -M n: n=[0-10],指开启几个线程
*/

char helpStr[] =
"\n***********************************************************\n"
"USAGE:\n"
"audit_mongo_insert [-deamon] [-t n] [-t0 n] [-t1 n] [-t2 n] [-D]\n"
"                   [-f filename | -p path] [-d database_name]\n"
"                   [-b n] [-h] [-k | -c path | -m path]\n"
"--deamon: set daemon mode(default: foreground)\n"
"-t n: rowkey time add n*1000, it must before -t0/-t1/-t2(default: 120)\n"
"-t0 n: cache rowkey time add n*1000\n"
"-t1 n: sqlserver rowkey time add n*1000\n"
"-t2 n: oracle rowkey time add n*1000\n"
"-D file: set debug mode, output the debug message into file(\"-\" means stdout)(default: closed)\n"
"-f filename | -p path: specify a single file or a dir to handle(default: -p /data/audit/sql/)\n"
"-d database_name: specify the database's name(default: mongoDB)\n"
"-b n: specify which CPU to bind(default: 0)\n"
"-h: output the help message, and exit immediately\n"
"-k: keep the file(default: delete)\n"
"-c path: copy the file to 'path'\n"
"-m path: move the file to 'path'\n"
"-T: set timing mode, output the time interval of processing a file or a dir\n"
"-M n: set thread number[0-10]\n"
"***********************************************************\n";

void parseArg(int argc,char ** argv) {
    int i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "--deamon")) {
            cfg.deamonOn = 1;
            i++;
        } else if (!strcmp(argv[i], "-t")) {
            cfg.cacheSecond = cfg.sqlserverSecond = cfg.oracleSecond = atoi(argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-t0")) {
            cfg.cacheSecond = atoi(argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-t1")) {
            cfg.sqlserverSecond = atoi(argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-t2")) {
            cfg.oracleSecond = atoi(argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-D")) {
            cfg.debugOn = 1;
            strcpy(cfg.debugFile, argv[i+1]);
            if (!strcmp(cfg.debugFile, "-")) {
                cfg.fp = stdout;
            } else {
                cfg.fp = fopen(cfg.debugFile, "a+");
                if (!(cfg.fp)) { cfg.fp = stdout; }
            }
            i += 2;
        } else if (!strcmp(argv[i], "-f")) {
            strcpy(cfg.singleFile, argv[i+1]);
            int fd = open(cfg.singleFile, O_RDONLY);
            if (fd < 0){ printf("open %s failed!\n", cfg.singleFile); exit(-1); }
            close(fd);
            i += 2;
        } else if (!strcmp(argv[i], "-p")) {
            strcpy(cfg.dirHandle, argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-d")) {
            strcpy(cfg.databaseName, argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-b")) {
            cfg.cpuNO = atoi(argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-h")) {
            printf("%s", helpStr);
            exit(0);
        } else if (!strcmp(argv[i], "-k")) {
            cfg.handleMethod = 1;
            i++;
        } else if (!strcmp(argv[i], "-c")) {
            cfg.handleMethod = 2;
            strcpy(cfg.toPath, argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-m")) {
            cfg.handleMethod = 3;
            strcpy(cfg.toPath, argv[i+1]);
            i += 2;
        } else if (!strcmp(argv[i], "-T")) {
            cfg.timing = 1;
            i += 1;
        } else if (!strcmp(argv[i], "-M")) {
            cfg.pthreadNum = atoi(argv[i+1]);
            if (cfg.pthreadNum<0 || cfg.pthreadNum>10) {
                printf("-M n: n must be [0-10]\n");
                printf("%s", helpStr);
                exit(0);
            }
            i += 2;
        } else {
            printf("%s", helpStr);
            exit(0);
        }
    }

    logger("%d:dirHandle=%s, databaseName=%s, debugOn=%d, debugFile=%s, cacheSecond=%d, "
           "oracleSecond=%d, sqlserverSecond=%d, deamonOn=%d, singleFile=%s, cpuNO=%d, handleMethod=%d, toPath=%s\n", __LINE__,
           cfg.dirHandle, cfg.databaseName, cfg.debugOn, cfg.debugFile, cfg.cacheSecond,
           cfg.oracleSecond, cfg.sqlserverSecond, cfg.deamonOn, cfg.singleFile, cfg.cpuNO, cfg.handleMethod, cfg.toPath);
    return;
}

void sigHandler(int sig) {
    switch(sig) {
        case SIGINT:
            printf("SIGINT-%s\n", filenameForSig); break;
        case SIGTERM:
            printf("SIGTERM-%s\n", filenameForSig); break;
        case SIGSEGV:
            printf("SIGSEGV-%s\n", filenameForSig); break;
        case SIGBUS:
            printf("SIGBUS-%s\n", filenameForSig); break;
    }
    exit(-1);
}

int main(int argc, char **argv) {
    char filePath[512], cmd[512], oldname[100], newname[100];
    int retval;
    DIR *dp;
    struct dirent *dirp;
    MONGO conn;
    DEBUG_TIME_INIT();

    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGSEGV, sigHandler);

    parseArg(argc, argv);
    if (cfg.deamonOn) NC_daemon_audit();
    if (cfg.cpuNO>=0) BindCpu(cfg.cpuNO);
    if (cfg.singleFile[0] != '\0') return handleSingleFile(cfg.singleFile);

    /* 万一程序挂掉, 重启后遍历移动后的目录, 把上次没处理完的处理下 */
    dp = opendir(cfg.processDir);
    if(dp) {
        retval = mongoConnect(&conn);
        if (retval < 0) {logger("%d:fail to connect to mongo\n", __LINE__); pthread_exit (NULL); }
        while((dirp=readdir(dp))) {
            if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
                continue;
            }
            if (!memcmp(dirp->d_name, "Sql_csp_", strlen("Sql_csp_"))) {
                databaseType = DB_TYPE_CSP;
            } else if (!memcmp(dirp->d_name, "Sql_studio_", strlen("Sql_studio_"))) {
                databaseType = DB_TYPE_STUDIO;
            } else if (!memcmp(dirp->d_name, "Sql_sqlserver_", strlen("Sql_sqlserver_"))) {
                databaseType = DB_TYPE_SQLSERVER;
            } else if (!memcmp(dirp->d_name, "Sql_oracle_", strlen("Sql_oracle_"))) {
                databaseType = DB_TYPE_ORACLE;
            } else if (!memcmp(dirp->d_name, "Sql_terminal_", strlen("Sql_terminal_"))) {
                databaseType = DB_TYPE_TELNET;
            }

            sprintf(filePath, "%s%s", cfg.dirHandle, dirp->d_name);
            retval = readFile(filePath, fileData, sizeof(fileData)-10);
            switch(cfg.handleMethod) {
                case METHOD_DEL:
                    unlink(filePath);
                    break;
                case METHOD_REMAIN:
                    break;
                case METHOD_COPY:
                    sprintf(cmd, "cp %s %s", filePath, cfg.toPath);
                    system(cmd);
                    break;
                case METHOD_MOVE:
                    sprintf(cmd, "mv %s %s", filePath, cfg.toPath);
                    system(cmd);
                    break;
            }
            if (retval <= 0) continue;
            fileData[retval] = '\0';
            if (parseFormatData(&conn, fileData, retval) < 0) {
                logger("%d:%d:\nfiledata = %s\n", __LINE__, fileData);
            }
        }
        closedir(dp);
        if (conn.client) mongoc_client_destroy (conn.client);
    }

    if (cfg.pthreadNum > 0) {
        sprintf(cmd, "cp -f %s/* %s", cfg.processDir, cfg.dirHandle);
        system(cmd);
        sprintf(cmd, "mkdir -p %s", cfg.processDir);
        system(cmd);
        poolInit(cfg.pthreadNum);
    }else {
        retval = mongoConnect(&conn);
        if (retval < 0) {logger("%d:fail to connect to mongo\n", __LINE__); pthread_exit (NULL); }
    }

    while (1) {
        if (cfg.timing) DEBUG_TIME_START();

        dp = opendir(cfg.dirHandle);
        if(!dp) { printf("Open dir failed"); return -1; }
        while((dirp=readdir(dp))) {
            if(!strcmp(dirp->d_name,".") || !strcmp(dirp->d_name,"..")){
                continue;
            }

            if (cfg.pthreadNum > 0) {
                sprintf(oldname, "%s%s", cfg.dirHandle, dirp->d_name);
                sprintf(newname, "%s%s", cfg.processDir, dirp->d_name);
                rename(oldname, newname);
                enqueue(dirp->d_name);
            } else {
                strcpy(filenameForSig, dirp->d_name);
                logger("****************%s*****************************\n", filenameForSig);

                if (!memcmp(dirp->d_name, "Sql_csp_", strlen("Sql_csp_"))) {
                    databaseType = DB_TYPE_CSP;
                } else if (!memcmp(dirp->d_name, "Sql_studio_", strlen("Sql_studio_"))) {
                    databaseType = DB_TYPE_STUDIO;
                } else if (!memcmp(dirp->d_name, "Sql_sqlserver_", strlen("Sql_sqlserver_"))) {
                    databaseType = DB_TYPE_SQLSERVER;
                } else if (!memcmp(dirp->d_name, "Sql_oracle_", strlen("Sql_oracle_"))) {
                    databaseType = DB_TYPE_ORACLE;
                } else if (!memcmp(dirp->d_name, "Sql_terminal_", strlen("Sql_terminal_"))) {
                    databaseType = DB_TYPE_TELNET;
                }

                sprintf(filePath, "%s%s", cfg.dirHandle, dirp->d_name);
                retval = readFile(filePath, fileData, sizeof(fileData)-10);
                switch(cfg.handleMethod) {
                    case METHOD_DEL:
                        unlink(filePath);
                        break;
                    case METHOD_REMAIN:
                        break;
                    case METHOD_COPY:
                        sprintf(cmd, "cp %s %s", filePath, cfg.toPath);
                        system(cmd);
                        break;
                    case METHOD_MOVE:
                        sprintf(cmd, "mv %s %s", filePath, cfg.toPath);
                        system(cmd);
                        break;
                }
                if (retval <= 0) continue;
                fileData[retval] = '\0';
                if (parseFormatData(&conn, fileData, retval) < 0) {
                    logger("%d:%d:\nfiledata = %s\n", __LINE__, fileData);
                }
            }
        }
        closedir(dp);
        if (cfg.timing) {
            if (cfg.pthreadNum > 0) {
                poolDestroy();
            }
            DEBUG_TIME_STOP("dir");
        }
    }
    if (cfg.pthreadNum > 0) {
        poolDestroy();
    }else {
        if (conn.client) mongoc_client_destroy (conn.client);
    }

    return 0;
}


