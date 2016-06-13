/*gcc audit_studio_main.c -o audit_studio_main -g -Wall */

#include<ctype.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<sys/types.h>
#include<dirent.h>
#include<unistd.h>
#include<stdlib.h>
#include <fcntl.h>
#include<sys/stat.h>

//#include"audit_api.h"
#include"audit_database_sql.h"
#include"csp_deal.h"
#include"csp_policy.h"

int processId = 0;

/*************************
	read from /dev/shm/dir/ files
   	get Sip Dip Sport Dport  appid  userid  time request_content response_content
*/

/*************************
	drop unlookable content0
	and construct  sqlcontent
*/

/*************************
	write sqlcontent into sqlfile
*/

/*************************
	from file path:/dev/shm/dir/
	to     file path:/data/audit/sql_tmp/
	termination file path:/data/audit/sql/
*/

#define DEBUG_OPEN 0
#if DEBUG_OPEN
#define DEBUG_LOG(s) fprintf(stderr, "DEBUG : <%s> : %d : %s\n", __FILE__, __LINE__, (s))
#else
#define DEBUG_LOG(s)
#endif
#define PRINT_ERR_MSG(s) fprintf(stderr, "ERROR : <%s> : %d : %s\n", __FILE__, __LINE__, (s))

/* 缓存文件名的第一个字段的值，区分应用 */
#define SQLSERVER_TMP_ID_PREFIX "10_"
#define CACHE_STUDIO_TMP_ID_PREFIX "20_"
#define CACHE_HIS_PORTAL_TMP_ID_PREFIX "100_"

#define SQLSERVER_TMP_ID 10
#define CACHE_STUDIO_TMP_ID 20
#define CACHE_HIS_PORTAL_TMP_ID 100

typedef struct {
    unsigned short sport;
    unsigned short dport;
    unsigned short app_id;
    unsigned short user_id;
    unsigned int sip;
    unsigned int dip;
    char smac[64];
    char dmac[64];
    int reqsize;
    int respsize;
    unsigned int seq;
    unsigned long time;
    char *data;
    char * request;
    char * response;
    int processId;
}TITLE_CONTNET_TYPE;

#define ITEM_MAX 1 /* 填数据库的文件的内容条数最大值 */

/* mongodb中app_id和应用对应关系 */
#define APP_ID_CACHE_STUDIO 2

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

char resultBuf[10485760];
static char cur_flie_name[256] = {0};/* 当前填数据库的文件的完全路径名 */
static char cur_dstflie_name[256] = {0};
static int item_cnt = 0;/* 每个填数据库的文件的条数的最大值 */
static int fname_suffix_n = 0;

/* 包执行的动作分类，返回代表包类型的正整数 */
#define PREFIX_SIZE 28 /* 大多数包是有28字节的前缀的 */
typedef struct {
    char keywords[128];
    int rv; /* 类型值 */
} KEY_VALUE_PAIR;

