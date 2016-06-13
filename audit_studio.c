#include<ctype.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include"audit_api.h"
#include"audit_database_sql.h"

#define STUDIO_ITEM_MAX 100 /* 填数据库的文件的内容条数最大值 */
#define REQUEST_RESPONSE_BUFFER_LIMIT (10*1024*1024) /* 缓存文件大小不能超10M */

#define PRINT_ERR(s) fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, (s))

#if 0
#define TEST_PRINT(s) \
do {\
    FILE* fp = fopen("/dev/shm/studio_tsi_tmp/debug_log", "a+");\
    fprintf(fp, "%s:%d:%s\n", __FILE__, __LINE__, (s)); \
    fclose(fp);\
}while(0)
#else
#define TEST_PRINT(s)
#endif

/* 这些宏的名字设计为跟包的关键字相同，不需要都大写了 */
/* Part1: 该部分类型，都有28字符d的前缀 */
#define STUDIO_ClassList                    10 //class状态心跳
#define STUDIO_GetCurrentTimeStamp          11 //时间戳心跳
#define STUDIO_IsGenerated                  12 //文件状态询问，文件操作的准备工作
#define STUDIO_GetSpecialListByStream       13 //工程信息列表询问，文件操作的准备工作
#define STUDIO_Exists                       14 //文件存在状态询问，文件操作的准备工作
#define STUDIO_OpenId                       15 //client打开文件时的请求之一
#define STUDIO_Name                         16 //打开/保存文件时的请求文件名或上传文件名
#define STUDIO_LanguageModeGet              17 //打开文件前获取语言模式
#define STUDIO_Code_S2C                     18 //打开文件时，server向client发送包内容
#define STUDIO_Code_C2S                     19 //保存文件时，client向server发送包内容
#define STUDIO_UserType                     20 //用户类型查询，打开，保存文件等操作时出现
#define STUDIO_LockClass                    21 //修改class定义前，给class上锁
#define STUDIO_UnlockClass                  22 //修改完class定义后，给class解锁
#define STUDIO_Lock                         23 //修改csp,inc,mac,MVB,js,xml,css,bas文件前上锁
#define STUDIO_Unlock                       24 //修改csp,inc,mac,MVB,js,xml,css,bas文件后解锁
#define STUDIO_SaveDefinition               25 //保存class
#define STUDIO_Save                         26 //保存csp,inc,mac,MVB,js,xml,css,bas文件
#define STUDIO_DeleteClassDefinition        27 //删除class
#define STUDIO_Delete                       28 //删除csp,inc,mac,MVB,js,xml,css,bas文件
#define STUDIO_NameSet                      29 //文件名设置
#define STUDIO_AddItem                      30 //向Project中添加class或csp,inc,mac,MVB,js,xml,css,bas文件
#define STUDIO_RemoveItem                   31 //从Project中删除class或csp,inc,mac,MVB,js,xml,css,bas文件
#define STUDIO_StudioOpenDialog             32 //打开一个对话框
#define STUDIO_IsClassnameConflict          33 //是否重名，新建，改名等操作中的查询包
#define STUDIO_GetStudioInfo                34 //查询server版本等信息
#define STUDIO_SourceControlCreate          35 //创建控制接口?
#define STUDIO_CompileClass                 36 //编译cls
#define STUDIO_Compile                      47 //编译csp
#define STUDIO_DocumentHistoryAdd           38 //增加历史记录
#define STUDIO_DocumentHistoryGet           39 //获得历史记录
#define STUDIO_GetDefinition                40 //获得class定义
#define STUDIO_StudioTempateMgrTemplates    41 //获得平台信息
#define STUDIO_GetAppName                   42 //获得应用对象名
#define STUDIO_New                          43 //新建csp
#define STUDIO_GetWebServerPort             44 //获得Web Server端口
#define STUDIO_AutoComplete                 45 //自动生成文件

