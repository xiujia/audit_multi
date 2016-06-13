/*
2015-12-15:
1. ��sqlserver������ͻظ��ֶθ�Ϊ��������ʽ
2. ����security_level�ֶΣ�������pairParse()�У���mongoInsertClassification(), struct keyValues�м������

2015-12-22:
1. �޸���getKeyValues()���sqlserver���ֵ�ʱ��+2���ӵ�����λ�ã�Ҫ�ŵ���󣬷���ᱻ����

2015-12-25:
1.�¼�update_time�ֶ�

2016-01-20:
�帱��ʱ����superserverҲ���ȥ�ˣ��Ĺ�����ֻ����portal/telnet/ftp/IBM http
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
#include "audit_release.h"
/* MongoDB���ݿ�Ľӿڷ�װ
   URI�ĸ�ʽ(http://docs.mongodb.org/manual/reference/connection-string/)
   mongodb://[username:password@]host1[:port1][,host2[:port2],...[,hostN[:portN]]][/[database][?options]]
   gcc -o audit_mongo_insert audit_mongo_insert.c $(pkg-config --cflags --libs libmongoc-1.0) -Wall -g
*/

#define HOST "localhost"
#define POST "27017"
#define URI "mongodb://localhost:27017/"
#define DB_NAME "mongoDB"
#define DIR_FOR_DB "/data/audit/sql/"

#define DEBUG_OPEN 0
#if DEBUG_OPEN
#define DEBUG_LOG(s) fprintf(stderr, "DEBUG : <%s> : %d : %s\n", __FILE__, __LINE__, (s))
#else
#define DEBUG_LOG(s)
#endif
#define PRINT_ERR_MSG(s) fprintf(stderr, "<%s> : %d : %s\n", __FILE__, __LINE__, (s))

#define HIS_TABLE_NAME_PREFIX		"web_monitor_data_"
#define CACHE_TABLE_NAME_PREFIX		"cache_monitor_data_"
#define CACHE_PORTAL_TELNET_PREFIX	"cache_portal_telnet_data_"
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

struct keyValues {
    char table[256];
    unsigned long capture_time; //ʱ��ĺ��뼶����
    char app_id[32];
    char sub_app_id[32];
    char src_ip[32];
    char src_mac[32];
    char src_port[32];
    char dst_ip[32];
    char dst_mac[32];
    char dst_port[32];
    char department[1024];
    char web_session[1024];
    char user_name[1024];
    char web_url[5000];
    char operation_command[1024*1024*10];
    char web_content[5000];
    char operation_sql[1024];
    char user_id[32];
    char response_content[1024*1024*10];
    char tform[1024];
    char level_1[1024];
    char alarm_id[1024];
    char process_state[1024];
    char file_content[1024*1024*2];
    char saveflag[8];
    char charset[32];

    long requestLen;
    long responseLen;
    long file_content_len;

    char pass[526];/* 2015-11-09:�����ֶ� */
    unsigned long  line_num;/* 2015-11-09:�����ֶ� */
    unsigned long  interval_time; /* 2015-11-09:�����ֶ� */
    int security_level; /* 2015-12-15:�����ֶ� */
    char request_sql[1024]; /* 2016-05-08�����ֶ� */
};

struct keyValues KVs;
static int database_type = -1;
char fileContent[10*1024*1024];
char line[1024*1024];
char pair[1024*1024];
char filename[1024];

void printKVs(int lineNum) {
#if DEBUG_OPEN
    fprintf(stderr, "****************************************************************************\n"
            "DEBUG : <%s> : %d : line = %s\n"
            "table=%s, capture_time=%lu, app_id=%s,\n"
            "src_ip=%s, src_mac=%s, src_port=%s, dst_ip=%s, dst_mac=%s, dst_port=%s,\n"
            "department=%s, web_session=%s, user_name=%s, web_url=%s,\n"
            "operation_command=%s, web_content=%s, operation_sql=%s, user_id=%s,\n"
            "response_content=%s, tform=%s, level_1=%s, alarm_id=%s, process_state=%s,\n"
            "file_content=%s, saveflag=%s, charset=%s\n",
            __FILE__, lineNum, line,
            KVs.table, KVs.capture_time, KVs.app_id,
            KVs.src_ip, KVs.src_mac, KVs.src_port, KVs.dst_ip, KVs.dst_mac, KVs.dst_port,
            KVs.department, KVs.web_session, KVs.user_name, KVs.web_url,
            KVs.operation_command, KVs.web_content, KVs.operation_sql, KVs.user_id,
            KVs.response_content, KVs.tform, KVs.level_1, KVs.alarm_id, KVs.process_state,
            KVs.file_content, KVs.saveflag, KVs.charset);
#endif
}

