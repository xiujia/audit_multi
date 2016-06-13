#include<ctype.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

#include"audit_api.h"
#include"audit_database_sql.h"
#include"./TDS_parser.c"

#define ITEM_MAX 20 /* �����ݿ���ļ��������������ֵ */
#define REQUEST_RESPONSE_BUFFER_LIMIT (10*1024*1024) /* �����ļ���С���ܳ�10M */
#define SQLSERVER_REQUEST_RESPONSE_TMP "/dev/shm/sqlserver_tmp/"

#if __NEW_TEST
#define DEBUG_LOG(s) \
do {\
    fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, (s)); \
}while(0)
#else
#define DEBUG_LOG(s)
#endif

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
static time_t last_time = 0;


/* ɾ��'str'��ͷ��ĩβ�����Ŀհ��ַ�. ԭַ����
   ���ؽ�����ĳ��� */
static int trim(char *str) {
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

static int get_file_size(char *filename) {
    struct stat buf;
    if(stat(filename, &buf)<0) {
        return -1;
    }
    return (int)buf.st_size;
}

/* ����'slen'�Ĵ�'s'��, 1���������Ķ�����ɴ�ӡ�ַ����һ���ո�, ��������浽'dst'��
 * ����'dst'�ַ����ĳ��� */
static int unprintable_to_space(char *src, int slen, char *dst, int dst_maxsize) {
    int i=0, j=0;

    memset(dst, 0, dst_maxsize);
    while (i<slen && j<(dst_maxsize-1)) {
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

/* ���ļ�'fname'д��'len'�ֽڵ�'content'.���ļ��������򴴽���
 * ���سɹ�д����ֽ���, ʧ�ܷ���-1 */
static int write_file(char *fname, char *content, int len) {
    int fd, nwr;

    if (NULL==fname || NULL==content || len<=0) { DEBUG_LOG("NULL==fname || NULL==content || len<=0**********"); return -1; }
    fd = open(fname, O_CREAT|O_RDWR|O_APPEND, 0666);
    if (fd < 0) {
        DEBUG_LOG("open file failed**********");
        return -1;
    }

    if ((nwr = write(fd, content, len)) < 0) {
        DEBUG_LOG("write file failed**********");
        close(fd);
        return -1;
    }
    close(fd);

    return nwr;
}

/* �����ļ�'fname'�ĵ�1���ֽ� */
static unsigned char get_first_byte_from_file(char *fname) {
    unsigned char c;
    int fd, nrd;

    fd = open(fname, O_RDONLY);
	if (fd < 0) { DEBUG_LOG("open file failed**********"); return 0xFF; }
    nrd = read(fd, &c, 1);
    if (nrd <=0) { DEBUG_LOG("read file failed**********"); return 0xFF; }
    close(fd);
    return c;
}

static int write_file_tds(char *fname, char *content, int len, int is_request_flag) {
    int fd, sum, retval;
    char tmpbuf[4096*10] = {0}, tds_str[4096*10] = {0};
    unsigned char type;
    int fsize = get_file_size(fname);

    if (NULL==fname || NULL==content || len<=0) { DEBUG_LOG("NULL==fname || NULL==content || len<=0**********"); return -1; }

    fd = open(fname, O_CREAT|O_WRONLY|O_APPEND, 0666);
    if (fd < 0) { fprintf(stderr, "%s:%d:Open file fails(%d)\n", __FILE__, __LINE__, errno); return -1; }
    sum = 0;

    /* ֻ������һ������tdsЭ�� */
    if (1 == is_request_flag && fsize <= 0) {
        type = TDS_parser(content, len, tds_str, sizeof(tds_str)-1);
        switch (type) {
            case Pre_Login_flag:
            case Pre_TDS7_Login_flag:
            case Federated_Authentication_Token_flag:
            case RPC_flag:
            case Attention_signal_flag:
            case Transaction_manager_request_flag:
            case other_flag:
                close(fd);
                return 0;
            case 0xFF:
#if __NEW_TEST
	            binary_to_hex_visible(content, len, tmpbuf, sizeof(tmpbuf)-1);
                DEBUG_LOG(tmpbuf);
#endif
                close(fd);
                return 0;
        }

        /* �ļ���1���ֽ��ǰ����͡� */
        if ((write(fd, &type, 1)) < 0) {
            DEBUG_LOG("write file failed**********");
            close(fd);
            return -1;
        }
        sum += 1;

        /* �н�������ַ���ʱ���ű������ͺͽ�������� */
        if (strlen(tds_str) > 0) {
#if __NEW_TEST
			fprintf(stderr, "%s:%d:SQL = %s\n", __FILE__, __LINE__, tds_str);
#endif
			if ((retval = write(fd, tds_str, strlen(tds_str))) < 0) {
				DEBUG_LOG("write file failed**********");
				close(fd);
				return -1;
			}
			sum += retval;

			if ((retval = write(fd, "<br>Packet=", strlen("<br>Packet="))) < 0) {
				DEBUG_LOG("write file failed**********");
				close(fd);
				return -1;
			}
			sum += retval;
        }
    }

#if __NEW_TEST
    /* ����ʮ�����Ƹ�ʽ */
    retval = binary_to_hex_visible(content, len, tmpbuf, sizeof(tmpbuf)-1);
	if ((retval = write(fd, tmpbuf, strlen(tmpbuf))) < 0) {
	    DEBUG_LOG("write file failed**********");
	    close(fd);
	    return -1;
	}
	sum += retval;

	if ((retval = write(fd, "<br>", 4)) < 0) {
	    DEBUG_LOG("write file failed**********");
	    close(fd);
	    return -1;
	}
	sum += retval;
#endif

    /* ȥ�����ķǲ������ַ�����ַ��� */
    retval = unprintable_to_space(content+8, len-8, tmpbuf, sizeof(tmpbuf)-1);
    trim(tmpbuf);
    if (strlen(tmpbuf) > 0) {
        if ((retval = write(fd, tmpbuf, strlen(tmpbuf))) < 0) {
            DEBUG_LOG("write file failed**********");
            close(fd);
            return -1;
        }
        sum += retval;
    }

    close(fd);
    return sum;
}

/* ���ļ�'src_fname'����������׷д���ļ�'dst_fname'
 * ���سɹ�д���ļ�'dst_fname'���ֽ���, ���ļ�ʧ�ܷ���-1
 * ע�� - ���ñ�����֮ǰ, 2���ļ�����Ϊ�Ǵ��ڵ� */
static int file2file_append(char *dst_fname, char *src_fname) {
    int dstfd, srcfd, nwr, nrd, success_num;
    char buf[4096];

    dstfd = open(dst_fname, O_RDWR | O_APPEND);
    if (dstfd < 0) {
        DEBUG_LOG("open file failed**********");
        return -1;
    }
    srcfd = open(src_fname, O_RDWR);
    if (srcfd < 0) {
        DEBUG_LOG("open file failed**********");
        close(dstfd);
        return -1;
    }

    success_num = 0;
    while ((nrd = read(srcfd, buf, sizeof(buf))) > 0) {
        nwr = write(dstfd, buf, nrd);
        if (nwr < 0) {
            DEBUG_LOG("write file failed**********");
            break;
        }
        success_num += nwr;
    }
    close(dstfd);
    close(srcfd);

    return success_num;
}



/* ��ȡ'ip'�ĵ����ʽ, 'ipstr'������, 'len'��'ipstr'�Ĵ�С */
static int get_ipstr(u_int32_t ip, char *ipstr, size_t len) {
    struct in_addr addr;
    addr.s_addr = ip;
    inet_ntop(AF_INET, &addr, ipstr, len);
    return 0;
}

/* �ļ�'fname'������޸�ʱ����2������ǰ����ɾ���ļ�'fname'
   ʼ�շ���0
   ע�� - ��Щ�ļ�������Ϣ�Ѿ���ʧ���ļ���������Ϣ̫�٣����ܽ������ݿ������ */
static int remove_single_oldfile(char *fname, int sec) {
    struct stat buf;
    int ret;
    time_t cur, mtime;

    if (stat(fname, &buf) < 0) return 0;
    if (time(NULL) - buf.st_mtime > sec) {
        unlink(fname);
    }
    return 0;
}

static int remove_multi_oldfiles() {
    DIR *dp;
    struct dirent *dirp;
    char dirname[]=SQLSERVER_REQUEST_RESPONSE_TMP, filename[256];

    if (0 == last_time) {
        if (time(&last_time) < 0) return -1;
        return 0;
    }

    /* ÿ��һ��ʱ�䣬����һ�ξ��ļ����� */
    if (time(NULL) - last_time < 10*60) {
        return 0;
    }

    if((dp=opendir(dirname)) == NULL) {
        return 0;
    }
    while((dirp=readdir(dp)) != NULL) {
        if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0)){
            continue;
        }
        memset(filename, 0, sizeof(filename));
        snprintf(filename, 255, "%s%s", dirname, dirp->d_name);
        remove_single_oldfile(filename, 20*60);
    }
    closedir(dp);
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
static int push_into_db(struct buffer_files *bf, struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info) {
    char buffer[4096]={0}, time_for_rowkey[32], time_for_table_item[AUDIT_TIME_LEN]={0},
         srcipstr[32]={0}, dstipstr[32]={0}, srcmac[64]={0}, dstmac[64]={0};
    u_int16_t srcport, dstport;
    int thread_id = p_info_hd->thrid;
    int request_len, response_len, byte_wr, request_exist, response_exist;
    unsigned char pkt_type;

    if (NULL==bf || NULL==p_info_hd || NULL==p_info) {
        DEBUG_LOG("<push_into_db> one or more argument is NULL**********");
        return -1;
    }
    if ('\0'==bf->request_fname || '\0'==bf->response_fname) {
        DEBUG_LOG("<push_into_db> request_fname/response_fname is/are NULL**********");
        return -1;
    }

    /* ����ļ���С */
    request_exist = response_exist = 1;
    request_len = get_file_size(bf->request_fname);
    response_len = get_file_size(bf->response_fname);
    if (request_len < 0) {
        request_exist =0;
        request_len = 0;
    }
    if (response_len < 0) {
        response_exist = 0;
        response_len = 0;
    }
    if (request_len<=5) {
        unlink(bf->request_fname);
        unlink(bf->response_fname);
        return 0;
    }

    /* appid */
	pkt_type = get_first_byte_from_file(bf->request_fname);
    switch (pkt_type) {
		case Pre_Login_flag:
        case Pre_TDS7_Login_flag:
        case Federated_Authentication_Token_flag:
        case RPC_flag:
        case Attention_signal_flag:
        case Transaction_manager_request_flag:
        case other_flag:
        case 0xFF:
	        unlink(bf->request_fname);
	        unlink(bf->response_fname);
	        return 0;
    }

    /* ���������ݿ�Ļ����ļ�����ȫ·�� */
    if (0==item_cnt[thread_id] || '\0'==cur_flie_name[thread_id][0] || '\0'==cur_dstflie_name[thread_id][0]) {
        item_cnt[thread_id] = 0;
        sprintf(cur_flie_name[thread_id],
            SQLSERVER_SQL_TMP_DIR
            SQLSERVER_PREFIX
            "%lu_%d_%d", p_info_hd->audit_time, fname_suffix_n[thread_id], thread_id);
        sprintf(cur_dstflie_name[thread_id],
            SQLSERVER_SQL_DIR
            SQLSERVER_PREFIX
            "%lu_%d_%d", p_info_hd->audit_time, fname_suffix_n[thread_id], thread_id);
        fname_suffix_n[thread_id]++;
    }

    /* ���ʽip (cli��Ӧ �������src    ser ��Ӧdes)*/
    get_ipstr(p_info_hd->cli_ip, srcipstr, 32);
    get_ipstr(p_info_hd->ser_ip, dstipstr, 32);

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

    /* ���������table���ʱ��ֵ */
    pthread_rwlock_rdlock(&tablesTime_lock);
    strcpy(time_for_table_item, tablesTime);
    pthread_rwlock_unlock(&tablesTime_lock);

    /* ���������rowkey���ʱ��ֵ */
    memset(time_for_rowkey, 0, sizeof(time_for_rowkey));
#if REL_HBASE
    snprintf(time_for_rowkey, 31, "%lu", p_info_hd->audit_time);
#else
    get_audit_time(time_for_rowkey);
#endif

    /* request���� */
    sprintf(buffer,
        "rowkey=%s"
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
        time_for_rowkey, time_for_table_item, pkt_type, 0, srcipstr, srcmac, srcport,
        dstipstr, dstmac, dstport, p_info_hd->user_id, request_len);

    write_file(cur_flie_name[thread_id], buffer, strlen(buffer));
    file2file_append(cur_flie_name[thread_id], bf->request_fname);

    /* response���� */
    sprintf(buffer,
        "\n"
        "rowkey=%s"
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
        time_for_rowkey, time_for_table_item, pkt_type, 0, srcipstr, srcmac, srcport,
        dstipstr, dstmac, dstport, p_info_hd->user_id, response_len);

    write_file(cur_flie_name[thread_id], buffer, strlen(buffer));
    file2file_append(cur_flie_name[thread_id], bf->response_fname);
    write_file(cur_flie_name[thread_id], "\n", 1);

    item_cnt[thread_id]++;

    /* �ƶ������ݿ���ļ� */
    if (ITEM_MAX == item_cnt[thread_id]) {
        DEBUG_LOG("move a file from sql_tmp to sql**********");
        rename(cur_flie_name[thread_id], cur_dstflie_name[thread_id]);
        unlink(cur_flie_name[thread_id]);
        item_cnt[thread_id] = 0;
        cur_flie_name[thread_id][0] = '\0';
        cur_dstflie_name[thread_id][0] = '\0';
    }

    unlink(bf->request_fname);
    unlink(bf->response_fname);
    return 0;
}

/* �����ݰ������򣬻��浽'files'���Ӧ���ļ��С���һ��request/response�غϽ�����
   �ѻ��������ĸûغϵ����ݣ�д�����ڲ����ݿ���ļ��С�
   ע�� - ����һ��request/response��������� - rst/fin/syn�������յ�response����
          ���ֵĵ�һ��payload�ǿյ�request�� */
u_int32_t stream_pair(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char *data, struct buffer_files *files) {
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

    /* ������д���Ӧ�Ļ����ļ� */
    if (!(p_info_hd->isin)) {/* request��(�����������server����) */
        if (p_info_hd->rst || p_info_hd->fin || p_info_hd->syn) {
            push_into_db(files, p_info_hd, p_info); /* �����ѻ������� */
            return 0;
        }

        /* û��payload��ack�� */
        if (0 == p_info_hd->payload_len) {
            return 0;
        }

        /* response������ֵĵ�һ��payload�ǿյ�request����
           ������һ��request/response */
        if (get_file_size(files->request_fname) > 0) {
            push_into_db(files, p_info_hd, p_info);
        }

        /* ����request�� */
        remove_single_oldfile(files->request_fname, 120);
        if (get_file_size(files->request_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
            write_file_tds(files->request_fname, (char *)data, p_info_hd->payload_len, 1);
        }
    } else { /* response�� */
        if (p_info_hd->rst || p_info_hd->fin || p_info_hd->syn) {
            if (p_info_hd->payload_len > 0) {
                remove_single_oldfile(files->response_fname, 120);
                if (get_file_size(files->response_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
                    write_file_tds(files->response_fname, (char *)data, p_info_hd->payload_len, 0);
                }
            }

            push_into_db(files, p_info_hd, p_info); /* �����ѻ������� */
            return 0;
        }

        /* û��payload��ack�� */
        if (0 == p_info_hd->payload_len) {
            return 0;
        }

        /* ����response�� */
        remove_single_oldfile(files->response_fname, 120);
        if (get_file_size(files->response_fname) < REQUEST_RESPONSE_BUFFER_LIMIT) {
            write_file_tds(files->response_fname, (char *)data, p_info_hd->payload_len, 0);
        }
    }
    return 0;
}

/* ����1433�˿ڵ������� */
u_int32_t audit_sqlserver(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char *data) {
    struct buffer_files cur_bf;

    /* request/response�����ļ��� */
    sprintf(cur_bf.request_fname, SQLSERVER_REQUEST_RESPONSE_TMP "request_%u_%d", p_info_hd->hash, p_info_hd->thrid);
    sprintf(cur_bf.response_fname, SQLSERVER_REQUEST_RESPONSE_TMP "response_%u_%d", p_info_hd->hash, p_info_hd->thrid);

    return stream_pair(p_info_hd, p_info, data, &cur_bf);
}