/* Part2: 下面这几个类型，前缀是少于28字符的 */
//#define STUDIO_norecursive                  110 //未知, 先放放, 注意，他的前缀是19字节
#define STUDIO_Compile_Following            111 //编译cls或其他包后跟的6个左右的14字节的请求包
//#define STUDIO_Login                        112 //登录操作

/* Part3: 未知类型 */
#define STUDIO_Unknown                      0 //未知类型

/* keywords和类型值对应表 */
struct kw_type_table {
    char keywords[128];
    int rv; /* 类型值 */
};

/* 包执行的动作分类，返回代表包类型的正整数 */
#define PREFIX_SIZE 28 /* 大多数包是有28字节的前缀的 */
int do_classify(char *src, int n) {
    int i, non_space, is_prefix, left;
    char *pos;
    struct kw_type_table ktt[100] = {
        {"%Studio.ClassMgr ClassList", STUDIO_ClassList},
        {"%Studio.Project(GetCurrentTimeStamp)", STUDIO_GetCurrentTimeStamp},
        {"%Library.RoutineMgr(GetCurrentTimeStamp)", STUDIO_GetCurrentTimeStamp},
        {"%Library.RoutineMgr IsGenerated", STUDIO_IsGenerated},
        {"%Studio.ClassMgr. GetSpecialListByStream", STUDIO_GetSpecialListByStream},
        {"%Library.RoutineMgr Exists", STUDIO_Exists},
        {"%SYSTEM.OBJ OpenId", STUDIO_OpenId},
        {"Name", STUDIO_Name},
        {"%Library.RoutineMgr LanguageModeGet", STUDIO_LanguageModeGet},
        {"Code", STUDIO_Code_S2C},
        {"Code", STUDIO_Code_C2S},
        {"%RoutineMgr UserType", STUDIO_UserType},
        {"%Library.qccServer Run LockClass", STUDIO_LockClass},
        {"%Library.qccServer Run UnlockClass", STUDIO_UnlockClass},
        {"%Library.RoutineMgr Lock", STUDIO_Lock},
        {"%Library.RoutineMgr Unlock", STUDIO_Unlock},
        {"%Studio.ClassMgr SaveDefinition", STUDIO_SaveDefinition},
        {"%Library.RoutineMgr %Save", STUDIO_Save},
        {"%Library.qccServer Run DeleteClassDefinition", STUDIO_DeleteClassDefinition},
        {"%Library.RoutineMgr Delete", STUDIO_Delete},
        {"%Library.RoutineMgr NameSet", STUDIO_NameSet},
        {"%Studio.Project AddItem", STUDIO_AddItem},
        {"%Studio.Project RemoveItem", STUDIO_RemoveItem},
        {"%Library.RoutineMgr StudioOpenDialog", STUDIO_StudioOpenDialog},
        {"%Library.qccServer Run ( IsClassnameConflict", STUDIO_IsClassnameConflict},
        {"%Studio.General GetStudioInfo", STUDIO_GetStudioInfo},
        {"%Studio.SourceControl.Interface(SourceControlCreate", STUDIO_SourceControlCreate},
        {"%Library.RoutineMgr CompileClass", STUDIO_CompileClass},
        {"%Library.RoutineMgr Compile", STUDIO_Compile},
        {"%SYS.Studio.DocumentHistory Add", STUDIO_DocumentHistoryAdd},
        {"%SYS.Studio.DocumentHistory Get", STUDIO_DocumentHistoryGet},
        {"%Studio.ClassMgr GetDefinition", STUDIO_GetDefinition},
        {"%CSP.StudioTemplateMgr Templates", STUDIO_StudioTempateMgrTemplates},
        {"%SYSTEM.CSP GetAppName", STUDIO_GetAppName},
        {"New", STUDIO_New},
        {"%Studio.General\" GetWebServerPort", STUDIO_GetWebServerPort},
        {"%Library.RoutineMgr AutoComplete", STUDIO_AutoComplete}
    };

    /* 登录包, 关键字前有11个空格 */
    //if (n>strlen("           CI") && !memcmp(src+1, "           CI", strlen("           CI"))) {
    //    return STUDIO_Login;
    //}

    /* 大多数包是有28字节的前缀的，但也有少量的不满32字节的交互包，例如编译完cls
     * 后，会有6个左右的14字节的请求包 */
    if (n <= PREFIX_SIZE) {
        if (14 == n && 'B'==src[12] && 'L'==src[13]) return STUDIO_Compile_Following;
        else return STUDIO_Unknown;
    }

    /* 非空格字符少于5个，则是前缀 */
    non_space = 0;
    for (i=0; i < ((n>=28)?28:n); i++) {
        if (src[i] != ' ') non_space++;
    }
    if (non_space > 5) return STUDIO_Unknown;

    pos = src + PREFIX_SIZE;
    left = n - PREFIX_SIZE;

    for (i=0; i<37; i++) {
        if (left > strlen(ktt[i].keywords) && !memcmp(pos, ktt[i].keywords, strlen(ktt[i].keywords))) {
            /* "Code"关键字对应2种情况 */
            if (STUDIO_Code_S2C == ktt[i].rv || STUDIO_Code_C2S == ktt[i].rv) {
                if (left-4 > 3) return STUDIO_Code_C2S;
                else return STUDIO_Code_S2C;
            }
            return ktt[i].rv;
        }
    }
    return STUDIO_Unknown;
}