int do_classify(char *src, int n) {
    int i, non_space, left;
    char *pos;
    KEY_VALUE_PAIR ktt[100] = {
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

/* 向文件'fname'写入'len'字节的'content'.若文件不存在则创建它
 * 返回成功写入的字节数, 失败返回-1 */
int studio_write_file(char *fname, char *content, int len) {
    int fd, nwr;

    if (NULL==fname || NULL==content || len<=0) return -1;
    fd = open(fname, O_CREAT|O_RDWR|O_APPEND, 0666);
    if (fd < 0) {
        PRINT_ERR_MSG("Fails to open the file.");
        return -1;
    }
    if ((nwr = write(fd, content, len)) < 0) {
        PRINT_ERR_MSG("Fails to write the data into file.");
        close(fd);
        return -1;
    }
    close(fd);

    return nwr;
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

    if (NULL==src || slen<=0 || NULL==dst || dstlen<=0) return 0;
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

    return studio_trim(dst);
    //return j;
}

/* 获取'ip'的点分形式, 'ipstr'保存结果, 'len'是'ipstr'的大小
   'ip'是网络序 */
static int getStrIp(u_int32_t ip, char *ipstr, size_t len) {
    struct in_addr addr;
    addr.s_addr = ip;
    inet_ntop(AF_INET, &addr, ipstr, len);
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

const char days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

struct tm *dhcc_localtime(time_t time, long time_zone, struct tm *tm_time) {
	unsigned  int pass_4year, hour_per_year;

	time = time + time_zone*60*60;
	if(time < 0) {
	   time = 0;
	}

	tm_time->tm_sec=(int)(time % 60);
	time /= 60;
	tm_time->tm_min=(int)(time % 60);
	time /= 60;
	pass_4year=((unsigned int)time / (1461 * 24));
	tm_time->tm_year=(pass_4year << 2) + 70;
	time %= 1461 * 24;

	for (;;)
	{
	    hour_per_year = 365 * 24;

	    if ((tm_time->tm_year & 3) == 0)
		{
	        hour_per_year += 24;
		}
	    if (time < hour_per_year)
		{
	        break;
		}
	    tm_time->tm_year++;
	    time -= hour_per_year;
	}

	tm_time->tm_hour=(int)(time % 24);
	time /= 24;
	time++;
	if ((tm_time->tm_year & 3) == 0)
	{
		if (time > 60)
		{
			time--;
		}
		else
		{
			if (time == 60)
			{
				tm_time->tm_mon = 1;
				tm_time->tm_mday = 29;
			}
		}
	}
	for (tm_time->tm_mon = 0; days[tm_time->tm_mon] < time; tm_time->tm_mon++)
	{
	      time -= days[tm_time->tm_mon];
	}
	tm_time->tm_mday = (int)(time);

    	return tm_time;
}

void log_time(struct tm *t_time)
{
	if((t_time->tm_hour) >=24) {
		t_time->tm_mday += 1;
		t_time->tm_hour -= 24;
	}

	switch(t_time->tm_mon) {
		case 0 ... 10:
			if(t_time->tm_mday > days[t_time->tm_mon]) {
				t_time->tm_mon += 1;
				t_time->tm_mday = 1;
			}

		case 11:
			{
				if(t_time->tm_mday > days[t_time->tm_mon]) {
					t_time->tm_mon = (t_time->tm_mon) - 11;
					t_time->tm_mday = (t_time->tm_mday) - (days[t_time->tm_mon]);
					t_time->tm_year += 1;
				}
			}

		default:
			break;
	}
}


/* 获得"2014-08"格式时间 */
void getTime3(char *times, size_t maxsize){
	time_t now;
	struct tm  timenow;

	time(&now);
	dhcc_localtime(now, 8, &timenow);
	log_time(&timenow);
	snprintf(times, maxsize, "%04d_%02d", timenow.tm_year+1900, timenow.tm_mon+1);
}

/* 文件的上次修改时间到当前时间，过了'secs'秒，则返回1，否则返回0 */
int IsOld(char *filePath, int secs) {
    time_t t;
	int retval;
    struct stat ft;

    retval = time(&t);
    if (retval < 0) return 1;

    retval = stat(filePath, &ft);
    if (retval < 0) return 1;

    //fprintf(stderr, "now=%ld, st_mtime=%ld, dif=%ld", (long)t, (long)(ft.st_mtime), (long)t - (long)(ft.st_mtime));
    if ((long)t - (long)(ft.st_mtime) > secs) return 1;
    return 0;
}

int GetPolicyTime(unsigned long time){
	int  time_sec=0;
	int  time_min=0;
	const int time_zone = 8*60;
	const int week_min = 7*24*60;
	const int day_min = 24*60;
	const int monday_start = 4*24*60;
	int res;
	time_sec = time/1000000;
	time_min = time_sec/60;

	res = (time_min + time_zone - monday_start)%week_min;

	return res;
}

CSP_FILE_INFO info;

int audit_studio(TITLE_CONTNET_TYPE *data) {
    char buffer[4096]={0}, time_for_table_item[64]={0},
         srcipstr[32]={0}, dstipstr[32]={0}, srcmac[64]={0}, dstmac[64]={0};
    int byte_wr, nParse;
    int pkt_type;

    /* 丢弃不是cache studio的情况 */
    if (data->app_id != CACHE_STUDIO_TMP_ID) {
        fprintf(stderr, "Wrong function.(app_id=%d)\n", data->app_id);
        return 0;
    }

    /* 取"2015-08"格式时间 */
	getTime3(time_for_table_item, sizeof(time_for_table_item)-1);

    /* 生成填数据库的缓存文件的完全路径 */
    if (0==item_cnt || '\0'==cur_flie_name[0] || '\0'==cur_dstflie_name[0]) {
        item_cnt = 0;
        sprintf(cur_flie_name,
            SQL_TMP
            STUDIO_SQL_FILE
            "%lu_%d_%d", data->time, fname_suffix_n, data->processId);
        sprintf(cur_dstflie_name,
            SQL_PATH
            STUDIO_SQL_FILE
            "%lu_%d_%d", data->time, fname_suffix_n, data->processId);
        fname_suffix_n++;
    }

    /* 解析 */
	pkt_type = 0;
	pkt_type = do_classify(data->request, data->reqsize);
    if (pkt_type < 0) pkt_type = 0;
    nParse = studio_unprintable_to_space(data->request, data->reqsize, resultBuf, sizeof(resultBuf)-1);
    if (nParse < 30) return -1;


    /* 确定要填入库文件后，再处理ip/mac */
    /* 点分式ip (cli对应 表里面的src    ser 对应des)*/
    getStrIp(ntohl(data->sip), srcipstr, 32);
    getStrIp(ntohl(data->dip), dstipstr, 32);

    /* mac */
    sprintf(srcmac, "%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",
                    data->smac[0], data->smac[1], data->smac[2], data->smac[3], data->smac[4], data->smac[5],
                    data->smac[6], data->smac[7], data->smac[8], data->smac[9], data->smac[10], data->smac[11]);
    sprintf(dstmac, "%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",
                    data->dmac[0], data->dmac[1], data->dmac[2], data->dmac[3], data->dmac[4], data->dmac[5],
                    data->dmac[6], data->dmac[7], data->dmac[8], data->dmac[9], data->dmac[10], data->dmac[11]);

    /* 策略匹配 */
	CACHE_POLICY_CONF *policy = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm();
	if(NULL == policy){
        PRINT_ERR_MSG("Fails to get CACHE_POLICY_CONF");
		return 0;
	}
    sprintf(info.cspHead.policytime, "%lu", GetPolicyTime(data->time));
    strcpy(info.cspHead.userip, srcipstr);
    info.type = 2;
    if (0 == policy_match(&info, policy)) {
        return 0;
    }

    /* request缓存
       注 - 'operation_command'的偏移量不计算偏移内容后的\n */
    sprintf(buffer,
        "rowkey=%lu"
        "|colfam1:table=%s"
        "|colfam1:app_id=%u"
        "|colfam1:saveflag=%d" /* 是否是文件内容的标记 */
        "|colfam1:src_ip=%s"
        "|colfam1:src_mac=%s"
        "|colfam1:src_port=%hu"
        "|colfam1:dst_ip=%s"
        "|colfam1:dst_mac=%s"
        "|colfam1:dst_port=%hu"
        "|colfam1:user_id=%hu"
        "|colfam1:operation_command=%d"
        "|colfam\n",
        data->time, time_for_table_item, APP_ID_CACHE_STUDIO, pkt_type, srcipstr, srcmac, data->sport,
        dstipstr, dstmac, data->dport, data->user_id, nParse);
    DEBUG_LOG(buffer);
    DEBUG_LOG(resultBuf);

    studio_write_file(cur_flie_name, buffer, strlen(buffer));
    if (nParse > 0) {
        byte_wr = studio_write_file(cur_flie_name, resultBuf, nParse);
        if (byte_wr != nParse) {
            PRINT_ERR_MSG("byte_wr != nParse");
        }
    }

    /* response包处理 */
    memset(resultBuf, 0, sizeof(resultBuf));
    nParse = studio_unprintable_to_space(data->response, data->respsize, resultBuf, sizeof(resultBuf)-1);
    if (nParse < 0) {
        nParse = 0;
    }

    /* response缓存 */
    sprintf(buffer,
        "\n"
        "rowkey=%lu"
        "|colfam1:table=%s"
        "|colfam1:app_id=%u"
        "|colfam1:saveflag=%d" /* 是否是文件内容的标记 */
        "|colfam1:src_ip=%s"
        "|colfam1:src_mac=%s"
        "|colfam1:src_port=%hu"
        "|colfam1:dst_ip=%s"
        "|colfam1:dst_mac=%s"
        "|colfam1:dst_port=%hu"
        "|colfam1:user_id=%hu"
        "|colfam1:response_content=%d"
        "|colfam\n",
        data->time, time_for_table_item, APP_ID_CACHE_STUDIO, pkt_type, srcipstr, srcmac, data->sport,
        dstipstr, dstmac, data->dport, data->user_id, nParse);
    DEBUG_LOG(buffer);
    DEBUG_LOG(resultBuf);

    studio_write_file(cur_flie_name, buffer, strlen(buffer));
    if (nParse > 0) {
        byte_wr = studio_write_file(cur_flie_name, resultBuf, nParse);
        if (byte_wr != nParse) {
            PRINT_ERR_MSG("byte_wr != nParse");
        }
    }
    studio_write_file(cur_flie_name, "\n", 1);

    /* 移动填数据库的文件 */
    item_cnt++;
    if (item_cnt>=ITEM_MAX || IsOld(cur_flie_name, 60)) {
        rename(cur_flie_name, cur_dstflie_name);
        unlink(cur_flie_name);
        item_cnt = 0;
        cur_flie_name[0] = '\0';
        cur_dstflie_name[0] = '\0';
    }
    return 0;
}

/* ======================================================================================================== */
#define CMDLEN  10485760
char RequestFileName[1000];
char ResponseFileName[1000];
char cmd1[CMDLEN];
char cmd2[CMDLEN];
char fileContentRequest[CMDLEN];
char fileContentResponse[CMDLEN];

void GetValues(TITLE_CONTNET_TYPE *tContent, char *filename){
    /* 例子 - 10_2887326051_000fe25c06a0_64234_168466306_80c16ef872cd_1433_0_5_1440916826045169_request */
    sscanf(filename, "%hu_%u_%[^_]_%hu_%u_%[^_]_%hu_%u_%hu_%lu_request",
                    &tContent->app_id, &tContent->sip, tContent->smac, &tContent->sport,
                    &tContent->dip, tContent->dmac, &tContent->dport, &tContent->seq,
                    &tContent->user_id, &tContent->time);
}

void GetValuesFromFullName(TITLE_CONTNET_TYPE *tContent, char *filename){
    /* 例子 - /dev/shm/1/10_2887326051_000fe25c06a0_64234_168466306_80c16ef872cd_1433_0_5_1440916826045169_request */
    sscanf(filename, "/dev/shm/%d/%hu_%u_%[^_]_%hu_%u_%[^_]_%hu_%u_%hu_%lu_request",
                    &tContent->processId, &tContent->app_id, &tContent->sip, tContent->smac, &tContent->sport,
                    &tContent->dip, tContent->dmac, &tContent->dport, &tContent->seq,
                    &tContent->user_id, &tContent->time);
}

static long GetFileSize(char *filename) {
    struct stat buf;
    if(stat(filename, &buf)<0) {
        return -1;
    }
    return (long)buf.st_size;
}

unsigned int FileInit(char * filename,TITLE_CONTNET_TYPE *tContent, char *fileContent, int maxsize){
	FILE * cspFp = NULL;
	long fsize =0;
	long readsize = 0;
	memset(fileContent,0,CMDLEN*sizeof(char));

	fsize = GetFileSize(filename);
	if(fsize <=0){
		return -1;
	}
	if(fsize > CMDLEN*sizeof(char)){
		return -1;
	}

	cspFp = fopen(filename,"r+");
	if(!cspFp){
		#if CSP_DEBUG
		printf("open %s file failed. \n",filename);
		#endif
		return  -1;
	}

	readsize = fread(fileContent,1,fsize,cspFp);
	if(readsize != fsize){
		#if CSP_DEBUG
		printf("fread error.\n");
		#endif
		fclose(cspFp);
		return -1;
	}

	fclose(cspFp);
	cspFp = NULL;

	return fsize;


}

void parse_request_file(TITLE_CONTNET_TYPE *tContent,char * requestPath){
	tContent->reqsize=FileInit(requestPath,tContent, fileContentRequest, sizeof(fileContentRequest)-1);
}

void parse_response_file(TITLE_CONTNET_TYPE *tContent,char * responsePath){
	tContent->respsize=FileInit(responsePath,tContent, fileContentResponse, sizeof(fileContentResponse)-1);
}

/* 使当前进程转为后台运行 */
int NC_daemon_audit(void) {
	pid_t pid;
	int ret;
	if ((pid = fork()) < 0) {
		return -1;
	} else if (pid != 0) {
		exit(0); /* parent goes bye-bye */
	}

	if((ret=setsid()) < 0) { /* become session leader */
		fprintf(stderr, "unable to setsid.\n");
        exit(0);
	}
	 setpgrp();
	 return 0;
}

#if 0
#define REQUEST_RESPONSE_CACHE_PATH "/dev/shm/"
char helpStr[] = "\n\n*************************************************************\n"
                 "USAGE:\n"
                 "audit_studio_process <id> <daemonFlag>\n"
                 "<id>[0...9]:       Specify which dir to read.\n"
                 "<daemonFlag>[0/1]: Set daemon mode, 1 for YES, 0 for NO\n"
                 "*************************************************************\n\n";
int main(int argc,char **argv){
	DIR *srcDir;
	struct dirent *srcDp;
	int id, flag;
	char *pos;
	TITLE_CONTNET_TYPE tContent;
    char dirName[1000], fileBaseName1[1000], fileBaseName2[1000];
    int exist;

    if (argc != 3) {
        perror(helpStr);
        exit(0);
    } else {
        id = atoi(argv[1]);
        flag = atoi(argv[2]);

        if (id<0 || id >9 || (flag!=1 && flag!=0)) {
            perror(helpStr);
            exit(0);
        }
    }

    /* 后台运行 */
    if (1 == flag) NC_daemon_audit();

    sprintf(dirName, REQUEST_RESPONSE_CACHE_PATH "%d/", id);

	while(1){
		if (NULL == (srcDir = opendir(dirName))) {
			PRINT_ERR_MSG("opendir error");
			return -1;
		}

		while ((srcDp = readdir(srcDir)) != NULL) {
			if(strcmp(srcDp->d_name,".") == 0 || strcmp(srcDp->d_name,"..") == 0){
				continue;
			}

            /* 跳过不是studio的文件 */
			if(memcmp(srcDp->d_name, CACHE_STUDIO_TMP_ID_PREFIX, strlen(CACHE_STUDIO_TMP_ID_PREFIX))!=0){
				continue;
			}

			memset(&tContent, 0, sizeof(TITLE_CONTNET_TYPE));
            tContent.processId = id;

			memset(cmd1, 0, sizeof(cmd1));
			memset(cmd2, 0, sizeof(cmd2));
			memset(RequestFileName, 0, sizeof(RequestFileName));
			memset(ResponseFileName, 0, sizeof(ResponseFileName));

            strcpy(fileBaseName1, srcDp->d_name);
            if ((pos = strstr(fileBaseName1, "_request")) != NULL) {
                *pos = '\0';
                sprintf(fileBaseName2, "%s_response", fileBaseName1);
                *pos = '_';
                sprintf(RequestFileName, "%s%s", dirName, fileBaseName1);
                sprintf(ResponseFileName, "%s%s", dirName, fileBaseName2);
                GetValues(&tContent, fileBaseName1);
            } else if ((pos = strstr(fileBaseName1,"_response")) != NULL) {
                *pos = '\0';
                sprintf(fileBaseName2, "%s_request", fileBaseName1);
                *pos = '_';
                sprintf(RequestFileName,"%s%s", dirName, fileBaseName2);
                sprintf(ResponseFileName,"%s%s", dirName, fileBaseName1);
                GetValues(&tContent, fileBaseName2);
            } else {
                sprintf(fileBaseName2, "%s%s", dirName, fileBaseName1);
                unlink(fileBaseName2);
            }

			tContent.request=fileContentRequest;
			tContent.response=fileContentResponse;

			parse_request_file(&tContent, RequestFileName);
			parse_response_file(&tContent, ResponseFileName);

            /* 只有一个存在时，若该存在的文件超时，则删除 */
            if((exist=access(RequestFileName,0))!=0){
                if (IsOld(ResponseFileName, 120) == 1) {
                    unlink(ResponseFileName);
                }
                continue;
            }

            if((exist=access(ResponseFileName,0))!=0){
                if (IsOld(RequestFileName, 120) == 1) {
                    unlink(RequestFileName);
                }
                continue;
            }

			audit_studio(&tContent);

            unlink(RequestFileName);
            unlink(ResponseFileName);
		}
        closedir(srcDir);
		usleep(1000);
	}

	return 0;
}
#else
char helpStr2[] = "\n\n*************************************************************\n"
                 "USAGE:\n"
                 "audit_studio_process fileFullName <daemonFlag>\n"
                 "fileFullName:       Specify which file to parse.\n"
                 "<daemonFlag>[0/1]: Set daemon mode, 1 for YES, 0 for NO\n"
                 "*************************************************************\n\n";
int main(int argc, char **argv) {
    int deamonOn, exist, i;
    char *pos, dirName[1024], fileBaseName1[1024], fileBaseName2[1024];
    TITLE_CONTNET_TYPE tContent;

    if (argc < 2) {
        perror(helpStr2);
        exit(0);
    }
    deamonOn = 1;
    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "1") == 0) {
            deamonOn = 1;
        } else if (strcmp(argv[i], "0") == 0) {
            deamonOn = 0;
        } else {
            strcpy(fileBaseName1, argv[i]);
        }
    }
    //if (deamonOn) NC_daemon_audit();

    memset(&tContent, 0, sizeof(TITLE_CONTNET_TYPE));
    memset(cmd1, 0, sizeof(cmd1));
    memset(cmd2, 0, sizeof(cmd2));
    memset(RequestFileName, 0, sizeof(RequestFileName));
    memset(ResponseFileName, 0, sizeof(ResponseFileName));

    if ((pos = strstr(fileBaseName1, "_request")) != NULL) {
        *pos = '\0';
        sprintf(fileBaseName2, "%s_response", fileBaseName1);
        *pos = '_';
        sprintf(RequestFileName, "%s", fileBaseName1);
        sprintf(ResponseFileName, "%s", fileBaseName2);
        GetValuesFromFullName(&tContent, fileBaseName1);
    } else if ((pos = strstr(fileBaseName1,"_response")) != NULL) {
        *pos = '\0';
        sprintf(fileBaseName2, "%s_request", fileBaseName1);
        *pos = '_';
        sprintf(RequestFileName,"%s", fileBaseName2);
        sprintf(ResponseFileName,"%s", fileBaseName1);
        GetValuesFromFullName(&tContent, fileBaseName2);
    } else {
        fprintf(stderr, "<%s>:%d:No request/resonse in file name(%s), remove it.\n", __FILE__, __LINE__,fileBaseName1);
        unlink(fileBaseName1);
    }

    tContent.request=fileContentRequest;
    tContent.response=fileContentResponse;

#if DEBUG_OPEN
    DEBUG_LOG("***********************************************************");
    fprintf(stderr, "<%s>:%d:"
            "id = %d, app_id=%hu, sip=%u, sport=%hu, smac=%s, dip=%u, dport=%hu, dmac=%s, seq=%u, user_id=%hu, time=%lu\n",
            __FILE__, __LINE__,
            tContent.processId, tContent.app_id, tContent.sip, tContent.sport, tContent.smac,
            tContent.dip, tContent.dport, tContent.dmac, tContent.seq,
            tContent.user_id, tContent.time);
#endif
    parse_request_file(&tContent, RequestFileName);
    parse_response_file(&tContent, ResponseFileName);

    /* 只有一个存在时，若该存在的文件超时，则删除
    if((exist=access(RequestFileName,0))!=0){
        if (IsOld(ResponseFileName, 120) == 1) {
            unlink(ResponseFileName);
        }
        return 0;
    }

    if((exist=access(ResponseFileName,0))!=0){
        if (IsOld(RequestFileName, 120) == 1) {
            unlink(RequestFileName);
        }
        return 0;
    }
    */

    audit_studio(&tContent);

    unlink(ResponseFileName);
    unlink(RequestFileName);
    return 0;
}
#endif
