#include<ctype.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include"audit_api.h"
#include"audit_database_sql.h"

#define STUDIO_ITEM_MAX 100 /* �����ݿ���ļ��������������ֵ */
#define REQUEST_RESPONSE_BUFFER_LIMIT (10*1024*1024) /* �����ļ���С���ܳ�10M */

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

/* ��Щ����������Ϊ�����Ĺؼ�����ͬ������Ҫ����д�� */
/* Part1: �ò������ͣ�����28�ַ�d��ǰ׺ */
#define STUDIO_ClassList                    10 //class״̬����
#define STUDIO_GetCurrentTimeStamp          11 //ʱ�������
#define STUDIO_IsGenerated                  12 //�ļ�״̬ѯ�ʣ��ļ�������׼������
#define STUDIO_GetSpecialListByStream       13 //������Ϣ�б�ѯ�ʣ��ļ�������׼������
#define STUDIO_Exists                       14 //�ļ�����״̬ѯ�ʣ��ļ�������׼������
#define STUDIO_OpenId                       15 //client���ļ�ʱ������֮һ
#define STUDIO_Name                         16 //��/�����ļ�ʱ�������ļ������ϴ��ļ���
#define STUDIO_LanguageModeGet              17 //���ļ�ǰ��ȡ����ģʽ
#define STUDIO_Code_S2C                     18 //���ļ�ʱ��server��client���Ͱ�����
#define STUDIO_Code_C2S                     19 //�����ļ�ʱ��client��server���Ͱ�����
#define STUDIO_UserType                     20 //�û����Ͳ�ѯ���򿪣������ļ��Ȳ���ʱ����
#define STUDIO_LockClass                    21 //�޸�class����ǰ����class����
#define STUDIO_UnlockClass                  22 //�޸���class����󣬸�class����
#define STUDIO_Lock                         23 //�޸�csp,inc,mac,MVB,js,xml,css,bas�ļ�ǰ����
#define STUDIO_Unlock                       24 //�޸�csp,inc,mac,MVB,js,xml,css,bas�ļ������
#define STUDIO_SaveDefinition               25 //����class
#define STUDIO_Save                         26 //����csp,inc,mac,MVB,js,xml,css,bas�ļ�
#define STUDIO_DeleteClassDefinition        27 //ɾ��class
#define STUDIO_Delete                       28 //ɾ��csp,inc,mac,MVB,js,xml,css,bas�ļ�
#define STUDIO_NameSet                      29 //�ļ�������
#define STUDIO_AddItem                      30 //��Project�����class��csp,inc,mac,MVB,js,xml,css,bas�ļ�
#define STUDIO_RemoveItem                   31 //��Project��ɾ��class��csp,inc,mac,MVB,js,xml,css,bas�ļ�
#define STUDIO_StudioOpenDialog             32 //��һ���Ի���
#define STUDIO_IsClassnameConflict          33 //�Ƿ��������½��������Ȳ����еĲ�ѯ��
#define STUDIO_GetStudioInfo                34 //��ѯserver�汾����Ϣ
#define STUDIO_SourceControlCreate          35 //�������ƽӿ�?
#define STUDIO_CompileClass                 36 //����cls
#define STUDIO_Compile                      47 //����csp
#define STUDIO_DocumentHistoryAdd           38 //������ʷ��¼
#define STUDIO_DocumentHistoryGet           39 //�����ʷ��¼
#define STUDIO_GetDefinition                40 //���class����
#define STUDIO_StudioTempateMgrTemplates    41 //���ƽ̨��Ϣ
#define STUDIO_GetAppName                   42 //���Ӧ�ö�����
#define STUDIO_New                          43 //�½�csp
#define STUDIO_GetWebServerPort             44 //���Web Server�˿�
#define STUDIO_AutoComplete                 45 //�Զ������ļ�

/* Part2: �����⼸�����ͣ�ǰ׺������28�ַ��� */
//#define STUDIO_norecursive                  110 //δ֪, �ȷŷ�, ע�⣬����ǰ׺��19�ֽ�
#define STUDIO_Compile_Following            111 //����cls�������������6�����ҵ�14�ֽڵ������
//#define STUDIO_Login                        112 //��¼����

/* Part3: δ֪���� */
#define STUDIO_Unknown                      0 //δ֪����

/* keywords������ֵ��Ӧ�� */
struct kw_type_table {
    char keywords[128];
    int rv; /* ����ֵ */
};