/* 包类型分析，具体操作由do_classify()完成
 * 成功返回代表类型的正整数，失败返回-1 */
int classify(char *fname) {
    int fd, n, i, unprintable;
    char buf[100] = {0}, *pos;

    fd = open(fname, O_RDWR, 0666);
    if (fd < 0) {
        TEST_PRINT("打开文件失败");
        return -1;
    }
    n = read(fd, buf, sizeof(buf)-1);/* 99个字符足够包含关键字了 */
    close(fd);
    if (n < 0) {
        TEST_PRINT("读文件失败");
        return -1;
    }

    return do_classify(buf, n);
}


struct buffer_files {
    char request_fname[128];
    char response_fname[128];
};
extern pthread_rwlock_t tablesTime_lock;
extern char tablesTime[AUDIT_TIME_LEN];
static char cur_flie_name[THREADS_NUM][256] = {{0}};/* 当前填数据库的文件的完全路径名 */
static char cur_dstflie_name[THREADS_NUM][256] = {{0}};
static int item_cnt[THREADS_NUM] = {0};/* 每个填数据库的文件的条数的最大值 */
static int fname_suffix_n[THREADS_NUM] = {0};

/* 向文件'fname'写入'len'字节的'content'.若文件不存在则创建它
 * 返回成功写入的字节数, 失败返回-1 */
int studio_write_file(char *fname, char *content, int len) {
    int fd, nwr;
    TEST_PRINT("<studio_write_file> start");

    if (NULL==fname || NULL==content || len<=0) return -1;
    fd = open(fname, O_CREAT|O_RDWR|O_APPEND, 0666);
    if (fd < 0) {
        PRINT_ERR("Fails to open the file.");
        return -1;
    }
    if ((nwr = write(fd, content, len)) < 0) {
        PRINT_ERR("Fails to write the data into file.");
        close(fd);
        return -1;
    }
    close(fd);

    TEST_PRINT("<studio_write_file> end");
    return nwr;
}

/* 把文件'src_fname'的所有内容追写到文件'dst_fname'
 * 返回成功写到文件'dst_fname'的字节数, 打开文件失败返回-1
 * 注意 - 调用本函数之前, 2个文件都认为是存在的 */