int mongoConnect(MONGO *mongo) {
    mongoc_init ();
    mongo->client = mongoc_client_new (URI);
    if (NULL == mongo->client) {
      PRINT_ERR_MSG("Failed to parse URI.\n");
      return -1;
    }
    return 0;
}

int mongoUseCollection(MONGO *mongo, char *dbName, char *collectionName) {
    mongo->collection = mongoc_client_get_collection (mongo->client, dbName, collectionName);
    return 0;
}

int mongoMultipleInsert(MONGO *mongo, bson_t *doc) {
    bson_error_t error;
    if (!mongoc_collection_insert (mongo->collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
        printf ("%s\n", error.message);
        exit(-1);
    }
    return 0;
}

int mongoClose(MONGO *mongo) {
	if (mongo->collection) mongoc_collection_destroy (mongo->collection);
	if (mongo->client) mongoc_client_destroy (mongo->client);
	return 0;
}

void mongoInsertClassification(MONGO *conn, struct keyValues *kvs, int type) {
    bson_t *doc;
    bson_oid_t oid;

    printKVs(__LINE__);

    doc = bson_new ();
    /* '_id'�Զ��������Զ����ɵģ����Ҫ�ĳ����Լ����ɣ���ô���������󣬼��������ã��ᵼ��'_id'�ظ�
       ��Ȼ�������ļ��б����ֵ*/
    bson_oid_init (&oid, NULL);
    BSON_APPEND_OID (doc, "_id", &oid);
    BSON_APPEND_INT64 (doc, "capture_time",  kvs->capture_time);
    BSON_APPEND_UTF8 (doc, "app_id",  kvs->app_id);
    BSON_APPEND_UTF8 (doc, "sub_app_id",  kvs->sub_app_id);
    BSON_APPEND_UTF8 (doc, "src_ip",  kvs->src_ip);
    BSON_APPEND_UTF8 (doc, "src_mac",  kvs->src_mac);
    BSON_APPEND_UTF8 (doc, "src_port",  kvs->src_port);
    BSON_APPEND_UTF8 (doc, "dst_ip",  kvs->dst_ip);
    BSON_APPEND_UTF8 (doc, "dst_mac",  kvs->dst_mac);
    BSON_APPEND_UTF8 (doc, "dst_port",  kvs->dst_port);
    BSON_APPEND_UTF8 (doc, "department",  kvs->department);
    BSON_APPEND_UTF8 (doc, "web_session",  kvs->web_session);
    BSON_APPEND_UTF8 (doc, "user_name",  kvs->user_name);
    BSON_APPEND_UTF8 (doc, "web_url",  kvs->web_url);
    if (!strcmp(kvs->saveflag, "206") || !strcmp(kvs->saveflag, "207")
     || !strcmp(kvs->saveflag, "208") || !strcmp(kvs->saveflag, "209")
     || !strcmp(kvs->saveflag, "300") || !strcmp(kvs->saveflag, "1")
     || !strcmp(kvs->saveflag, "2")
     || (DB_TYPE_ORACLE == database_type)
     || (DB_TYPE_SQLSERVER == database_type)) {
        BSON_APPEND_BINARY (doc, "operation_command", BSON_SUBTYPE_BINARY, (uint8_t *)(kvs->operation_command), kvs->requestLen);
    } else {
        BSON_APPEND_UTF8 (doc, "operation_command",  kvs->operation_command);
    }
    //BSON_APPEND_BINARY (doc, "operation_command", BSON_SUBTYPE_BINARY, (uint8_t *)(kvs->operation_command), strlen(kvs->operation_command));
    //BSON_APPEND_UTF8 (doc, "operation_command",  kvs->operation_command);
    BSON_APPEND_UTF8 (doc, "web_content",  kvs->web_content);
    BSON_APPEND_UTF8 (doc, "operation_sql",  kvs->operation_sql);
    BSON_APPEND_UTF8 (doc, "user_id",  kvs->user_id);
    if (!strcmp(kvs->saveflag, "206") || !strcmp(kvs->saveflag, "207")
     || !strcmp(kvs->saveflag, "208") || !strcmp(kvs->saveflag, "209")
     || !strcmp(kvs->saveflag, "300") || !strcmp(kvs->app_id, "3")
     || !strcmp(kvs->saveflag, "1")|| !strcmp(kvs->saveflag, "2")
     || (DB_TYPE_ORACLE == database_type)
     || (DB_TYPE_SQLSERVER == database_type)) {
        BSON_APPEND_BINARY (doc, "response_content", BSON_SUBTYPE_BINARY, (uint8_t *)(kvs->response_content), kvs->responseLen);
    } else {
        BSON_APPEND_UTF8 (doc, "response_content", kvs->response_content);
    }
    //BSON_APPEND_BINARY (doc, "response_content", BSON_SUBTYPE_BINARY, (uint8_t *)(kvs->response_content), strlen(kvs->response_content));
    //BSON_APPEND_UTF8 (doc, "response_content", kvs->response_content);
    BSON_APPEND_UTF8 (doc, "tform",  kvs->tform);
    BSON_APPEND_UTF8 (doc, "level_1",  kvs->level_1);
    BSON_APPEND_UTF8 (doc, "alarm_id",  kvs->alarm_id);
    BSON_APPEND_UTF8 (doc, "process_state",  kvs->process_state);
    BSON_APPEND_BINARY (doc, "file_content", BSON_SUBTYPE_BINARY, (uint8_t *)(kvs->file_content), kvs->file_content_len);
    //BSON_APPEND_UTF8 (doc, "file_content", kvs->file_content);
    BSON_APPEND_UTF8 (doc, "saveflag",  kvs->saveflag);
    BSON_APPEND_UTF8 (doc, "charset",  kvs->charset);

    /* 2015-11-09:�����ֶ� */
    BSON_APPEND_UTF8 (doc, "pass",  kvs->pass);
    BSON_APPEND_INT64 (doc, "line_num",  kvs->line_num);
    BSON_APPEND_INT64 (doc, "interval_time",  kvs->interval_time);
    BSON_APPEND_INT32 (doc, "security_level",  kvs->security_level);

    /* 2015-12-25�����ֶ� */
    unsigned long update_time;
    struct timeval tp;
    gettimeofday(&tp, NULL);
    update_time = (unsigned long)tp.tv_sec * 1000 + tp.tv_usec/1000;
    BSON_APPEND_INT64 (doc, "update_time",  update_time);

    BSON_APPEND_UTF8 (doc, "request_sql",  kvs->request_sql);

    /* his=1
       superserver=2
       portal=3
       telnet=4
       ftp=5
       IBM��http=30*/

    mongoc_bulk_operation_insert (conn->bulk, doc);
    if((type<6 && type>2) || 30==type){
        mongoc_bulk_operation_insert (conn->bulk_PortalTelnet, doc);
    }

    bson_destroy (doc);
}

/* @pair��ʽΪ"key=value", value�п����пհ��ַ� */
int pairParse(char *pair, struct keyValues *kv){
    char *name, *value;

    if (strlen(pair) > 5000) {
        pair[5000] = '\0';
    }
    name = pair;
    value = strchr(pair, '=');
    if (NULL == value) {
        PRINT_ERR_MSG("strchr(pair, '=') = NULL");
        value = pair+strlen(pair);
    } else {
        *value = '\0';
        value++;
    }
    if (strcmp(name, "rowkey") == 0) {
        sscanf(value, "%lu", &kv->capture_time);
        kv->capture_time /= 1000;
    } else if (strcmp(name, "table") == 0) {
        strcpy(kv->table, value);
    } else if (strcmp(name, "app_id") == 0) {
        strcpy(kv->app_id, value);
    } else if (strcmp(name, "sub_app_id") == 0) {
        strcpy(kv->sub_app_id, value);
    } else if (strcmp(name, "src_ip") == 0) {
        strcpy(kv->src_ip, value);
    } else if (strcmp(name, "src_mac") == 0) {
        strcpy(kv->src_mac, value);
    } else if (strcmp(name, "src_port") == 0) {
        strcpy(kv->src_port, value);
    } else if (strcmp(name, "dst_ip") == 0) {
        strcpy(kv->dst_ip, value);
    } else if (strcmp(name, "dst_mac") == 0) {
        strcpy(kv->dst_mac, value);
    } else if (strcmp(name, "dst_port") == 0) {
        strcpy(kv->dst_port, value);
    } else if (strcmp(name, "department") == 0) {
        strcpy(kv->department, value);
    } else if (strcmp(name, "web_session") == 0) {
        strcpy(kv->web_session, value);
    } else if (strcmp(name, "user_name") == 0) {
        strcpy(kv->user_name, value);
    } else if (strcmp(name, "web_url") == 0) {
        strcpy(kv->web_url, value);
    } else if (strcmp(name, "operation_command") == 0) {
        strcpy(kv->operation_command, value);
    } else if (strcmp(name, "web_content") == 0) {
        strcpy(kv->web_content, value);
    } else if (strcmp(name, "operation_sql") == 0) {
        strcpy(kv->operation_sql, value);
    } else if (strcmp(name, "user_id") == 0) {
        strcpy(kv->user_id, value);
    } else if (strcmp(name, "response_content") == 0) {
        strcpy(kv->response_content, value);
    } else if (strcmp(name, "tform") == 0) {
        strcpy(kv->tform, value);
    } else if (strcmp(name, "level_1") == 0) {
        strcpy(kv->level_1, value);
    } else if (strcmp(name, "alarm_id") == 0) {
        strcpy(kv->alarm_id, value);
    } else if (strcmp(name, "process_state") == 0) {
        strcpy(kv->process_state, value);
    } else if (strcmp(name, "file_content") == 0) {
        strcpy(kv->file_content, value);
    } else if (strcmp(name, "saveflag") == 0) {
        strcpy(kv->saveflag, value);
    } else if (strcmp(name, "charset") == 0) {
        strcpy(kv->charset, value);
    } else if (strcmp(name, "pass") == 0) {/* 2015-11-09:�����ֶ� */
        strcpy(kv->pass, value);
    } else if (strcmp(name, "line_num") == 0) {/* 2015-11-09:�����ֶ� */
        sscanf(value, "%lu", &kv->line_num);
    } else if (strcmp(name, "interval_time") == 0) {/* 2015-11-09:�����ֶ� */
        sscanf(value, "%lu", &kv->interval_time);
    } else if (strcmp(name, "security_level") == 0) {/* 2015-12-15:�����ֶ� */
        sscanf(value, "%d", &kv->security_level);
    } else if (strcmp(name, "request_sql") == 0) {/* 2016-05-08:�����ֶ� */
        strcpy(kv->request_sql, value);
    }
    return 0;
}

int lineParse(char *line, struct keyValues *kv) {
    char *start, *end;
    int tocopy;

    start = strstr(line, "rowkey");
    if (NULL == start) {
        return -1;
    }
    end = strstr(start, "|colfam");
    if (NULL == end) {
        return -1;
    }
    tocopy = ((end-start) >= (sizeof(pair)-1)) ? (sizeof(pair)-1) : (end-start);
    memset(pair, 0, sizeof(pair));
    if (tocopy > 0) {
        memcpy(pair, start, tocopy);
    }
    pair[tocopy] = '\0';

    pairParse(pair, kv);

    start = end + strlen("|colfam");
    while ((end = strstr(start, "|colfam")) != NULL) {
        start += 2;
        tocopy = ((end-start) >= (sizeof(pair)-1)) ? (sizeof(pair)-1) : (end-start);
        memset(pair, 0, sizeof(pair));
        if (tocopy > 0) {
            memcpy(pair, start, tocopy);
        }
        pair[tocopy] = '\0';
        pairParse(pair, kv);
        start = end + strlen("|colfam");
    }
    return 0;
}

/* ��@src��@dst����@srclen�ֽ����ݡ�@end��@src�����߽磬����Խ����� */
int copyHelper(char *dst, int dstMaxSize, long *dstlen, char *src, int srclen, char *end) {
    int tocopy;
    if (src+srclen > end) {
        PRINT_ERR_MSG("src+srclen > end");
        srclen = end-src;
        if (srclen < 0) return -1;
    }

    tocopy = 0;
    if(srclen > 0) {
        tocopy = srclen > (dstMaxSize-1) ? (dstMaxSize-1) : srclen;
        memcpy(dst, src, tocopy);
    }
    dst[tocopy] = '\0';
    *dstlen = tocopy;
    return srclen;
}

int getKeyValues(MONGO *conn, char *data, long contentLen) {
    char *pos, *end;
    int len, type=0, offset;
    int first = 1;

    memset(&KVs, 0, sizeof(struct keyValues));

    pos = data;
    while((end = strstr(pos, "|colfam\n"))) {
        /* posָ��һ�п�ͷ��endָ����ƫ�����ݿ�ͷ */
        end += 8;
        len = end - pos;
        if (len >= sizeof(line)-1) {
            PRINT_ERR_MSG("small @line");
            return -1;
        }
        memcpy(line, pos, len);
        line[len]='\0';

        /* posָ��ƫ�����ݿ�ͷ */
        pos = end;

        if (lineParse(line, &KVs) < 0) {
            PRINT_ERR_MSG("lineParse(line, &KVs) < 0");
            fprintf(stderr, "%s\n", line);
            return -1;
        }
        printKVs(__LINE__);
        type = atoi(KVs.app_id);

        if (1 == first) {
            char tableName[256];
            if (1==type) {
                /* cache(his) */
                sprintf(tableName, "%s%s", HIS_TABLE_NAME_PREFIX, KVs.table);
            } else if ((type<6 && type>1) || 30==type) {
                /* cache(portal/telent/ftp/superserver) */
                sprintf(tableName, "%s%s", CACHE_TABLE_NAME_PREFIX, KVs.table);
            } else if(type >= 6 && DB_TYPE_SQLSERVER==database_type){
                 /* SQL Server */
                sprintf(tableName, "%s%s", SQLSERVER_TABLE_NAME_PREFIX, KVs.table);
            } else if(type >= 6 && DB_TYPE_ORACLE==database_type){
                 /* Oracle */
                sprintf(tableName, "%s%s", ORACLE_TABLE_NAME_PREFIX, KVs.table);
            }
            conn->collection = mongoc_client_get_collection (conn->client, DB_NAME, tableName);
            conn->bulk = mongoc_collection_create_bulk_operation (conn->collection, 1, NULL);
            /* Cache��Portal/telnet�ٶ����һ�ε������ */
            if((type<6 && type>2) || 30==type){
                sprintf(tableName, "%s%s", CACHE_PORTAL_TELNET_PREFIX, KVs.table);
                conn->collection_PortalTelnet = mongoc_client_get_collection (conn->client, DB_NAME, tableName);
                conn->bulk_PortalTelnet = mongoc_collection_create_bulk_operation (conn->collection_PortalTelnet, 1, NULL);
            }
            first = 0;
        }

        /* capture_time�趨: telnet,ftpʵʱ��, oralce�Ӻ�5s�������Ӻ�120s���� */
        if (DB_TYPE_SQLSERVER==database_type || APP_ID_SUPERSERVER==type) {
            if (DB_TYPE_SQLSERVER==database_type && type<APP_ID_SQLSERVER_SELECT) return 0;

            /* endָ��ƫ������ĩβ(pos��ָ��ƫ�����ݿ�ͷ) */
            offset = copyHelper(KVs.operation_command, sizeof(KVs.operation_command), &(KVs.requestLen), pos, atoi(KVs.operation_command), data+contentLen);
            if (offset < 0) return -1;
            end += (offset + 1); /* offset�ֽ����� + 1�ֽ�\n */

            /* response����(cache�Ĵ���: ��ֻ��requestʱ���ⲿ�־Ͳ�д��) */
            if (end-data < contentLen) {
                pos = end; /* posָ��һ�п�ͷ */

                end = strstr(pos,"|colfam\n");
                if (NULL == end) {
                    PRINT_ERR_MSG("NULL == end");
                    return -1;
                }
                /* endָ����ƫ�����ݿ�ͷ */
                end += 8;
                len = end - pos;
                if (len >= sizeof(line)) {
                    PRINT_ERR_MSG("small @line");
                    return -1;
                }
                memcpy(line, pos, len);
                line[len]='\0';

                /* posָ��ƫ�����ݿ�ͷ */
                pos = end;

                if (lineParse(line, &KVs) < 0) {
                    PRINT_ERR_MSG("lineParse(line, &KVs) < 0");
                    fprintf(stderr, "%s\n", line);
                    return -1;
                }

                /* endָ��ƫ������ĩβ(pos��ָ��ƫ�����ݿ�ͷ) */
                offset = copyHelper(KVs.response_content, sizeof(KVs.response_content), &(KVs.responseLen), pos, atoi(KVs.response_content), data+contentLen);
                if (offset < 0) return -1;
                end += (offset + 1); /* offset�ֽ����� + 1�ֽ�\n */
            }
            KVs.capture_time += 120000; /*  /dev/shm/�µ��ļ���2����֮ǰ���ļ� */
        } else if (APP_ID_HIS==type || APP_ID_PORTAL==type || 30==type) {
            /* endָ��ƫ������ĩβ(pos��ָ��ƫ�����ݿ�ͷ) */
            KVs.capture_time += 120000; /*  /dev/shm/�µ��ļ���2����֮ǰ���ļ� */
            offset = copyHelper(KVs.file_content, sizeof(KVs.file_content), &(KVs.file_content_len), pos, atoi(KVs.file_content), data+contentLen);
            if (offset < 0) return -1;
            end += (offset + 1); /* offset�ֽ����� + 1�ֽ�\n */
        } else if (APP_ID_TELNET==type || APP_ID_FTP==type || DB_TYPE_ORACLE==database_type) {
            /* endָ��ƫ������ĩβ(pos��ָ��ƫ�����ݿ�ͷ) */
            if (DB_TYPE_ORACLE==type) {
                KVs.capture_time += 120000; /*  /dev/shm/�µ��ļ���2����֮ǰ���ļ� */
            }
            offset = copyHelper(KVs.response_content, sizeof(KVs.response_content), &(KVs.responseLen), pos, atoi(KVs.response_content), data+contentLen);
            if (offset < 0) return -1;
            end += (offset + 1); /* offset�ֽ����� + 1�ֽ�\n */
            KVs.requestLen = strlen(KVs.operation_command);
        }

        /* �����ݿ�mongodb��ʱ�ṹ */
        mongoInsertClassification(conn, &KVs, type);

        /* ���������һ�У��������������"rowkey="��ͷ */
        if((end - data) < contentLen) {
            if(strncmp(end, "rowkey=", 7) != 0){
                break;
            }
        } else {
            break;
        }

        /* �������� */
        pos = end;
    }

    bson_t reply;
    bson_error_t error;
    int ret = mongoc_bulk_operation_execute (conn->bulk, &reply, &error);
    if (!ret) {
        fprintf (stderr, "Error: %s\n", error.message);
    }

    bson_destroy (&reply);
    mongoc_bulk_operation_destroy (conn->bulk);
    mongoc_collection_destroy(conn->collection);
    if((type<6 && type>2) || 30==type){
        int ret = mongoc_bulk_operation_execute (conn->bulk_PortalTelnet, &reply, &error);
        if (!ret) {
            fprintf (stderr, "Error: %s\n", error.message);
        }
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

/* read()�ķ�װ���ɹ����ض�ȡ���ֽ�����ʧ�ܷ���-1 */
ssize_t readBytes(int fd, void *buf, size_t n) {
    char *pos = buf;
    ssize_t nrd, nLeft=n;

    while (nLeft > 0) {
        if ( (nrd = read(fd, pos, nLeft)) < 0) {
            if (EINTR == errno) {
                nrd = 0;  /* �ٴε���read */
            } else {
	            PRINT_ERR_MSG("read() failed");
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
    if (fd < 0) { PRINT_ERR_MSG("Fails to open the file"); return -1; }
    if ((nrd = readBytes(fd, pos, n)) < 0) { PRINT_ERR_MSG("read failed"); return -1; }
    close(fd);
    return nrd;
}

/* �������ļ�,@fileName����·�� */
int handleOnlyOneFile(char *fileName) {
    char filePath[512];
    int retval;
    MONGO conn;

    if (!memcmp(fileName, "Sql_csp_", strlen("Sql_csp_"))) {
        database_type = DB_TYPE_CSP;
    } else if (!memcmp(fileName, "Sql_studio_", strlen("Sql_studio_"))) {
        database_type = DB_TYPE_STUDIO;
    } else if (!memcmp(fileName, "Sql_sqlserver_", strlen("Sql_sqlserver_"))) {
        database_type = DB_TYPE_SQLSERVER;
    } else if (!memcmp(fileName, "Sql_oracle_", strlen("Sql_oracle_"))) {
        database_type = DB_TYPE_ORACLE;
    } else if (!memcmp(fileName, "Sql_terminal_", strlen("Sql_terminal_"))) {
        database_type = DB_TYPE_TELNET;
    } else {
        PRINT_ERR_MSG("wrong file name");
        exit(-1);
    }

    retval = mongoConnect(&conn);
    if (retval < 0) {
        return -1;
    }

    sprintf(filePath, "%s%s", DIR_FOR_DB, fileName);
    retval = readFile(filePath, fileContent, sizeof(fileContent)-1);
    if (retval <= 0) return 0;
    //unlink(filePath);

    getKeyValues(&conn, fileContent, retval);
    mongoClose(&conn);
    return 0;
}

//ֻ��3���ܰ󶨣�1U����4����0123��2U��8����01234567
int BindCpu(int toBind){
	cpu_set_t mask;
	CPU_ZERO(&mask);    //�ÿ�
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
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//�����߳�CPU�׺���
    {
//       printf("warning: could not set CPU affinity!\n");
	   return -1;
    }
//	printf("set CPU affinity success!\n");
	return 0;
}


char helpStr[] = "\n\n***********************************************************\n"
                 "USAGE:\n"
                 "audit_mongo_insert <daemonFlag> [filename]\n"
                 "<daemonFlag>(0/1): Set daemon mode, 1 for YES, 0 for NO\n"
                 "[filename]: Optional. Specify the only file need to be handled. without path\n"
                 "***********************************************************\n\n";
#if 1
#define DEBUG_TIME_INIT() struct timeval timeval1, timeval2
#define DEBUG_TIME_START() gettimeofday(&timeval1, NULL)
#define DEBUG_TIME_STOP(msg) \
do {\
    gettimeofday(&timeval2, NULL);\
}while(0)

//fprintf(stderr, "%s : sec=%lu, usec=%lu\n", (msg), timeval2.tv_sec-timeval1.tv_sec, timeval2.tv_usec-timeval1.tv_usec);

int main(int argc,char ** argv) {
    char filePath[512];
    int retval, flag;
    DIR *dp;
    struct dirent *dirp;
    MONGO conn;
    BindCpu(7);

    if (2 == argc || 3 == argc) {
        flag = atoi(argv[1]);
        if (flag!=0 && flag != 1) {
            fprintf(stderr, helpStr);
            exit(0);
        }
    } else {
        fprintf(stderr, helpStr);
        exit(0);
    }

    /* ��̨���� */
    if (1 == flag) NC_daemon_audit();

    /* ֻ����һ���ļ� */
    if (3 == argc) {
        handleOnlyOneFile(argv[2]);
        return 0;
    }

    retval = mongoConnect(&conn);
    if (retval < 0) {
        return -1;
    }

    while(1) {
        /* ��ȡ·��'DIR_FOR_DB'�µ����5000���ļ��������浽'file_name'�� */
        if((dp=opendir(DIR_FOR_DB)) == NULL) {
            PRINT_ERR_MSG("Open dir failed");
            return 0;
        }
        while((dirp=readdir(dp)) != NULL) {
            if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0)){
                continue;
            }

            if (!memcmp(dirp->d_name, "Sql_csp_", strlen("Sql_csp_"))) {
                database_type = DB_TYPE_CSP;
            } else if (!memcmp(dirp->d_name, "Sql_studio_", strlen("Sql_studio_"))) {
                database_type = DB_TYPE_STUDIO;
            } else if (!memcmp(dirp->d_name, "Sql_sqlserver_", strlen("Sql_sqlserver_"))) {
                database_type = DB_TYPE_SQLSERVER;
            } else if (!memcmp(dirp->d_name, "Sql_oracle_", strlen("Sql_oracle_"))) {
                database_type = DB_TYPE_ORACLE;
            } else if (!memcmp(dirp->d_name, "Sql_terminal_", strlen("Sql_terminal_"))) {
                database_type = DB_TYPE_TELNET;
            }

            sprintf(filePath, "%s%s", DIR_FOR_DB, dirp->d_name);
            retval = readFile(filePath, fileContent, sizeof(fileContent)-1);
            unlink(filePath);
            if (retval <= 0) continue;
            if (getKeyValues(&conn, fileContent, retval) < 0) {
                int fd = open("/home/mongoInsertDebug", O_CREAT|O_RDWR|O_APPEND);
                if (fd > 0) {
                    write(fd, fileContent, retval);
                    write(fd, "\n\n\n", 3);
                    close(fd);
                }
            }
        }
        closedir(dp);
        usleep(1000);
    }
    mongoClose(&conn);
    return 0;
}
#endif
#if 0
int main() {
    char testBuf[] =
        /* supersver��sqlserver����operation_command��request_content */
        "rowkey=123456789"
        "|colfam1:table=20150818"
        "|colfam1:app_id=2"/*supersver*/
        "|colfam1:saveflag=1"
        "|colfam1:src_ip=192.168.1.1"
        "|colfam1:src_mac=11-11-11-11-11-11"
        "|colfam1:src_port=88"
        "|colfam1:dst_ip=192.168.1.2"
        "|colfam1:dst_mac=22-22-22-22-22-22"
        "|colfam1:dst_port=99"
        "|colfam1:user_id=12"
        "|colfam1:operation_command=2"
        "|colfam\n"
        "aa"
        "\n"
	    "rowkey=123456789"
	    "|colfam1:response_content=3"
	    "|colfam\n"
        "bbb"
	    "\n"
        /* his��portal����file_content */
        "rowkey=123"
        "|colfam1:table=20150818"
        "|colfam1:app_id=1"/*supersver*/
        "|colfam1:saveflag=0"
        "|colfam1:src_ip=192.168.1.100"
        "|colfam1:src_mac=11-11-11-11-11-11"
        "|colfam1:src_port=880"
        "|colfam1:dst_ip=192.168.1.200"
        "|colfam1:dst_mac=22-22-22-22-22-22"
        "|colfam1:dst_port=990"
        "|colfam1:user_id=120"
        "|colfam1:file_content=4"
        "|colfam\n"
        "cccc"
        "\n"
        /* telnet,oracle��ftp����response_content */
        "rowkey=123"
        "|colfam1:table=20150818"
        "|colfam1:app_id=3"
        "|colfam1:saveflag=0"
        "|colfam1:src_ip=192.168.1.100"
        "|colfam1:src_mac=22-22-22-22-22-22"
        "|colfam1:src_port=880"
        "|colfam1:dst_ip=192.168.1.200"
        "|colfam1:dst_mac=22-22-22-22-22-22"
        "|colfam1:dst_port=990"
        "|colfam1:user_id=120"
        "|colfam1:response_content=5"
        "|colfam\n"
        "ddddd"
	    "\n";

    int retval;
    MONGO conn;

    retval = mongoConnect(&conn);
    if (retval < 0) {
        return 0;
    }

    getKeyValues(&conn, testBuf, strlen(testBuf));
	return 0;
}
#endif