/* ��ִ�еĶ������࣬���ش�������͵������� */
#define PREFIX_SIZE 28 /* �����������28�ֽڵ�ǰ׺�� */
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

    /* ��¼��, �ؼ���ǰ��11���ո� */
    //if (n>strlen("           CI") && !memcmp(src+1, "           CI", strlen("           CI"))) {
    //    return STUDIO_Login;
    //}

    /* �����������28�ֽڵ�ǰ׺�ģ���Ҳ�������Ĳ���32�ֽڵĽ����������������cls
     * �󣬻���6�����ҵ�14�ֽڵ������ */
    if (n <= PREFIX_SIZE) {
        if (14 == n && 'B'==src[12] && 'L'==src[13]) return STUDIO_Compile_Following;
        else return STUDIO_Unknown;
    }

    /* �ǿո��ַ�����5��������ǰ׺ */
    non_space = 0;
    for (i=0; i < ((n>=28)?28:n); i++) {
        if (src[i] != ' ') non_space++;
    }
    if (non_space > 5) return STUDIO_Unknown;

    pos = src + PREFIX_SIZE;
    left = n - PREFIX_SIZE;

    for (i=0; i<37; i++) {
        if (left > strlen(ktt[i].keywords) && !memcmp(pos, ktt[i].keywords, strlen(ktt[i].keywords))) {
            /* "Code"�ؼ��ֶ�Ӧ2����� */
            if (STUDIO_Code_S2C == ktt[i].rv || STUDIO_Code_C2S == ktt[i].rv) {
                if (left-4 > 3) return STUDIO_Code_C2S;
                else return STUDIO_Code_S2C;
            }
            return ktt[i].rv;
        }
    }
    return STUDIO_Unknown;
}

/* �����ͷ��������������do_classify()���
 * �ɹ����ش������͵���������ʧ�ܷ���-1 */