int studio_file2file_append(char *dst_fname, char *src_fname) {
    TEST_PRINT("<studio_file2file_append> start");

    int dstfd, srcfd, nwr, nrd, success_num;
    char buf[4096];

    success_num = 0;
    dstfd = open(dst_fname, O_RDWR | O_APPEND);
    if (dstfd < 0) {
        PRINT_ERR("Fails to open the file.");
        return -1;
    }
    srcfd = open(src_fname, O_RDWR);
    if (srcfd < 0) {
        PRINT_ERR("Fails to open the file.");
        close(dstfd);
        return -1;
    }

    while ((nrd = read(srcfd, buf, sizeof(buf))) > 0) {
        nwr = write(dstfd, buf, nrd);
        if (nwr < 0) {
            PRINT_ERR("Fails to write the file.");
            break;
        }
        success_num += nwr;
    }
    close(dstfd);
    close(srcfd);

    TEST_PRINT("<studio_file2file_append> end");
    return success_num;
}

/* 删除'str'开头和末尾连续的空白字符. 原址进行 */
static int studio_trim(char *str) {
    if (NULL==str || '\0'==str[0]) return 0;
    char *start = str,
         *end = str+strlen(str)-1;
    int num;

    while (' '==*start || '\t'==*start || '\r'==*start || '\n'==*start) {
        start++;
    }
    while (end > start
           && (' '==*end || '\t'==*end || '\r'==*end || '\n'==*end)) {
        end--;
    }

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
 * 返回'dst'字符串的长度
 * 最后将结果字符串的开头和结尾的连续的空白字符去掉
 * 注意 - 前28个字符的不可显字符变为空格，但不合并，仍保持28字符长度 */
static int studio_unprintable_to_space(char *src, int slen, char *dst, int dstlen) {
    int i=0, j=0;
    /* 前28个字节的不可打印字符变为空格，且不能合并字符，保证开始的28字符偏移量不变
     * 为后面的关键字查询提供遍历 */
    while (i<28 && j<(dstlen-1) && i<slen) {
        dst[j] = (isprint(src[i]) || isspace(src[i])) ? src[i] : ' ';
        ++i, ++j;
    }
    while (i<slen && j<(dstlen-1)) {
        if ('\0' == src[i]) {
            i++;
            continue;
        }
        if (isprint(src[i]) || isspace(src[i])) {
            dst[j++] = src[i++];
        } else {
            dst[j++] = ' ';
            while(i<slen && !(isprint(src[i]) || isspace(src[i]))) i++;
        }
    }
    dst[j] = '\0';

    return j;
}

/* 获取'ip'的点分形式, 'ipstr'保存结果, 'len'是'ipstr'的大小 */
int studio_get_ipstr(u_int32_t ip, char *ipstr, size_t len) {
    struct in_addr addr;
    addr.s_addr = ip;
    inet_ntop(AF_INET, &addr, ipstr, len);
    return 0;
}

int studio_get_file_size(char *filename) {
    struct stat buf;
    if(stat(filename, &buf)<0) {
        return -1;
    }
    return (int)buf.st_size;
}

/* 删除文件最近修改时间是2分钟以前的文件'fname' */
int studio_handle_olddata(char *fname) {
    struct stat buf;
    int ret;
    time_t cur, mtime;

    ret = stat(fname, &buf);
    if (ret < 0) return -1;
    mtime = buf.st_mtime;
    cur = time(NULL);

    if (cur-mtime > 120) {
        unlink(fname);
    }

    return 0;
}


int get_pkt_type(char *fname) {
    int fd, n, i, unprintable;
    char buf[100] = {0}, *pos;

    fd = open(fname, O_RDWR, 0666);
    if (fd < 0) {
        return -1;
    }
    n = read(fd, buf, sizeof(buf)-1);
    close(fd);
    if (n < 0) {
        return -1;
    }

    /* 当不够或刚够28+"Code"/"%Studio.ClassMgr SaveDefinition"的最小长度，即32时，
     * 则不是文件 */
    if (n <= 32) return 0;

    /* 这两种情况，前28个字符的可显示字符很少 */
    unprintable = 0;
    for (i=0; i<28; i++) {
	    if (!isprint(buf[i])) {
            unprintable++;
        }
    }
    if (unprintable > 5) {
        return 0;
    }

    pos = buf + 28;
    if (((n-28) > 4)
        && ('C'==pos[0] && 'o'==pos[1] && 'd'==pos[2] && 'e'==pos[3])) {
        printf("request=%s\n", buf);
        return 1;
    }

    if (((n-28) > strlen("%Studio.ClassMgr SaveDefinition"))
        && !memcmp(pos, "%Studio.ClassMgr SaveDefinition", strlen("%Studio.ClassMgr SaveDefinition")))
    {
        printf("request=%s\n", buf);
        return 1;
    }
    return 0;
}

/* 写文件'db_fname'. 成功返回0, 失败返回-1 */
int studio_push_into_db(struct buffer_files *bf, struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info) {
    char buffer[4096]={0}, time_for_rowkey[32], time_for_table_item[AUDIT_TIME_LEN]={0},
         srcipstr[32]={0}, dstipstr[32]={0}, srcmac[64]={0}, dstmac[64]={0};
    u_int16_t srcport, dstport;
    int thread_id = p_info_hd->thrid;
    int request_len, response_len, byte_wr;
    int request_exist, response_exist, pkt_type;

    if (NULL==bf || NULL==p_info_hd || NULL==p_info) {
        PRINT_ERR("<studio_push_into_db> one or more argument is NULL");
    }
    if ('\0'==bf->request_fname || '\0'==bf->response_fname) {
        PRINT_ERR("<studio_push_into_db> request_fname/response_fname is/are NULL");
    }

    TEST_PRINT("<studio_push_into_db> start");

    /* 获得缓存文件长度 */
    request_exist = response_exist = 1;
    request_len = studio_get_file_size(bf->request_fname);
    response_len = studio_get_file_size(bf->response_fname);

    if (request_len < 0) {
        request_exist =0;
        request_len = 0;
    }
    if (response_len < 0) {
        response_exist = 0;
        response_len = 0;
    }
    if (0==request_len && 0==response_len)
    {
        if (request_exist) unlink(bf->request_fname);
        if (response_exist) unlink(bf->response_fname);
        return 0;
    }

    /* 生成填数据库的缓存文件的完全路径 */
    if (0==item_cnt[thread_id] || '\0'==cur_flie_name[thread_id][0]
        || '\0'==cur_dstflie_name[thread_id][0])
    {
        item_cnt[thread_id] = 0;
        sprintf(cur_flie_name[thread_id],
            SQL_TMP
            STUDIO_SQL_FILE
            "%lu_%d_%d", p_info_hd->audit_time, fname_suffix_n[thread_id], thread_id);
        sprintf(cur_dstflie_name[thread_id],
            SQL_PATH
            STUDIO_SQL_FILE
            "%lu_%d_%d", p_info_hd->audit_time, fname_suffix_n[thread_id], thread_id);
        fname_suffix_n[thread_id]++;
    }

    /* 点分式ip (cli对应 表里面的src    ser 对应des)*/
    studio_get_ipstr(p_info_hd->cli_ip, srcipstr, 32);
    studio_get_ipstr(p_info_hd->ser_ip, dstipstr, 32);

    /* mac */
    sprintf(srcmac, "%02x-%02x-%02x-%02x-%02x-%02x",
        p_info_hd->cli_mac[0], p_info_hd->cli_mac[1], p_info_hd->cli_mac[2],
        p_info_hd->cli_mac[3], p_info_hd->cli_mac[4], p_info_hd->cli_mac[5]);
    sprintf(dstmac, "%02x-%02x-%02x-%02x-%02x-%02x",
        p_info_hd->ser_mac[0], p_info_hd->ser_mac[1], p_info_hd->ser_mac[2],
        p_info_hd->ser_mac[3], p_info_hd->ser_mac[4], p_info_hd->ser_mac[5]);

    /* port */
    srcport = p_info_hd->cli_port;
    dstport = p_info_hd->ser_port;

    /* 取下面的table项的值 */
    pthread_rwlock_rdlock(&tablesTime_lock);
    strcpy(time_for_table_item, tablesTime);
    pthread_rwlock_unlock(&tablesTime_lock);

    /* 取下面的rowkey项的值 */
    memset(time_for_rowkey, 0, sizeof(time_for_rowkey));
#if REL_HBASE
    snprintf(time_for_rowkey, 31, "%lu", p_info_hd->audit_time);
#else
    get_audit_time(time_for_rowkey);
#endif

	pkt_type = 0;
	pkt_type = classify(bf->request_fname);
    if (pkt_type < 0) pkt_type = 0;

    /* request缓存 */
    sprintf(buffer,
        "rowkey=%s"
        //"rowkey=%s-00%u"
        "|colfam1:table=%s"
        "|colfam1:app_id=%u"
        "|colfam1:saveflag=%d" /* 是否是文件内容的标记 */
        "|colfam1:src_ip=%s"
        "|colfam1:src_mac=%s"
        "|colfam1:src_port=%u"
        "|colfam1:dst_ip=%s"
        "|colfam1:dst_mac=%s"
        "|colfam1:dst_port=%u"
        "|colfam1:user_id=%u"
        "|colfam1:operation_command=%d"
        "|colfam\n",
        time_for_rowkey, /*2,*/ time_for_table_item, 2, pkt_type, srcipstr, srcmac, srcport,
        dstipstr, dstmac, dstport, p_info_hd->user_id, request_len);

    studio_write_file(cur_flie_name[thread_id], buffer, strlen(buffer));
    if (request_len > 0) {
        byte_wr = studio_file2file_append(cur_flie_name[thread_id], bf->request_fname);
        if (byte_wr != request_len) {//是否需要修正处理?
            PRINT_ERR("byte_wr != request_len");
        }
    }

    /* response缓存 */
    sprintf(buffer,
        "\n"
        "rowkey=%s"
        //"rowkey=%s-00%u"
        "|colfam1:table=%s"
        "|colfam1:app_id=%u"
        "|colfam1:saveflag=%d" /* 是否是文件内容的标记 */
        "|colfam1:src_ip=%s"
        "|colfam1:src_mac=%s"
        "|colfam1:src_port=%u"
        "|colfam1:dst_ip=%s"
        "|colfam1:dst_mac=%s"
        "|colfam1:dst_port=%u"
        "|colfam1:user_id=%u"
        "|colfam1:response_content=%d"
        "|colfam\n",
        time_for_rowkey, /*2,*/ time_for_table_item, 2, pkt_type, srcipstr, srcmac, srcport,
        dstipstr, dstmac, dstport, p_info_hd->user_id, response_len);
    studio_write_file(cur_flie_name[thread_id], buffer, strlen(buffer));
    if (response_len > 0) {
        byte_wr = studio_file2file_append(cur_flie_name[thread_id], bf->response_fname);
        if (byte_wr != response_len) {//是否需要修正处理?
            PRINT_ERR("byte_wr != response_len");
        }
    }
    studio_write_file(cur_flie_name[thread_id], "\n", 1);

    /* 删除request, response文件 */
    unlink(bf->request_fname);
    unlink(bf->response_fname);

    item_cnt[thread_id]++;

    /* 移动填数据库的文件 */
    if (STUDIO_ITEM_MAX == item_cnt[thread_id]) {
        rename(cur_flie_name[thread_id], cur_dstflie_name[thread_id]);
        unlink(cur_flie_name[thread_id]);
        item_cnt[thread_id] = 0;
        cur_flie_name[thread_id][0] = '\0';
        cur_dstflie_name[thread_id][0] = '\0';
    }

    TEST_PRINT("<studio_push_into_db> normally return");
    return 0;
}

/* 结束一对request/response的情况 - rst/fin/syn包，下一个有内容的request包 */
u_int32_t audit_studio(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char * data) {
    char printable_data[4096] = {0};
    struct buffer_files cur_bf;
    int printable_len;

    /* 过滤重复包 */
    if (p_info_hd->isin == 0) {
        if (p_info->out_seq != p_info_hd->seq) {
            p_info->out_seq = p_info_hd->seq;
        } else {
            return 0;
        }
    } else {
        if (p_info->in_seq != p_info_hd->seq) {
            p_info->in_seq = p_info_hd->seq;
        } else {
            return 0;
        }
    }

    /* request/response缓存文件名 */
    sprintf(cur_bf.request_fname, "/dev/shm/studio_tsi_tmp/request_%u_%d", p_info_hd->hash, p_info_hd->thrid);
    sprintf(cur_bf.response_fname, "/dev/shm/studio_tsi_tmp/response_%u_%d", p_info_hd->hash, p_info_hd->thrid);

    if (!(p_info_hd->isin)) {/* request包(监测对象机器到server方向) */
        if (p_info_hd->rst || p_info_hd->fin || p_info_hd->syn) {
            /* 处理已缓存内容 */
            if (studio_get_file_size(cur_bf.request_fname) > 0
                || studio_get_file_size(cur_bf.response_fname) > 0) {
                studio_push_into_db(&cur_bf, p_info_hd, p_info);
            }
            return 0;
        }

        /* 单纯的ack包 */
        if (0 == p_info_hd->payload_len) {
            return 0;
        }

        /* response包缓存非空, 表示新来回的开始, 则结束上一个request/response */
        if (studio_get_file_size(cur_bf.response_fname) > 0) {
            studio_push_into_db(&cur_bf, p_info_hd, p_info);
        }

        /* 缓存request包 */
        if (studio_get_file_size(cur_bf.request_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
            printable_len = studio_unprintable_to_space((void*)data, p_info_hd->payload_len, printable_data, sizeof(printable_data));
            studio_handle_olddata(cur_bf.request_fname);
            studio_write_file(cur_bf.request_fname, printable_data, printable_len);
        } else {
            TEST_PRINT("studio_get_file_size(cur_bf.request_fname) >= REQUEST_RESPONSE_BUFFER_LIMIT");
        }
    } else { /* response包 */
        if (p_info_hd->rst || p_info_hd->fin || p_info_hd->syn) {
            if (p_info_hd->payload_len > 0) {
                if (studio_get_file_size(cur_bf.response_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
                    printable_len = studio_unprintable_to_space((void*)data, p_info_hd->payload_len, printable_data, sizeof(printable_data));
                    studio_handle_olddata(cur_bf.response_fname);
                    studio_write_file(cur_bf.response_fname, printable_data, printable_len);
                } else {
                    TEST_PRINT("studio_get_file_size(cur_bf.request_fname) >= REQUEST_RESPONSE_BUFFER_LIMIT");
                }
            }

            /* 处理已缓存内容 */
            if (studio_get_file_size(cur_bf.request_fname) > 0
                || studio_get_file_size(cur_bf.response_fname) > 0) {
                studio_push_into_db(&cur_bf, p_info_hd, p_info);
            }
            return 0;
        }

        /* 单纯的ack包 */
        if (0 == p_info_hd->payload_len) {
            return 0;
        }

        /* 缓存response包 */
        if (studio_get_file_size(cur_bf.response_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
            printable_len = studio_unprintable_to_space((void*)data, p_info_hd->payload_len, printable_data, sizeof(printable_data));
            studio_handle_olddata(cur_bf.response_fname);
            studio_write_file(cur_bf.response_fname, printable_data, printable_len);
        } else {
            TEST_PRINT("studio_get_file_size(cur_bf.request_fname) >= REQUEST_RESPONSE_BUFFER_LIMIT");
        }
    }

    return 0;
}