int classify(char *fname) {
    int fd, n, i, unprintable;
    char buf[100] = {0}, *pos;

    fd = open(fname, O_RDWR, 0666);
    if (fd < 0) {
        TEST_PRINT("���ļ�ʧ��");
        return -1;
    }
    n = read(fd, buf, sizeof(buf)-1);/* 99���ַ��㹻�����ؼ����� */
    close(fd);
    if (n < 0) {
        TEST_PRINT("���ļ�ʧ��");
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
static char cur_flie_name[THREADS_NUM][256] = {{0}};/* ��ǰ�����ݿ���ļ�����ȫ·���� */
static char cur_dstflie_name[THREADS_NUM][256] = {{0}};
static int item_cnt[THREADS_NUM] = {0};/* ÿ�������ݿ���ļ������������ֵ */
static int fname_suffix_n[THREADS_NUM] = {0};

/* ���ļ�'fname'д��'len'�ֽڵ�'content'.���ļ��������򴴽���
 * ���سɹ�д����ֽ���, ʧ�ܷ���-1 */
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

/* ���ļ�'src_fname'����������׷д���ļ�'dst_fname'
 * ���سɹ�д���ļ�'dst_fname'���ֽ���, ���ļ�ʧ�ܷ���-1
 * ע�� - ���ñ�����֮ǰ, 2���ļ�����Ϊ�Ǵ��ڵ� */
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

/* ɾ��'str'��ͷ��ĩβ�����Ŀհ��ַ�. ԭַ���� */
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

/* ����'slen'�Ĵ�'s'��, 1���������Ķ�����ɴ�ӡ�ַ����һ���ո�, ��������浽'dst'��
 * ����'dst'�ַ����ĳ���
 * ��󽫽���ַ����Ŀ�ͷ�ͽ�β�������Ŀհ��ַ�ȥ��
 * ע�� - ǰ28���ַ��Ĳ������ַ���Ϊ�ո񣬵����ϲ����Ա���28�ַ����� */
static int studio_unprintable_to_space(char *src, int slen, char *dst, int dstlen) {
    int i=0, j=0;
    /* ǰ28���ֽڵĲ��ɴ�ӡ�ַ���Ϊ�ո��Ҳ��ܺϲ��ַ�����֤��ʼ��28�ַ�ƫ��������
     * Ϊ����Ĺؼ��ֲ�ѯ�ṩ���� */
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

/* ��ȡ'ip'�ĵ����ʽ, 'ipstr'������, 'len'��'ipstr'�Ĵ�С */
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

/* ɾ���ļ�����޸�ʱ����2������ǰ���ļ�'fname' */
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

    /* ��������չ�28+"Code"/"%Studio.ClassMgr SaveDefinition"����С���ȣ���32ʱ��
     * �����ļ� */
    if (n <= 32) return 0;

    /* �����������ǰ28���ַ��Ŀ���ʾ�ַ����� */
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

/* д�ļ�'db_fname'. �ɹ�����0, ʧ�ܷ���-1 */
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

    /* ��û����ļ����� */
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

    /* ���������ݿ�Ļ����ļ�����ȫ·�� */
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

    /* ���ʽip (cli��Ӧ �������src    ser ��Ӧdes)*/
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

    /* ȡ�����table���ֵ */
    pthread_rwlock_rdlock(&tablesTime_lock);
    strcpy(time_for_table_item, tablesTime);
    pthread_rwlock_unlock(&tablesTime_lock);

    /* ȡ�����rowkey���ֵ */
    memset(time_for_rowkey, 0, sizeof(time_for_rowkey));
#if REL_HBASE
    snprintf(time_for_rowkey, 31, "%lu", p_info_hd->audit_time);
#else
    get_audit_time(time_for_rowkey);
#endif

	pkt_type = 0;
	pkt_type = classify(bf->request_fname);
    if (pkt_type < 0) pkt_type = 0;

    /* request���� */
    sprintf(buffer,
        "rowkey=%s"
        //"rowkey=%s-00%u"
        "|colfam1:table=%s"
        "|colfam1:app_id=%u"
        "|colfam1:saveflag=%d" /* �Ƿ����ļ����ݵı�� */
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
        if (byte_wr != request_len) {//�Ƿ���Ҫ��������?
            PRINT_ERR("byte_wr != request_len");
        }
    }

    /* response���� */
    sprintf(buffer,
        "\n"
        "rowkey=%s"
        //"rowkey=%s-00%u"
        "|colfam1:table=%s"
        "|colfam1:app_id=%u"
        "|colfam1:saveflag=%d" /* �Ƿ����ļ����ݵı�� */
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
        if (byte_wr != response_len) {//�Ƿ���Ҫ��������?
            PRINT_ERR("byte_wr != response_len");
        }
    }
    studio_write_file(cur_flie_name[thread_id], "\n", 1);

    /* ɾ��request, response�ļ� */
    unlink(bf->request_fname);
    unlink(bf->response_fname);

    item_cnt[thread_id]++;

    /* �ƶ������ݿ���ļ� */
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

/* ����һ��request/response����� - rst/fin/syn������һ�������ݵ�request�� */
u_int32_t audit_studio(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char * data) {
    char printable_data[4096] = {0};
    struct buffer_files cur_bf;
    int printable_len;

    /* �����ظ��� */
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

    /* request/response�����ļ��� */
    sprintf(cur_bf.request_fname, "/dev/shm/studio_tsi_tmp/request_%u_%d", p_info_hd->hash, p_info_hd->thrid);
    sprintf(cur_bf.response_fname, "/dev/shm/studio_tsi_tmp/response_%u_%d", p_info_hd->hash, p_info_hd->thrid);

    if (!(p_info_hd->isin)) {/* request��(�����������server����) */
        if (p_info_hd->rst || p_info_hd->fin || p_info_hd->syn) {
            /* �����ѻ������� */
            if (studio_get_file_size(cur_bf.request_fname) > 0
                || studio_get_file_size(cur_bf.response_fname) > 0) {
                studio_push_into_db(&cur_bf, p_info_hd, p_info);
            }
            return 0;
        }

        /* ������ack�� */
        if (0 == p_info_hd->payload_len) {
            return 0;
        }

        /* response������ǿ�, ��ʾ�����صĿ�ʼ, �������һ��request/response */
        if (studio_get_file_size(cur_bf.response_fname) > 0) {
            studio_push_into_db(&cur_bf, p_info_hd, p_info);
        }

        /* ����request�� */
        if (studio_get_file_size(cur_bf.request_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
            printable_len = studio_unprintable_to_space((void*)data, p_info_hd->payload_len, printable_data, sizeof(printable_data));
            studio_handle_olddata(cur_bf.request_fname);
            studio_write_file(cur_bf.request_fname, printable_data, printable_len);
        } else {
            TEST_PRINT("studio_get_file_size(cur_bf.request_fname) >= REQUEST_RESPONSE_BUFFER_LIMIT");
        }
    } else { /* response�� */
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

            /* �����ѻ������� */
            if (studio_get_file_size(cur_bf.request_fname) > 0
                || studio_get_file_size(cur_bf.response_fname) > 0) {
                studio_push_into_db(&cur_bf, p_info_hd, p_info);
            }
            return 0;
        }

        /* ������ack�� */
        if (0 == p_info_hd->payload_len) {
            return 0;
        }

        /* ����response�� */
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
