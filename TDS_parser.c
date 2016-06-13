/*
2015-12-15:
1. 把getFirstSqlAction()里面insert等关键词的strlen改为常量
2. 把TDS_parser()里面switch处sql语句及其回复的类型调到最前面
3. 把;:r去掉

2015-12-16:
1. 在ServerMgr_parse()中，把所有的类型都加上了
2. 把ServerMgr_parse()中，大结构的变量都移到其类型定义处，改为全局变量，防止栈空间不够

2015-12-21:
1. 新增findNextSemicolonOrLinebreak(), 由findNextSemicolon()改来。
   把getHighestLevelSqlAction()里的findNextSemicolon()函数，改为调用findNextSemicolonOrLinebreak()。
   多个sql语句不但可以用分号，还可以用换行分开
2. TDS_parser()开头新增debug打印语句，打印tds类型
3. 增加了char(n), varchar(n),text的识别，即在函数getDescriptionInRow()中，增加了下面集中情况的识别
    case CHARTYPE:
    case VARCHARTYPE:
    case BIGVARCHRTYPE:
    case BIGCHARTYPE:
    case TEXTTYPE:
4. 增加了binary(n), varbinary(n),image的识别，即在函数getDescriptionInRow()中，增加了下面集中情况的识别
	case VARBINARYTYPE:
	case BIGBINARYTYPE:
	case BIGVARBINTYPE:
	和case IMAGE:

2015-12-22:
1. getDescriptionInRow(), getDescriptionInNBCRow()里面的分类型处理数据部分代码是相同的，把这部分
   代码拿出来作为一个函数parseValues()，上面两个函数调用它即可
2. getDescriptionInRow(), getDescriptionInNBCRow()的循环开头增加了PLP_BODY类型数据的合并处理代码
3. getDescriptionInRow(), getDescriptionInNBCRow()里的PLP_BODY类型数据的合并处理的第1部分case最后不上break，修改该bug
4. binaryToHexVisible()函数，原先只处理前20个字节，其他省略，后加...，改为全处理
   且修改了它1个bug，数据传入由char * 改为unsigned char *，否则大于127时会出错
   去掉binaryToHexVisible()里的memset，并保证它最后加\0.
5. 把parseValues()里的整数部分，由调用函数，改为直接用指针变换，解决了负数出错问题
6. parseValues()里的8字节的money类型，前后2部分原先是xxx.hhh的关系，改为相加
7. parseValues()里的浮点数计算，中间量由LONG改为double，/=10之类，改为*=0.1
8. getFloat64IEEE754()和getFloat32IEEE754()函数有错误，改用getFloat64IEEE754_1()和getFloat32IEEE754_1()
   具体错误待查
9. 修改getThreeTundredthsOfASecondFrom00_00_00()里1个bug，第3个循环里60*300改为300
10. 修改了getTimeFromScaleSecsSince12AM()的一个bug，countsPerSec的类型由int改为unsigned long，否则当
    countsPerSec = 10^7时，3600*countsPerSec会溢出
11. 修正getTimeFromScaleSecsSince12AM()最后计算msec和usec的bug，公式对countsPerSec应该是除以而不是乘以
12. 把parseValues()的参数COLMETADATA *COLMETADATAres, int i,换成struct __TYPE_INFO TYPE_INFOres.
    否则解析sql_variant类型时，不能实现递归调用

2015-12-23:
1. 把getDescriptionInNBCROW()里面，循环开头判断是否是NULL的语句去掉, 因为若为NULL值，NBCROWres->AllColumnData[i].Data
   是置0的，所以长度为0，而后面长度为0的全部设为NULL了。另外该判断NULL语句少了plp_body格式的判断，有错误。
2. 把返回的other类型，统一为29
3. 去掉codeConv()里，对dst的memset清零
4. 去掉unprintable_to_space_tds()里，对dst的memset清零
5. 去掉getDescriptionInRow()和getDescriptionInNBCRow()里，对ColValBefore的memset清零，因为有长度tocopy限制，所以不需要清零
6. 把所有函数的strlen(";:n")和strlen(";:s")改为常数3
7. 修改字符集转换部分代码，NCHAR,NVARCHAR的UCS-2编码，都转为为和CHAR,VARCHAR等相同的编码，
   增加一个宏TDS_CHARSET，用于指定CHAR,VARCHAR的编码。
   删除CHAR,VARCHAR等部分的codeConv()调用，改为直接memcpy()
8. ServerMgr_parse()里, 删除对COLMETADATAres, INFOres, ENVCHANGEres, DONEres, ORDERres的memset清零
9. 改了getThreeTundredthsOfASecondFrom00_00_00()里面的前3个循环，明显能用/和%计算。还没测试

2015-12-30:
1. 函数名里单词拼写错误，parseVales()改为parseValues()
2. 把parseValues()里的unprintable_to_space_tds()调用，全部改为binaryToHexVisible()
3. 增加两个函数getDescriptionInALTMETADATA()和getDescriptionInALTROW(), 并在ServerMgr_parse()里增加这两个函数的调用
*/

/* 解析TDS协议数据
   TDS协议全称Tabular Data Stream Protocol，是一个用于和SQL Server 通讯的
   应用层协议
   注: 关于本文件标识符命名，只要是TDS文档中给出的术语，一律采用文档原文，包括大小写
   格式。虽然会有些命名格式混乱，但是容易和文档对应起来 */

#include<stdio.h>
#include<string.h>
#include<iconv.h>
#include<ctype.h>

#define TDS_DEBUG_OPEN 0
#if TDS_DEBUG_OPEN
#define TDS_DEBUG_LOG(s) fprintf(stderr, "DEBUG : <%s> : %d : %s\n", __FILE__, __LINE__, (s))
#else
#define TDS_DEBUG_LOG(s)
#endif
#define TDS_PRINT_ERR_MSG(s) fprintf(stderr, "ERROR : <%s> : %d : %s\n", __FILE__, __LINE__, (s))

#define TDS_CHARSET "GBK"

/*==================================================================================*/
/* 字节对齐 */
#pragma pack(1)

/*==================================================================================*/
/* TDS协议版本控制宏 */
/* After a protocol feature is introduced, subsequent versions of the TDS protocol
   support that feature until that feature is removed */
#define __TDS_7_0 1
#define __TDS_7_1 1
#define __TDS_7_2 1
#define __TDS_7_3 1
#define __TDS_7_4 0

#define TDS_V_7_0 70
#define TDS_V_7_1 71
#define TDS_V_7_2 72
#define TDS_V_7_3 73
#define TDS_V_7_4 74

static int TDS_VERSION = TDS_V_7_3;
static char ColValBefore[10240], ColValAfter[10240];
static unsigned char tabularResultBuf[5*1024*1024];
static char tmpbuf[1024*10];

/*==================================================================================*/
/* 字节序转换 */
#define HOST_IS_LITTLE_ENDIAN 1 /* 主机字节序 */

/* 将'data'指向的'n'个字节的数据，变换成相反的顺序 */
void mem_reverse(void *data, int n) {
    char *pos = data, tmp;
    int start=0, end=n-1;

    while (start < end) {
        tmp = pos[start];
        pos[start] = pos[end];
        pos[end] = tmp;
        start++, end--;
    }
}

/* 大端字节序转为主机字节序。当主机字节序就是大端时，该函数不做任何操作 */
void big_2_host(void *val, int size) {
#if HOST_IS_LITTLE_ENDIAN
    mem_reverse(val, size);
#endif
}

/* 小端字节序转为主机字节序。当主机字节序就是小端时，该函数不做任何操作 */
void little_2_host(char *val, int size) {
#if !(HOST_IS_LITTLE_ENDIAN)
    mem_reverse(val, size);
#endif
}

/* 把'data'指向的'len'字节的数据，都转成十六进制的可见格式字符串，保存到
   最大为'desMaxLen'字节的空间'des'中。字符串格式为:
   00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f<br>
   10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f<br>
   ......
   若'desMaxLen'字节不足，那么就把'des'填满后就返回，剩下的字节不转换。
   函数始终在结果串末尾添加'\0'. */
int binary_to_hex_visible (char *data, int len, char *des, int desMaxLen) {
    char *datapos, *despos, tmp;
    int num_in_line;
    const int BR_LEN = 4, CHARS_PER_LINE = 16, PAD_SIZE = 1;

    datapos = data;
    despos = des;
    num_in_line = 0;
    while ((datapos < (data+len)) && (desMaxLen-(despos-des) >= 2+PAD_SIZE)) {
        tmp = *datapos/16;
        *despos++ = (tmp >=0 && tmp<=9) ? (tmp+48) : (tmp+55);
        tmp = *datapos%16;
        *despos++ = (tmp >=0 && tmp<=9) ? (tmp+48) : (tmp+55);
        datapos++;

        num_in_line++;
        if (CHARS_PER_LINE == num_in_line) {
            if (desMaxLen-(despos-des) < BR_LEN+PAD_SIZE ) {
                fprintf(stderr, "Space is too small\n");
                break;
            }
            *despos++ = '<';
            *despos++ = 'b';
            *despos++ = 'r';
            *despos++ = '>';
            num_in_line = 0;
        } else {
            if (desMaxLen-(despos-des) < 1+PAD_SIZE ) {
                fprintf(stderr, "Space is too small\n");
                break;
            }
            *despos++ = ' ';
        }
    }
    *despos++ = '\0';
    return (despos - des);
}

/* 但是若'data'过多，则最多处理20字节，并后跟"..."
   例如:
   "12345"转为"0x3132333435"
   "012345678901234567890123456789" = "0x3031323334353637383930313233343536373839..." */
#define BINARY_TO_HEX_VISIBLE_MAX 20
int binaryToHexVisible (unsigned char *data, int len, char *des, int desMaxLen) {
    unsigned char *datapos, tmp;
    char *despos;
    int tohandle;

    datapos = data;
    despos = des;
    *despos++ = '0';
    *despos++ = 'x';
    *despos = '\0';

    tohandle = len;

    /* "despos < (desMaxLen+des-4)"是为\0预留空间 */
    while ((datapos < (data+tohandle)) && despos < (desMaxLen+des-1)) {
        tmp = *datapos/16;
        *despos++ = (tmp >=0 && tmp<=9) ? (tmp+48) : (tmp+55);
        tmp = *datapos%16;
        *despos++ = (tmp >=0 && tmp<=9) ? (tmp+48) : (tmp+55);
        datapos++;
    }
    *despos++ = '\0';
    return (despos - des);
}

#if 0
/* 将长'slen'的串's'中, 1个或连续的多个不可打印字符变成一个空格, 结果串保存到'dst'中
 * 返回'dst'字符串的长度 */
static int unprintable_to_space_tds(char *src, int slen, char *dst, int dst_maxsize) {
    int i=0, j=0;

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
#endif


/* 字符集转换。成功则返回结果串长度，失败返回-1
   注意 - GBK/GB2312等，属于一种，所以他们之间转换会在iconv()上出错 */
static int codeConv(char* srcCharSet, char *dstCharSet, char *src, size_t srcLen, char *dst, size_t dstMaxSize) {
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

    if(iconv(cd, &src, &srcLen, &tmppos, &tmpLen) < 0){
        iconv_close(cd);
        perror("iconv() failed\n");
        return -1;
    }
    iconv_close(cd);

    dst[dstMaxSize - tmpLen] = '\0';
    return (dstMaxSize - tmpLen);
}

/*
<1>非整百年能被4整除的为闰年。（如2004年就是闰年,2010年不是闰年）
<2>整百年能被400整除的是闰年。(如2000年是闰年，1900年不是闰年)
<3>对于数值很大的年份,这年如果能整除3200,并且能整除172800则是闰年。如172800年是闰年，86400年不是闰年(因为虽然能整除3200，但不能整除172800)*/
int isLeapYear(long year) {
    if (year % 100 != 0 && year % 4 == 0) {
        return 1;
    }
    if (year % 100 == 0 && year % 400 == 0) {
        return 1;
    }
    return 0;
}

int getDaysInYear(long year) {
    if (isLeapYear(year)) return 366;
    return 365;
}

int getDaysInMonth(int year, int month) {
    switch (month) {
        case 1: case 3:case 5: case 7:case 8: case 10:case 12:
            return 31;
        case 2:
            if (isLeapYear(year)) return 29;
            else return 28;
        default:
            return 30;
    }
}


/* 'days'是要计算的日期，距离公元Y年1月1日的天数，包括该天，但不包括要计算的那天。 */
int getDateFromDaysSinceYYYY0101 (int days, int YYYY, int *year, int *month, int *day) {
    int tmpYear, tmpMonth, tmpDay, dayInCurYear, dayInCurMonth;
    long leftDays;

    tmpYear = YYYY;
    leftDays = days;
    while ((dayInCurYear = getDaysInYear(tmpYear)) <= leftDays) {
        leftDays -= dayInCurYear;
        tmpYear ++;
    }
    *year = tmpYear;

    if (0 == leftDays) {
        *month = 1;
        *day = 1;
        return 0;
    }

    tmpMonth = 1;
    while ((dayInCurMonth = getDaysInMonth(tmpYear, tmpMonth)) <= leftDays) {
        leftDays -= dayInCurMonth;
        tmpMonth ++;
    }
    *month = tmpMonth;

    tmpDay = 1;
    while (leftDays-- > 0) tmpDay++;
    *day = tmpDay;

    return 0;
}


/* 'days'是要计算的日期，距离公元1年1月1日的天数，包括该天，但不包括要计算的那天。 */
int getDateFromDaysSince00010101 (int days, int *year, int *month, int *day) {
    return getDateFromDaysSinceYYYY0101(days, 1, year, month, day);
}

/* 'days'是要计算的日期距离公元1900年1月1日的天数，包括该天，但不包括要计算的那天。 */
int getDateFromDaysSince19000101 (int days, int *year, int *month, int *day) {
    return getDateFromDaysSinceYYYY0101(days, 1900, year, month, day);
}

/* 'days'是要计算的日期距离公元1753年1月1日的天数，包括该天，但不包括要计算的那天。 */
int getDateFromDaysSince17530101 (int days, int *year, int *month, int *day) {
    return getDateFromDaysSinceYYYY0101(days, 1753, year, month, day);
}


/* 'mins'是要计算的时间距离00:00:00的时间，单位是分钟min */
int getMinsFrom00_00_00(int mins, int *hour, int *min, int *sec) {
    int curHour, leftMins;
    curHour = 0;
    leftMins = mins;
    while (leftMins >= 60) {
        curHour ++;
        leftMins -= 60;
    }
    *hour = curHour;
    *min = leftMins;
    *sec = 0;
    return 0;
}

/* 'threeTundredthsOfASeconds'是要计算的时间距离00:00:00的时间，单位是1/300秒 */
int getThreeTundredthsOfASecondFrom00_00_00(int threeTundredthsOfASeconds, int *hour, int *min, int *sec, int *msec) {
    int curHour, curMin, curSec;
    long left;

    left = threeTundredthsOfASeconds;
#if 0
    curHour = 0;
    while (left >= 3600*300) {
        curHour ++;
        left -= 3600*300;
    }

    curMin = 0;
    while (left >= 60*300) {
        curMin ++;
        left -= 60*300;
    }

    curSec = 0;
    while (left >= 300) {
        curSec ++;
        left -= 300;
    }
#endif
    curHour = left/(3600*300);
    left %= (3600*300);

    curMin = left/(60*300);
    left %= (60*300);

    curSec = left/300;
    left %= 300;

    *hour = curHour;
    *min = curMin;
    *sec = curSec;
    *msec = left*3/10;
    return 0;
}


/* 'ssecs'是要计算的时间距离00:00:00有多少个10^(-'scale')秒。 */
int getTimeFromScaleSecsSince12AM1(unsigned long ssecs, int scale, int *hour, int *min, int *sec, int *msec, int *usec) {
    unsigned long secs;
    int curHour, curMin, base;
    int i;

    /* 微秒 */
    base = 1;
    for (i=7; i<=scale; i++) {
        base *= 10;
    }
    if (scale >= 6) {
        *usec = (ssecs/base)%1000;
    } else {
        *usec = 0;
    }

    /* 毫秒 */
    base = 1;
    for (i=4; i<=scale; i++) {
        base *= 10;
    }
    if (scale >= 3) {
        *msec = (ssecs/base)%1000;
    } else {
        *msec = 0;
    }

    /* 小时 */
    secs = ssecs;
    for (i=0; i<scale; i++) {
        secs /= 10;
    }
    curHour = 0;
    while (secs >= 3600) {
        curHour ++;
        secs -= 3600;
    }
    *hour = curHour;

    /* 分钟/秒 */
    curMin = 0;
    while (secs >= 60) {
        curMin ++;
        secs -= 60;
    }
    *min = curMin;
    *sec = secs;
    return 0;
}

/* 'ssecs'是要计算的时间距离00:00:00有多少个10^(-'scale')秒。 */
int getTimeFromScaleSecsSince12AM(unsigned long ssecs, int scale, int *hour, int *min, int *sec, int *msec, int *usec) {
    unsigned long left;
    int curHour, curMin, curSec;
    int i;
    unsigned long countsPerSec = 1; /* 必须是unsigned long，否则在3600*countsPerSec时会溢出 */

    for (i=0; i<scale; i++) {
        countsPerSec *= 10;
    }

    left = ssecs;

    curHour = 0;
    while (left > 3600*countsPerSec) {
        curHour++;
        left -= (3600*countsPerSec);
    }

    curMin = 0;
    while (left > 60*countsPerSec) {
        curMin++;
        left -= (60*countsPerSec);
    }

    curSec = 0;
    while (left > countsPerSec) {
        curSec++;
        left -= countsPerSec;
    }

    *hour = curHour;
    *min = curMin;
    *sec = curSec;
    *msec = left / countsPerSec / 1000;
    *usec = left / countsPerSec / 1000000 ;
    return 0;
}


/*==================================================================================*/
#define CLR_ZERO_CHARS(pos, len)                    \
do {                                                \
    int i, j;                                       \
    for (i=0,j=0; i<len;) {                         \
        if ('\0'!=pos[i]) pos[j++]=pos[i++];        \
        else i++;                                   \
    }                                               \
    pos[j] = '\0';                                  \
}while(0)

#define STEP(pos, leftlen, type_size, type, obj)    \
do {                                                \
    if (leftlen >= (type_size)) {                   \
        (obj) = *(type *)(pos);                     \
        (pos) += (type_size);                       \
    } else {TDS_PRINT_ERR_MSG("short of length"); return -1;}      \
}while(0)

/* 'pos'再前进'nbytes'字节 */
#define STEP_N(pos, len, nbytes)                    \
do {                                                \
    if ((len) >= (nbytes)) {                        \
        (pos) += nbytes;                            \
    } else {                                        \
        TDS_PRINT_ERR_MSG("short of length"); return -1;    \
    }                                               \
}while(0)

/* 让'pointer'指向'pos'，'pos'再前进'nbytes'字节 */
#define STEP_PL(pos, len, pointer, nbytes)          \
do {                                                \
    (pointer) = (unsigned char *)(pos);              \
    if ((len) >= (nbytes)) {                        \
        (pos) += nbytes;                            \
    } else {                                        \
        TDS_PRINT_ERR_MSG("short of length"); return -1;    \
    }                                               \
}while(0)

#define COPY_STR(dst, size, src, slen)              \
do {                                                \
    if ((size)>(slen)) {                            \
        memcpy(dst,src,slen);                       \
    } else {TDS_PRINT_ERR_MSG("lack of space"); return -1;} \
}while(0)

/*==================================================================================*/
/* 2.2.3节，TDS协议头 */
/* TDS协议头的'type'字段的取值。
   <1>其中除'TYPE_Attention_signal'外，其他都有数据
   <2>server发送的所有包，都是'TYPE_Tabular_result'类型*/
#define TYPE_SQL_batch 0x01
#define TYPE_Pre_TDS7_Login 0x02 /* Only legacy clients that support SQL Server versions that were released prior to sql_server 7.0 can use Pre-TDS7 Login. */
#define TYPE_RPC  0x03
#define TYPE_Tabular_result  0x04
#define TYPE_Attention_signal 0x06
#define TYPE_Bulk_load_data 0x07 /* This message sent to the server contains bulk data to be inserted. */
#define TYPE_Federated_Authentication_Token 0x08 /* Federated authentication is not supported by SQL Server. */
#define TYPE_Transaction_manager_request 0x0e
#define TYPE_TDS7_Login 0x10 /* Only clients that support sql_server 7.0 or later can use TDS7 Login. */
#define TYPE_SSPI 0x11
#define TYPE_Pre_Login 0x12

/* 数据库里的类型标识 */
#define Pre_Login_flag 12
#define Pre_TDS7_Login_flag 13
#define TDS7_Login_flag 13
#define Federated_Authentication_Token_flag 14
#define Bulk_load_data_flag 15
#define RPC_flag 16
#define Attention_signal_flag 17
#define Transaction_manager_request_flag 18
#define SSPI_flag 17
#define select_flag 6
#define update_flag 7
#define delete_flag 8
#define insert_flag 9
#define create_flag 10
#define drop_flag 11
#define other_sql_flag 19
#define other_flag 20
#define alter_flag 21
#define use_flag 22
#define set_flag 23
#define grant_flag 24
#define deny_flag 25
#define revoke_flag 26
#define commit_flag 27
#define rollback_flag 28
#define other_sql_state_flag 29

#define response_flag 30
#define error_flag (-1)

/* TDS协议头的'status'字段是位域，下面是它各个位的定义。所有其他位应该忽略 */
#define STATUS_Normal_message 0x00   /* "Normal" message */
#define STATUS_EOM 0x01  /* End of message (EOM). The packet is the last packet in the whole request. */
#define STATUS_Ignore_this_event 0x02 /* (From client to server) Ignore this event (0x01 MUST also be set) */
#define STATUS_RESETCONNECTION 0x08 /* (From client to server) Reset this connection before processing event */
#define STATUS_RESETCONNECTIONSKIPTRAN 0x10 /* (From client to server) Reset the connection before processing event but do not modify the transactionstate */

/* TDS协议头，共8字节 */
typedef struct {
    unsigned char Type; /* type of message */
    unsigned char Status; /* a bit field used to indicate the message state. */
    unsigned short Length;/* It is the number o f bytes from the start of this header to the start of
                             the next packet header. Length is a 2-byte, unsigned short int and is
                             represented in network byte order (big-endian). */
    unsigned short SPID;/* the process ID on the server, corresponding to the current connection
                           It is provided for debugging purposes.
                           This is a 2-byte value and is represented in network byte order (big-endian) */
    unsigned char PacketID;/* used for numbering message packets that contain data in addition to the packet header.
                              This value is currently ignored. */
    unsigned char Window; /* This 1 byte is currently not used */
} TDS_HEADERS;

/*==================================================================================*/
/* 来自2.2.4节，判别tokenType值(如ALTMETADATA_TOKEN)属于哪个类别 */
#define is_zero_length_token(token) ((token)&0x30 == 0x10) /* 没后续数据 */
#define is_fixed_length_token_1(token) ((token)&0x3c == 0x30) /* 后跟1字节数据 */
#define is_fixed_length_token_2(token) ((token)&0x3c == 0x34) /* 后跟2字节数据 */
#define is_fixed_length_token_4(token) ((token)&0x3c == 0x38) /* 后跟4字节数据 */
#define is_fixed_length_token_8(token) ((token)&0x3c == 0x3c) /* 后跟8字节数据 */
#define is_variale_length_token(token) ((token)&0x30 == 0x20) /* 后跟1个长度值+该长度的数据 */
#define is_variale_count_token(token) ((token)&0x30 == 0x00) /* 后跟1个域的个数n+n个域 */

/*==================================================================================*/
/* 2.2.5.1节 */
/* All integer types are represented in reverse byte order (little-endian) unless
   otherwise specified
   注意 - 在公司机器上，long/unsigned long/long long /unsigned long long都是8位 */
typedef unsigned char BYTE;                /* BYTE = 8BIT */
typedef unsigned char BYTELEN;             /* BYTELEN = BYTE */
typedef unsigned short USHORT;             /* USHORT = 2BYTE */
typedef int LONG;                         /* LONG = 4BYTE */
typedef unsigned int ULONG;                /* ULONG = 4BYTE */
typedef unsigned int DWORD;                /* DWORD = 32BIT */
typedef long long LONGLONG;                /* LONGLONG = 8BYTE */
typedef unsigned long ULONGLONG;           /* ULONGLONG = 8BYTE */
typedef unsigned char UCHAR;               /* UCHAR = BYTE */
typedef unsigned short USHORTLEN;          /* USHORTLEN = 2BYTE */
typedef unsigned short USHORTCHARBINLEN;   /* USHORTCHARBINLEN = 2BYTE */
typedef int LONGLEN;                       /* LONGLEN = 4BYTE */
typedef unsigned int ULONGLEN;             /* ULONGLEN = 4BYTE */
typedef unsigned long ULONGLONGLEN;        /* ULONGLONGLEN = 8BYTE */
typedef unsigned char PRECISION;           /* PRECISION = 8BIT */
typedef unsigned char SCALE;               /* SCALE = 8BIT */
#define GEN_NULL 0x00 /* A single byte (8-bit) value representing a NULL value */
#define CHARBIN_NULL2 0xffff /* a T-SQL NULL value for a character or binary data type. */
#define CHARBIN_NULL4 0xffffffff /* a T-SQL NULL value for a character or binary data type. */
#define FRESERVEDBYTE 0x00; /* a BYTE value used for padding that does not transmit information. */
typedef unsigned short UNICODECHAR;

/*==================================================================================*/
/* 2.2.5.1.2节，COLLATION规则
   The collation rule is used to specify collation information for character data or metadata describing character data<16>.
   This is typically specified as part of the LOGIN7 message or part of a column definition in server results containing character data.
   A SQL (SortId==1) collation is one of a predefined set of sort orders. It is identified by having SortId
with values as described by [MSDN-SQLCollation].
   For a SortId==0 collation, the LCID bits correspond to a LocaleId as defined by the National Language
Support (NLS) functions. For more details, see [MS-LCID].*/
typedef struct {
    ULONG Flags;
    UCHAR SortId;
} COLLATION;

/* 长'len'字节的'data'指向的数据开头是'COLLATION'结构，解出来，结果放
   'res'保存，该结构长度最后赋给'len'来返回。
   成功返回0，失败返回-1。 */
int COLLATION_parse (unsigned char *data, int *len, COLLATION *res) {
    int leftlen = *len;
    unsigned char *pos = data;

    if (leftlen <= 0) {TDS_PRINT_ERR_MSG("leftlen <= 0"); return -1;}
    STEP(pos, *len-(pos-data), 4, ULONG, res->Flags);
    STEP(pos, *len-(pos-data), 1, UCHAR, res->SortId);
    *len = pos-data;
    return 0;
}

/*==================================================================================*/
/* 2.2.5.2.2小节 */
typedef struct {
    BYTELEN wlen; /* 单位2字节 */
    UCHAR *data; /* 文档中是CHAR类型，但是大部分字节类型都是UCHAR,且CHAR在文档中没定义 */
}B_VARCHAR;

typedef struct {
    USHORTLEN wlen; /* 单位2字节 */
    UCHAR *data;
}US_VARCHAR;

typedef struct {
    BYTELEN blen; /* 单位1字节 */
    BYTE *data;
}B_VARBYTE;

typedef struct {
    USHORTLEN blen; /* 单位1字节 */
    BYTE *data;
}US_VARBYTE;

typedef struct {
    LONGLEN blen; /* 单位1字节 */
    BYTE *data;
}L_VARBYTE;

int B_VARCHAR_parse (unsigned char *data, int *len, B_VARCHAR *res) {
    unsigned char *pos = data;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}

    STEP(pos, *len-(pos-data), 1, UCHAR, res->wlen);
    STEP_PL(pos, *len-(pos-data), res->data, res->wlen *2);

    *len = pos-data;
    return 0;
}

int US_VARCHAR_parse (unsigned char *data, int *len, US_VARCHAR *res) {
    unsigned char *pos = data;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}

    STEP(pos, *len-(pos-data), 2, USHORTLEN, res->wlen);
    STEP_PL(pos, *len-(pos-data), res->data, res->wlen *2);

    *len = pos-data;
    return 0;
}

int B_VARBYTE_parse (unsigned char *data, int *len, B_VARBYTE *res) {
    unsigned char *pos = data;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}
    STEP(pos, *len-(pos-data), 1, BYTELEN, res->blen);
    STEP_PL(pos, *len-(pos-data), res->data, res->blen);

    *len = pos-data;
    return 0;
}

int US_VARBYTE_parse (unsigned char *data, int *len, US_VARBYTE *res) {
    unsigned char *pos = data;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}
    STEP(pos, *len-(pos-data), 2, USHORTLEN, res->blen);
    STEP_PL(pos, *len-(pos-data), res->data, res->blen);

    *len = pos-data;
    return 0;
}

int L_VARBYTE_parse (unsigned char *data, int *len, L_VARBYTE *res) {
    unsigned char *pos = data;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}
    STEP(pos, *len-(pos-data), 4, LONGLEN, res->blen);
    STEP_PL(pos, *len-(pos-data), res->data, res->blen);

    *len = pos-data;
    return 0;
}

/*==================================================================================*/
/* 2.2.5.2.3小节: TYPE_VARLEN / PLP_BODY / TYPE_VARBYTE */

/* Data type-dependent integers can be either a BYTELEN, USHORTCHARBINLEN, or LONGLEN in length.
   This length is dependent on the TYPE_INFO associated with the message.
   <1>If the data type (for example, FIXEDLENTYPE or VARLENTYPE rule of the TYPE_INFO rule) is of type SSVARIANTTYPE,
TEXTTYPE, NTEXTTYPE, or IMAGETYPE, the integer length is LONGLEN.
   <2>If the data type is BIGCHARTYPE, BIGVARCHARTYPE, NCHARTYPE, NVARCHARTYPE, BIGBINARYTYPE, or
BIGVARBINARYTYPE, the integer length is USHORTCHARBINLEN.
   <3>For all other data types, the integer length is BYTELEN.

   LONGLEN - SSVARIANTTYPE, TEXTTYPE, NTEXTTYPE, IMAGETYPE
   USHORTCHARBINLEN - BIGCHARTYPE, BIGVARCHARTYPE, NCHARTYPE, NVARCHARTYPE, BIGBINARYTYPE, BIGVARBINARYTYPE
   BYTELEN - 其他 */
typedef struct {
    ULONGLONGLEN reallen;
    BYTELEN len1;
    USHORTCHARBINLEN len2;
    LONGLEN len4;
}TYPE_VARLEN;

#define PLP_NULL 0xFFFFFFFFFFFFFFFF        /* 8字节 */
#define UNKNOWN_PLP_LEN 0xFFFFFFFFFFFFFFFE /* 8字节，跟前者，就最后1位不同 */
#define PLP_TERMINATOR 0x00000000          /* 4字节 */

typedef struct {
    ULONGLEN len;
    BYTE *data;
} PLP_CHUNK;

/* Introduced in TDS 7.2
   PLP_BODY可以是3种格式
   当'len'域等于PLP_NULL时，'struct __PLP_BODY'就只有这1个域的大小
   当'len'域等于UNKNOWN_PLP_LEN时，'len'后跟多个PLP_CHUNK，且以PLP_TERMINATOR结尾
   当'len'域是其他值时，'len'后跟多个PLP_CHUNK，且这些PLP_CHUNK的总长是'len'字节
   Unlike fixed or variable byte stream formats, Partially length-prefixed bytes (PARTLENTYPE), introduced in TDS 7.2, do not require the full data length to be specified before the actual data is streamed out.
   Thus, it is ideal for those applications where the data length is not known upfront (that is, xml serialization).
   A value sent as PLP can be either
   <1>NULL (PLP_NULL),
   <2>a length followed by chunks (as defined by PLP_CHUNK),
   <3>or an unknown length token(UNKNOWN_PLP_LEN) followed by chunks, which MUST end with a PLP_TERMINATOR.

   <1>TYPE_INFO rule specifies a Partially Length-prefixed Data type (PARTLENTYPE, see 2.2.5.4.3).
   <2>In the UNKNOWN_PLP_LEN case, the data is represented as a series of zero or more chunks, each
consisting of the length field followed by length bytes of data (see the PLP_CHUNK rule). The data
is terminated by PLP_TERMINATOR (which is essentially a zero-length chunk).
   <3>In the actual data length case, the ULONGLONGLEN specifies the length of the data and is followed
by any number of PLP_CHUNKs containing the data. The length of the data specified by
ULONGLONGLEN is used as a hint for the receiver. The receiver SHOULD validate that the length
value specified by ULONGLONGLEN matches the actual data length. */
typedef struct {
    ULONGLONGLEN len;
    PLP_CHUNK plp_chunk[256];
} PLP_BODY;

int PLP_BODY_parse(unsigned char *data, int *len, PLP_BODY *res) {
    unsigned char *pos = data;
    int i;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}

    STEP(pos, *len, 8, ULONGLONGLEN, res->len);
    if (0xFFFFFFFFFFFFFFFF == res->len) {
        *len = pos -data;
        res->plp_chunk[0].len = 0;
        return 0;
    } else {
        for (i=0; i<256; i++) {
            STEP(pos, *len-(pos-data), 4, ULONGLEN, res->plp_chunk[i].len);
            if (res->plp_chunk[i].len > 0) {
                STEP_PL(pos, *len-(pos-data), res->plp_chunk[i].data, res->plp_chunk[i].len);
            } else {
                res->plp_chunk[i].data = NULL;
                break;
            }
        }
        *len = pos - data;
        return 0;
    }
    return 0;
}

/*==================================================================================*/
/* 2.2.5.3小节，ALL_HEADERS规则 */
/* 'ALL_HEADERS'里'HeaderType'的可能取值 */
#define HeaderType_Query_Notifications 0x0001 /* introduced in TDS 7.2 */
#define HeaderType_Transaction_Descriptor 0x0002 /* introduced in TDS 7.2 */
#define HeaderType_Trace_Activity 0x0003 /* introduced in TDS 7.4 */

typedef struct {
    DWORD HeaderLength; /* Total length of an individual header, including itself */
    USHORT HeaderType;  /* The type of header, */
    BYTE *HeaderData;   /* The data stream for the header */
} ALL_HEADERS_Header;

typedef struct {
    DWORD TotalLength;      /* Total length of ALL_HEADERS stream, including itself */
    ALL_HEADERS_Header Header; /* 应该是多个Header，但此处为了当作处理缓存用 */
} ALL_HEADERS;

/* ALL_HEADERS中3种类型的HeaderData的结构 */

/* This packet data stream header allows the client to specify that a notification is to be supplied on the
results of the request. The contents of the header specify the information necessary for delivery of the
notification.  */
typedef struct {
    USHORT NotifyId_len; /* 单位1字节 */
    UCHAR *NotifyId; /* user specified value when subscribing to the query notification */
    USHORT SSBDeployment_len; /* 单位1字节 */
    UCHAR *SSBDeployment;
    ULONG NotifyTimeout; /* 可选地，duration in which the query notification subscription is valid */
} Query_Notifications_Header;

typedef struct {
    ULONGLONG TransactionDescriptor; /* number of requests currently active on the connection */
    /* For each connection, a number that uniquely identifies the transaction the
       request is associated with.Initially generated by the server when a new
       transaction is created and returned to the client as part of the ENVCHANGE
       token stream. */
    DWORD OutstandingRequestCount;
} Transaction_Descriptor_Header;

typedef struct {
    BYTE ActivityId[20];/* client Activity ID for debugging purposes */
} Trace_Activity_Header;

int ALL_HEADERS_parse (unsigned char *data, int *len, ALL_HEADERS *res) {
    unsigned char *pos = data;

    STEP(pos, *len, 4, DWORD, res->TotalLength);
    if (*len >= res->TotalLength)
        pos = data + res->TotalLength; /* 注意该值是整个结构的字节数 */
    else {
        if ('\0'==pos[1] && '\0'==pos[3]) {
            *len = 0;
            return 0;
        }
        TDS_PRINT_ERR_MSG("lack of data");
        return -1;
    }
    *len = pos - data;
    return 0;
}

/*==================================================================================*/
/* 2.2.5.4.1小节
   FIXEDLENTYPE取值范围 (TYPE_INFO = FIXEDLENTYPE)
   <1>Non-nullable values are returned using these fixed-length data types.
   <2>There is no data associated with NULLTYPE.
   <3>For the rest of the fixed-length data types, the length of data is predefined by the type.
   <4>There is no TYPE_VARLEN field in the TYPE_INFO(2.2.5.6) rule for these types.
   <5>In the TYPE_VARBYTE(2.2.5.2.3) rule for these types, the TYPE_VARLEN field is BYTELEN, and the value is
   1 for INT1TYPE/BITTYPE,
   2 for INT2TYPE,
   4 for INT4TYPE/DATETIM4TYPE/FLT4TYPE/MONEY4TYPE
   8 for MONEYTYPE/DATETIMETYPE/FLT8TYPE/INT8TYPE
   <6>The value represents the number of bytes of data to be followed.
   <7>The SQL data types of the corresponding fixed-length data types are in the comment part
of each data type. */
#define NULLTYPE 0x1F     // Null,无后续数据
#define INT1TYPE 0x30     // TinyInt，1字节
#define BITTYPE 0x32      // Bit，1字节
#define INT2TYPE 0x34     // SmallInt，2字节
#define INT4TYPE 0x38     // Int，4字节
#define DATETIM4TYPE 0x3A // SmallDateTime，4字节
#define FLT4TYPE 0x3B     // Real，4字节
#define MONEYTYPE 0x3C    // Money，8字节
#define DATETIMETYPE 0x3D // DateTime，8字节
#define FLT8TYPE 0x3E     // Float，8字节
#define MONEY4TYPE 0x7A   // SmallMoney，4字节
#define INT8TYPE 0x7F     // BigInt，8字节

/*==================================================================================*/
/* 2.2.5.4.2小节
   VARLENTYPE = BYTELEN_TYPE/USHORTLEN_TYPE/LONGLEN_TYPE
   <1>Nullable values are returned by using the INTNTYPE, BITNTYPE, FLTNTYPE, GUIDTYPE, MONEYNTYPE,
and DATETIMNTYPE tokens which will use the length byte to specify the length of the value or
GEN_NULL as appropriate.
   <2>There are two types of variable-length data types. These are real variable-length data types, like char
and binary, and nullable data types, which have either a normal fixed length that corresponds to their
type or to a special length if null.
   <3>Char and binary data types have values that either are null or are 0 to 65534 (0x0000 to 0xFFFE)
bytes of data. Null is represented by a length of 65535 (0xFFFF). A non-nullable char or binary can
still have a length of zero (for example, an empty value). A program that MUST pad a value to a fixed
length typically adds blanks to the end of a char and adds binary zeros to the end of a binary.
   <4>Text and image data types have values that either are null or are 0 to 2 gigabytes (0x00000000 to
0x7FFFFFFF bytes) of data. Null is represented by a length of -1 (0xFFFFFFFF). No other length
specification is supported.
   <5>Other nullable data types have a length of 0 when they are null. */

/* BYTELEN_TYPE取值范围
   <1>The length value associated with these data types is specified within a BYTE
   <2>Exceptions are thrown when invalid lengths are presented to the server during BulkLoadBCP and RPC
requests.*/
#define GUIDTYPE 0x24            //UniqueIdentifier. For GUIDTYPE, the only valid lengths are 0x10 for non-null instances and 0x00 for NULL instances.
#define INTNTYPE 0x26            //(see below) For INTNTYPE, the only valid lengths are 0x01, 0x02, 0x04, and 0x08, which map to tinyint, smallint, int, and bigint SQL data types respectively.
#define DECIMALTYPE 0x37         //Decimal (legacy support)
#define NUMERICTYPE 0x3F         //Numeric (legacy support)
#define BITNTYPE 0x68            //(see below) For BITNTYPE, the only valid lengths are 0x01 for non-null instances and 0x00 for NULL instances.
#define DECIMALNTYPE 0x6A        //Decimal
#define NUMERICNTYPE 0x6C        //Numeric
#define FLTNTYPE 0x6D            //(see below) For FLTNTYPE, the only valid lengths are 0x04 and 0x08, which map to 7-digit precision float and 15-digit precision float SQL data types respectively.
#define MONEYNTYPE 0x6E          //(see below) For MONEYNTYPE, the only valid lengths are 0x04 and 0x08, which map to smallmoney and money SQL data types respectively.
#define DATETIMNTYPE 0x6F        //(see below) For DATETIMNTYPE, the only valid lengths are 0x04 and 0x08, which map to smalldatetime and datetime SQL data types respectively.
#define DATENTYPE 0x28           //(introduced in TDS 7.3)
                                 // For DATENTYPE, the only valid lengths are 0x03 for non-NULL instances and 0x00 for NULL instances.
#define TIMENTYPE 0x29           //(introduced in TDS 7.3)
                                 // For TIMENTYPE, the only valid lengths (along with the associated scale value) are:
                                 // SCALE  1   2   3   4   5   6   7
                                 // LENGTH 0x3 0x3 0x4 0x4 0x5 0x5 0x5
#define DATETIME2NTYPE 0x2A      //(introduced in TDS 7.3)
                                 // For DATETIME2NTYPE, the only valid lengths (along with the associated scale value) are:
                                 // SCALE  1   2   3   4   5   6   7
                                 // LENGTH 0x6 0x6 0x7 0x7 0x8 0x8 0x8
#define DATETIMEOFFSETNTYPE 0x2B //(introduced in TDS 7.3)
                                 // For DATETIMEOFFSETNTYPE, the only valid lengths (along with the associated scale value) are:
                                  // SCALE  1   2   3   4   5   6   7
                                  // LENGTH 0x8 0x8 0x9 0x9 0xA 0xA 0xA
#define CHARTYPE 0x2F            //Char (legacy support)
#define VARCHARTYPE 0x27         //VarChar (legacy support)
#define BINARYTYPE 0x2D          //Binary (legacy support)
#define VARBINARYTYPE 0x25       //VarBinary (legacy support)

/* USHORTLEN_TYPE取值范围
   the length value associated with these data types is specified within a USHORT */
#define BIGVARBINTYPE 0xA5       //VarBinary
#define BIGVARCHRTYPE 0xA7       //VarChar (TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION)
#define BIGBINARYTYPE 0xAD       //Binary
#define BIGCHARTYPE 0xAF         //Char (TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION)
#define NVARCHARTYPE 0xE7        //NVarChar (TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION)
#define NCHARTYPE 0xEF           //NChar (TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION)

/* LONGLEN_TYPE取值范围
   the length value associated with these data types is specified within a LONG*/
#define XMLTYPE 0xF1             //XML (introduced in TDS 7.2) XMLTYPE is only a valid LONGLEN_TYPE for BulkLoadBCP.
#define TEXTTYPE 0x23            //Text (TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION)
#define IMAGETYPE 0x22           //Image
#define NTEXTTYPE 0x63           //NText (TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION)
#define SSVARIANTTYPE 0x62       //Sql_Variant (introduced in TDS 7.2), MaxLength for an SSVARIANTTYPE is 8009 (8000 for strings).

/* 其他值 */
#define UDTTYPE 0xF0             //CLR UDT (introduced in TDS 7.2)

/*==================================================================================*/
/* 2.2.5.4.3小节
   PARTLENTYPE = XMLTYPE
               / BIGVARCHRTYPE
               / BIGVARBINTYPE
               / NVARCHARTYPE
               / UDTTYPE
   <1>The data value corresponding to the set of data types defined in this section follows the rule defined in
the partially length-prefixed stream definition (section 2.2.5.2.3).
   <2>BIGVARCHRTYPE, BIGVARBINTYPE, and NVARCHARTYPE can represent two types each:
      1>The regular type with a known maximum size range from 0 to 8000, defined by USHORTLEN_TYPE.
      2>A type with unlimited max size, known as varchar(max), varbinary(max) and nvarchar(max),
   which has a max size of 0xFFFF, defined by PARTLENTYPE. This class of types was introduced
   in TDS 7.2. */

/*==================================================================================*/
/* TYPE_VARBYTE可以是NULL,或字节流。
   当是NULL时，有以下几个取值
   CHARBIN_NULL2 - BIGCHARTYPE, BIGVARCHARTYPE, NCHARTYPE, NVARCHARTYPE, BIGBINARYTYPE, BIGVARBINARYTYPE
   CHARBIN_NULL4 - TEXTTYPE, NTEXTTYPE, IMAGETYPE
   GEN_NULL - 其他
   当非NULL时，有
   PLP_BODY - PARTLENTYPE类型区域的取值(2.2.5.4.3小节)
   [TYPE_VARLEN] *BYTE - 除上一行类型之外的类型

   TYPE_VARBYTE = GEN_NULL
                / CHARBIN_NULL
                / PLP_BODY
                / ([TYPE_VARLEN] *BYTE)
   <1>The 2-byte CHARBIN_NULL rule is used for BIGCHARTYPE, BIGVARCHARTYPE, NCHARTYPE, NVARCHARTYPE, BIGBINARYTYPE, and BIGVARBINARYTYPE types,
   <2>and the 4-byte CHARBIN_NULL rule is used for TEXTTYPE, NTEXTTYPE, and IMAGETYPE.
   <3>The GEN_NULL rule applies to all other types aside from PLP*/
typedef struct {
    PLP_BODY plp_body;
    TYPE_VARLEN len;
    BYTE *data;
} TYPE_VARBYTE;

/* 2.2.5.5.3小节，XML_INFO规则
   XML_INFO = SCHEMA_PRESENT [DBNAME OWNING_SCHEMA XML_SCHEMA_COLLECTION]
   当SCHEMA_PRESENT=0x01时，有后面[]中部分。=0x00则没有。 */
typedef struct {
    BYTE SCHEMA_PRESENT;/* specifies "0x01" if the type has an associated schema collection and DBNAME, OWNING_SCHEMA and XML_SCHEMA_COLLECTION MUST be included in the stream, or '0x00'otherwise. */
    B_VARCHAR DBNAME; /* the name of the database where the schema collection is defined */
    B_VARCHAR OWNING_SCHEMA; /* the name of the relational schema containing the schema collection. */
    US_VARCHAR XML_SCHEMA_COLLECTION; /* the name of the XML schema collection to which the type is bound. */
} XML_INFO;

/*==================================================================================*/
/* 2.2.5.5.2小节，UDT_INFO规则 */
typedef struct {
    USHORT MAX_BYTE_SIZE;            /* max length in bytes */
    B_VARCHAR DB_NAME1;     /* database name of the UDT */
    B_VARCHAR SCHEMA_NAME;  /* schema name of the UDT */
    B_VARCHAR TYPE_NAME;    /* type name of the UDT */
    US_VARCHAR ASSEMBLY_QUALIFIED_NAME; /* name of the CLR assembly */
} UDT_INFO_IN_COLMETADATA;

typedef struct {
    B_VARCHAR DB_NAME1;     /* database name of the UDT */
    B_VARCHAR SCHEMA_NAME;  /* schema name of the UDT */
    B_VARCHAR TYPE_NAME;    /* type name of the UDT */
}UDT_INFO_IN_RPC;

/* UDT_INFO = UDT_INFO_IN_COLMETADATA / UDT_INFO_IN_RPC */
typedef struct {
    UDT_INFO_IN_COLMETADATA UDT_INFO_inCOLMETADATA; /* when sent as part of COLMETADATA */
    UDT_INFO_IN_RPC UDT_INFO_inRPC; /* when sent as part of RPC call */
} UDT_INFO;

/* 2.2.5.6小节，TYPE_INFO规则
   <0>The TYPE_INFO rule applies to several messages used to describe column information.
   <0>For columns of fixed data length, the type is all that is required to determine the data length.
   <0>For columns of a variable-length type, TYPE_VARLEN defines the length of the data contained within the column, with
the following exceptions introduced in TDS 7.3:
   <1>DATE MUST NOT have a TYPE_VARLEN. The value is either 3 bytes or 0 bytes (null).
   <2>TIME, DATETIME2, and DATETIMEOFFSET MUST NOT have a TYPE_VARLEN. The lengths are
determined by the SCALE as indicated in section 2.2.5.4.2.
   <3>PRECISION and SCALE MUST occur if the type is NUMERIC, NUMERICN, DECIMAL, or DECIMALN.
   <4>SCALE (without PRECISION) MUST occur if the type is TIME, DATETIME2, or DATETIMEOFFSET
(introduced in TDS 7.3). PRECISION MUST be less than or equal to decimal 38 and SCALE MUST be
less than or equal to the precision value.
   <5>COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHRTYPE, TEXTTYPE, NTEXTTYPE,
NCHARTYPE, or NVARCHARTYPE.
   <6>UDT_INFO always occurs if the type is UDTTYPE.
   <7>XML_INFO always occurs if the type is XMLTYPE.
   <8>USHORTMAXLEN(USHORTMAXLEN = %xFFFF) does not occur if PARTLENTYPE is XMLTYPE or UDTTYPE.
   TYPE_INFO = FIXEDLENTYPE
            / (VARLENTYPE TYPE_VARLEN [COLLATION])
            / (VARLENTYPE TYPE_VARLEN [PRECISION SCALE])
            / (VARLENTYPE SCALE) ; (introduced in TDS 7.3)
            / VARLENTYPE ; (introduced in TDS 7.3)
            / (PARTLENTYPE
              [USHORTMAXLEN]
              [COLLATION]
              [XML_INFO]
              [UDT_INFO])*/
#define FIXEDLENTYPE_ONLY 0
#define VARLENTYPE_TYPE_VARLEN 1
#define VARLENTYPE_TYPE_VARLEN_COLLATION 2
#define VARLENTYPE_TYPE_VARLEN_PRECISION_SCALE 3
#define VARLENTYPE_SCALE 4
#define VARLENTYPE_ONLY 5
#define PARTLENTYPE_USHORTMAXLEN 6
#define PARTLENTYPE_USHORTMAXLEN_COLLATION 7
#define PARTLENTYPE_XML_INFO 8
#define PARTLENTYPE_UDT_INFO 9

struct __TYPE_INFO {
    UCHAR LENTYPE; /* =FIXEDLENTYPE / VARLENTYPE / PARTLENTYPE (2.2.5.4.1和2.2.5.4.2) */
    /* LENTYPE    = BYTELEN      / USHORTCHARBINLEN / LONGLEN (2.2.5.2.3)      /// 0xFFFF
   对应VARLENTYPE = BYTELEN_TYPE / USHORTLEN_TYPE   / LONGLEN_TYPE (2.2.5.4.2) /// PARTLENTYPE */
    ULONG TYPE_VARLEN; /* 4字节 =SSVARIANTTYPE, TEXTTYPE, NTEXTTYPE, or IMAGETYPE
                          2字节 = BIGCHARTYPE, BIGVARCHARTYPE, NCHARTYPE, NVARCHARTYPE, BIGBINARYTYPE, or BIGVARBINARYTYPE
                          1字节 = 对应其他类型
                          此处统一用最大的类型, UCHAR/USHORT都可以保存进来 */
    COLLATION COLLATIONval; /* COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHRTYPE, TEXTTYPE, NTEXTTYPE, NCHARTYPE, or NVARCHARTYPE.包括VARLENTYPE和PARTLENTYPE范围 */
    PRECISION precision; /* the precision of a numeric number */
    SCALE scale; /* the scale of a numeric number */
    XML_INFO xml_info;
    UDT_INFO udt_info;

    int typeFlag; /* 自己加的域，指明TYPE_INFO的格式 */
};

int TYPE_VARLEN_parse(unsigned char *data, int *len, unsigned char LENTYPE, TYPE_VARLEN *res) {
    unsigned char *pos = data;

    if (*len <= 0) {TDS_PRINT_ERR_MSG("*len <= 0"); return -1;}
    switch (LENTYPE) {
        case SSVARIANTTYPE:
        case TEXTTYPE:
        case NTEXTTYPE:
        case IMAGETYPE:
            STEP(pos, *len-(pos-data), 4, LONGLEN, res->len4);
            res->reallen = res->len4;
            break;
        case BIGCHARTYPE:
        case BIGVARCHRTYPE:
        case NCHARTYPE:
        case NVARCHARTYPE:
        case BIGBINARYTYPE:
        case BIGVARBINTYPE:
            STEP(pos, *len-(pos-data), 2, USHORTCHARBINLEN, res->len2);
            res->reallen = res->len2;
            break;
        case XMLTYPE: /*case BIGVARCHRTYPE: case BIGVARBINTYPE: case NVARCHARTYPE:*/
        case UDTTYPE:
            break;
        default:
            STEP(pos, *len-(pos-data), 1, BYTELEN, res->len1);
            res->reallen = res->len1;
    }

    *len = pos-data;
    return 0;
}

int TYPE_VARBYTE_parse (unsigned char *data, int *len, struct __TYPE_INFO *typeInfoRes, TYPE_VARBYTE *res) {
    unsigned char *pos = data;
    //ULONGLONGLEN tmplen;
    int leftlen, retval;

    /* TYPE_VARBYTE = PLP_BODY
       注意 - plp_chunk里的len和data最后要copy到res->len.reallen和res->data，方便以后使用*/
    switch (typeInfoRes->typeFlag) {
        case PARTLENTYPE_USHORTMAXLEN:
        case PARTLENTYPE_USHORTMAXLEN_COLLATION:
        case PARTLENTYPE_XML_INFO:
        case PARTLENTYPE_UDT_INFO:
            leftlen = *len;
            retval = PLP_BODY_parse(pos, &leftlen, &(res->plp_body));
            if (retval < 0) return -1;
            STEP_N(pos, *len, leftlen);
            *len = leftlen;
            return 0;
        default:
            break;
    }

    /* TYPE_VARBYTE = GEN_NULL / CHARBIN_NULL2 / CHARBIN_NULL4 / TYPE_VARLEN *BYTE */
    switch (typeInfoRes->LENTYPE) {
        case GUIDTYPE:
        case INTNTYPE:
        case DECIMALTYPE:
        case NUMERICTYPE:
        case BITNTYPE:
        case DECIMALNTYPE:
        case NUMERICNTYPE:
        case FLTNTYPE:
        case MONEYNTYPE:
        case DATETIMNTYPE:
        case DATENTYPE:
        case TIMENTYPE:
        case DATETIME2NTYPE:
        case DATETIMEOFFSETNTYPE:
        case CHARTYPE:
        case VARCHARTYPE:
        case BINARYTYPE :
        case VARBINARYTYPE:
            if (GEN_NULL == *pos) {
                res->data = NULL;
                *len = 1;
                return 0;
            }
            STEP(pos, *len, 1, BYTELEN, res->len.len1);
            res->len.reallen = res->len.len1;
            STEP_PL(pos, *len-(pos-data), res->data, res->len.reallen);
            *len = pos-data;
            return 0;
        case BIGVARBINTYPE:
        case BIGVARCHRTYPE:
        case BIGBINARYTYPE:
        case BIGCHARTYPE:
        case NVARCHARTYPE:
        case NCHARTYPE:
            if (CHARBIN_NULL2 == *(USHORT *)pos) {
                res->data = NULL;
                *len = 2;
                return 0;
            }
            STEP(pos, *len, 2, USHORTCHARBINLEN, res->len.len2);
            res->len.reallen = res->len.len2;
            STEP_PL(pos, *len-(pos-data), res->data, res->len.reallen);
            *len = pos-data;
            return 0;
        case XMLTYPE:
        case TEXTTYPE:
        case IMAGETYPE:
        case NTEXTTYPE:
        case SSVARIANTTYPE:
            if (CHARBIN_NULL4 == *(ULONG *)pos) {
                res->data = NULL;
                *len = 4;
                return 0;
            }
            STEP(pos, *len, 4, LONGLEN, res->len.len4);
            res->len.reallen = res->len.len4;
            STEP_PL(pos, *len-(pos-data), res->data, res->len.reallen);
            *len = pos - data;
            return 0;
    }

    /* TYPE_VARBYTE = *BYTE */
    switch (typeInfoRes->LENTYPE) {
        case NULLTYPE:     // Null,无后续数据
            res->data = NULL;
            res->len.reallen = 0;
            *len = 0;
            return 0;
        case INT1TYPE:     // TinyInt，1字节
        case BITTYPE:      // Bit，1字节
            res->data = (BYTE *)data;
            res->len.reallen = 1;
            *len = 1;
            return 0;
        case INT2TYPE:     // SmallInt，2字节
            res->data = (BYTE *)data;
            res->len.reallen = 2;
            *len = 2;
            return 0;
        case INT4TYPE:     // Int，4字节
        case DATETIM4TYPE: // SmallDateTime，4字节
        case FLT4TYPE:     // Real，4字节
        case MONEY4TYPE:   // SmallMoney，4字节
            res->data = (BYTE *)data;
            res->len.reallen = 4;
            *len = 4;
            return 0;
        case MONEYTYPE:    // Money，8字节
        case DATETIMETYPE: // DateTime，8字节
        case FLT8TYPE:     // Float，8字节
        case INT8TYPE:     // BigInt，8字节
            res->data = (BYTE *)data;
            res->len.reallen = 8;
            *len = 8;
            return 0;
    }
    *len = pos-data;
    return 0;
}

/*==================================================================================*/

int UDT_INFO_parse (unsigned char *data, int *len, int isRPC, UDT_INFO *res) {
    unsigned char *pos = data;
    int leftlen, retval;

    if (isRPC) {
        leftlen = *len;
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inRPC.DB_NAME1));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inRPC.SCHEMA_NAME));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inRPC.TYPE_NAME));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);
    } else {
        STEP(pos, *len, 2, USHORT, res->UDT_INFO_inCOLMETADATA.MAX_BYTE_SIZE);

        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inCOLMETADATA.DB_NAME1));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inCOLMETADATA.SCHEMA_NAME));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inCOLMETADATA.TYPE_NAME));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = US_VARCHAR_parse(pos, &leftlen, &(res->UDT_INFO_inCOLMETADATA.ASSEMBLY_QUALIFIED_NAME));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);
    }

    *len = pos - data;
    return 0;
}

/*==================================================================================*/
/* TDS 7.2引入 */
int XML_INFO_parse (unsigned char *data, int *len, XML_INFO *res) {
    unsigned char *pos = data;
    int leftlen, retval;

    STEP(pos, *len, 1, BYTE, res->SCHEMA_PRESENT);
    if (0x01 == res->SCHEMA_PRESENT) {
        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->DBNAME));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->OWNING_SCHEMA));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = (*len - (pos-data));
        retval = US_VARCHAR_parse(pos, &leftlen, &(res->XML_SCHEMA_COLLECTION));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);
    }

    *len = pos - data;
    return 0;
}

/*==================================================================================*/
/* 2.2.5.4小节，这些定义移到文件最上面了 */

/*==================================================================================*/
/* 2.2.5.5小节，TVP规则，由于这节用了2.2.5.6节的TYPE_INFO定义，所以挪到2.2.5.6节后面 */

/*==================================================================================*/
/* 2.2.5.6小节，TYPE_INFO规则
   <1>DATE MUST NOT have a TYPE_VARLEN. The value is either 3 bytes or 0 bytes (null).
   <2>TIME, DATETIME2, and DATETIMEOFFSET MUST NOT have a TYPE_VARLEN. The lengths are
determined by the SCALE as indicated in section 2.2.5.4.2.
   <3>PRECISION and SCALE MUST occur if the type is NUMERIC, NUMERICN, DECIMAL, or DECIMALN.
   <4>SCALE (without PRECISION) MUST occur if the type is TIME, DATETIME2, or DATETIMEOFFSET
(introduced in TDS 7.3). PRECISION MUST be less than or equal to decimal 38 and SCALE MUST be
less than or equal to the precision value.
   <5>COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHRTYPE, TEXTTYPE, NTEXTTYPE,
NCHARTYPE, or NVARCHARTYPE.
   <6>UDT_INFO always occurs if the type is UDTTYPE.
   <7>XML_INFO always occurs if the type is XMLTYPE.
   <8>USHORTMAXLEN does not occur if PARTLENTYPE is XMLTYPE or UDTTYPE.
   USHORTMAXLEN = %xFFFF
   TYPE_INFO = FIXEDLENTYPE
            / (VARLENTYPE TYPE_VARLEN [COLLATION])
            / (VARLENTYPE TYPE_VARLEN [PRECISION SCALE])
            / (VARLENTYPE SCALE) ; (introduced in TDS 7.3)
            / VARLENTYPE ; (introduced in TDS 7.3)
            / (PARTLENTYPE
              [USHORTMAXLEN]
              [COLLATION]
              [XML_INFO]
              [UDT_INFO])*/
int TYPE_INFO_parse (unsigned char *data, int *len, struct __TYPE_INFO *TYPE_INFO, int isRPC) {
    unsigned char * pos = data;
    int leftlen, retval;

    STEP(pos, *len, 1, UCHAR, TYPE_INFO->LENTYPE);

    switch (TYPE_INFO->LENTYPE) {
        case XMLTYPE:/* TYPE_INFO = PARTLENTYPE XML_INFO */
            leftlen = *len - (pos-data);
            retval = XML_INFO_parse(pos, &leftlen, &(TYPE_INFO->xml_info));
            if (retval < 0) return -1;
            STEP_N(pos, *len - (pos-data), leftlen);

            TYPE_INFO->typeFlag = PARTLENTYPE_XML_INFO;
            *len = pos - data;
            return 0;
        case UDTTYPE:/* TYPE_INFO = PARTLENTYPE UDT_INFO */
            leftlen = *len - (pos-data);
            retval = UDT_INFO_parse(pos, &leftlen, isRPC, &(TYPE_INFO->udt_info));
            if (retval < 0) return -1;
            STEP_N(pos, *len - (pos-data), leftlen);

            TYPE_INFO->typeFlag = PARTLENTYPE_UDT_INFO;
            *len = pos - data;
            return 0;
        case BIGVARBINTYPE:/* TYPE_INFO = PARTLENTYPE 0xFFFF */
            if (0xFFFF == *(USHORTLEN *)pos) {
                STEP_N(pos, *len-(pos-data), 2);
                TYPE_INFO->typeFlag = PARTLENTYPE_USHORTMAXLEN;
                *len = pos - data;
                return 0;
            }
            break;
        case BIGVARCHRTYPE:/* TYPE_INFO = PARTLENTYPE 0xFFFF COLLATION */
        case NVARCHARTYPE:
            if (0xFFFF == *(USHORTLEN *)pos) {
                STEP_N(pos, *len-(pos-data), 7);
                TYPE_INFO->typeFlag = PARTLENTYPE_USHORTMAXLEN_COLLATION;
                *len = pos - data;
                return 0;
            }
            break;
    }

    /* TYPE_INFO = FIXEDLENTYPE */
    switch (TYPE_INFO->LENTYPE) {
        /* FIXEDLENTYPE类型范围 */
        case NULLTYPE:
        case INT1TYPE:
        case BITTYPE:
        case INT2TYPE:
        case INT4TYPE:
        case DATETIM4TYPE:
        case FLT4TYPE:
        case MONEYTYPE:
        case DATETIMETYPE:
        case FLT8TYPE:
        case MONEY4TYPE:
        case INT8TYPE:
            TYPE_INFO->typeFlag = FIXEDLENTYPE_ONLY;
            *len = pos - data;
            return 0;

        /* VARLENTYPE=BYTELEN_TYPE / USHORTLEN_TYPE / LONGLEN_TYPE类型范围 */
        /* BYTELEN_TYPE范围 */
        case GUIDTYPE:
        case INTNTYPE:
        case DECIMALTYPE:
        case NUMERICTYPE:
        case BITNTYPE:
        case DECIMALNTYPE:
        case NUMERICNTYPE:
        case FLTNTYPE:
        case MONEYNTYPE:
        case DATETIMNTYPE:
        case DATENTYPE:
        case TIMENTYPE:
        case DATETIME2NTYPE:
        case DATETIMEOFFSETNTYPE:
        case CHARTYPE:
        case VARCHARTYPE:
        case BINARYTYPE:
        case VARBINARYTYPE:

        /* USHORTLEN_TYPE范围 */
        case BIGVARBINTYPE:
        case BIGVARCHRTYPE:
        case BIGBINARYTYPE:
        case BIGCHARTYPE:
        case NVARCHARTYPE:
        case NCHARTYPE:

        /* LONGLEN_TYPE范围 */
        case XMLTYPE:
        case TEXTTYPE:
        case IMAGETYPE:
        case NTEXTTYPE:
        case SSVARIANTTYPE:

            /* TYPE_INFO = VARLENTYPE */
            /* <1>DATE MUST NOT have a TYPE_VARLEN. The value is either 3 bytes or 0 bytes (null). */
            if (DATENTYPE == TYPE_INFO->LENTYPE) {
                TYPE_INFO->typeFlag = VARLENTYPE_ONLY;
                *len = pos - data;
                return 0;
            }

            /* TYPE_INFO = VARLENTYPE SCALE */
            /* <2>TIME, DATETIME2, and DATETIMEOFFSET MUST NOT have a TYPE_VARLEN. The lengths are determined by the SCALE */
            /* <4>SCALE (without PRECISION) MUST occur if the type is TIME, DATETIME2, or DATETIMEOFFSET (introduced in TDS 7.3). PRECISION MUST be less than or equal to decimal 38 and SCALE MUST be less than or equal to the precision value. */
            if (TIMENTYPE==TYPE_INFO->LENTYPE
             || DATETIME2NTYPE==TYPE_INFO->LENTYPE
             || DATETIMEOFFSETNTYPE==TYPE_INFO->LENTYPE) {
                STEP(pos, *len-(pos-data), 1, SCALE, TYPE_INFO->scale);
                TYPE_INFO->typeFlag = VARLENTYPE_SCALE;
                *len = pos - data;
                return 0;
            }

            /* TYPE_INFO = VARLENTYPE TYPE_VARLEN PRECISION SCALE*/
            /* <3>PRECISION and SCALE MUST occur if the type is NUMERIC, NUMERICN, DECIMAL, or DECIMALN. */
            if (NUMERICTYPE==TYPE_INFO->LENTYPE
             || NUMERICNTYPE==TYPE_INFO->LENTYPE
             || DECIMALTYPE==TYPE_INFO->LENTYPE
             || DECIMALNTYPE==TYPE_INFO->LENTYPE) {
                STEP(pos, *len-(pos-data), 1, UCHAR, TYPE_INFO->TYPE_VARLEN);
                STEP(pos, *len-(pos-data), 1, PRECISION, TYPE_INFO->precision);
                STEP(pos, *len-(pos-data), 1, SCALE, TYPE_INFO->scale);
                TYPE_INFO->typeFlag = VARLENTYPE_TYPE_VARLEN_PRECISION_SCALE;
                *len = pos - data;
                return 0;
            }

            /* TYPE_INFO = VARLENTYPE TYPE_VARLEN COLLATION*/
            /* <5>COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHRTYPE, TEXTTYPE, NTEXTTYPE, NCHARTYPE, or NVARCHARTYPE */
            if (BIGCHARTYPE==TYPE_INFO->LENTYPE
             || BIGVARCHRTYPE==TYPE_INFO->LENTYPE
             || NCHARTYPE==TYPE_INFO->LENTYPE
             || NVARCHARTYPE==TYPE_INFO->LENTYPE
             || TEXTTYPE==TYPE_INFO->LENTYPE
             || NTEXTTYPE==TYPE_INFO->LENTYPE ) {

                 if (TEXTTYPE==TYPE_INFO->LENTYPE
                 || NTEXTTYPE==TYPE_INFO->LENTYPE) {
                     STEP(pos, *len-(pos-data), 4, ULONG, TYPE_INFO->TYPE_VARLEN);
                 } else {
                     STEP(pos, *len-(pos-data), 2, USHORT, TYPE_INFO->TYPE_VARLEN);
                 }

                leftlen = *len - (pos-data);
                retval = COLLATION_parse(pos, &leftlen, &(TYPE_INFO->COLLATIONval));
                if (retval < 0) return -1;
                STEP_N(pos, *len-(pos-data), leftlen);

                TYPE_INFO->typeFlag = VARLENTYPE_TYPE_VARLEN_COLLATION;
                *len = pos - data;
                return 0;
            }

            /* TYPE_INFO = VARLENTYPE TYPE_VARLEN*/
            if (IMAGETYPE==TYPE_INFO->LENTYPE || SSVARIANTTYPE==TYPE_INFO->LENTYPE ) {
                STEP(pos, *len-(pos-data), 4, ULONG, TYPE_INFO->TYPE_VARLEN);
            } else if (BIGBINARYTYPE==TYPE_INFO->LENTYPE || BIGVARBINTYPE==TYPE_INFO->LENTYPE ) {
                STEP(pos, *len-(pos-data), 2, USHORT, TYPE_INFO->TYPE_VARLEN);
            } else {
                STEP(pos, *len-(pos-data), 1, UCHAR, TYPE_INFO->TYPE_VARLEN);
            }
            TYPE_INFO->typeFlag = VARLENTYPE_TYPE_VARLEN;
            *len = pos - data;
            return 0;
        default:
            fprintf(stderr, "ERROR : <%s> : %d : Type=%x\n", __FILE__, __LINE__, TYPE_INFO->LENTYPE);
            break;
    }
    return -1;
}

/*==================================================================================*/
/* 2.2.5.5小节，TVP规则，由于这节用了2.2.5.6节的TYPE_INFO定义，所以挪到此处
   At the present time, TVPs are permitted to be used only as input parameters and do not appear as output
parameters or in result set columns.
   TVPs MUST be sent only by a TDS client that reports itself as a TDS major version 7.3 or later. If a
client reporting itself as older than TDS 7.3 attempts to send a TVP, the server MUST reject the
request with a TDS protocol error.*/

typedef struct {
    B_VARCHAR DbName; /* Database where TVP type resides */
    B_VARCHAR OwningSchema; /* Schema where TVP type resides */
    B_VARCHAR TypeName; /* TVP type name */
} TVP_TYPENAME;

typedef struct {
    ULONG UserType; /* UserType of column */
    USHORT Flags;
    struct __TYPE_INFO TYPE_INFO;
    B_VARCHAR ColName; /* Name of column */
} TvpColumnMetaData;

typedef struct {
    USHORT Count; /* Column count up to 1024 max, 若等于0xFFFF,则本结构体就这一个域，后面没数据 */
    TvpColumnMetaData TvpColumnMetaData;
} TVP_COLMETADATA;

typedef struct {
    USHORT ColNum; /* A single-column ordinal, start with 1 */
    BYTE OrderUniqueFlags; /*  */
} TVP_ORDER_UNIQUE_sub;

typedef struct {
    BYTE TVP_ORDER_UNIQUE_TOKEN; /* =0x10 */
    USHORT Count; /* Count of ColNums to follow */
    TVP_ORDER_UNIQUE_sub sub; /* Count个该结构 */
} TVP_ORDER_UNIQUE;

typedef struct {
    BYTE TVP_COLUMN_ORDERING_TOKEN; /* =0x11 */
    USHORT Count; /* Count of ColNums to follow */
    USHORT ColNum; /* A single-column ordinal, start with 1 ，这里是'Count'个'ColNum'，此处做缓存用*/
} TVP_COLUMN_ORDERING;

/* Terminator tag for TVP type meaning no moreTVP_ROWs to
   follow and end of successful transmission of a single TVP */
#define TVP_END_TOKEN 0x00
typedef struct {
    BYTE TokenType; /* =TVP_ROW_TOKEN */
    TYPE_VARBYTE AllColumnData; /* 多个TYPE_VARBYTE，仅做缓存用. The actual data for the TVP column. */
} TVP_ROW_other;

typedef struct {
    BYTE TVPTYPE; /* =0xF3 */
    TVP_TYPENAME tvp_typename; /* Type name of the TVP */
    TVP_COLMETADATA tvp_COLMETADATA; /* Column-specific metadata */
    TVP_ORDER_UNIQUE tvp_order_unique; /* Optional metadata token */
    TVP_COLUMN_ORDERING tvp_column_ordering; /* Optional metadata token */
    TVP_ROW_other tvp_row; /* 0..N TVP_ROW tokens 有多个，此处缓存用，以TVP_END_TOKEN结尾 */
} TVP_TYPE_INFO;


/*===============================================================================*/
/* 来自2.2.5.7节 */
/* The metadata and encrypted value that describe an encryption key. */
typedef struct {
    US_VARBYTE EncryptedKey;  /* The ciphertext containing the encryption key that is secured with the master. */
    B_VARCHAR KeyStoreName;   /* The key store name component of the location where the master key is saved */
    US_VARCHAR KeyPath;       /* The key path component of the location where the master key is saved. */
    B_VARCHAR AsymmetricAlgo; /* The name of the algorithm that is used for encrypting the encryption key */
} EK_INFO_EncryptionKeyValue;

typedef struct {
    ULONG DatabaseId; /* A 4 byte integer value that represents the database ID where the column encryption key is stored. */
    ULONG CekId; /* An identifier for the column encryption key. */
    ULONG CekVersion; /* The key version of the column encryption key.  */
    ULONGLONG CekMDVersion; /* The metadata version for the column encryption key. */
    BYTE Count; /* The count of EncryptionKeyValue elements that are present in the message. */
    EK_INFO_EncryptionKeyValue EncryptionKeyValue; /*  */
} EK_INFO;

int EK_INFO_parse (unsigned char *data, int *len, EK_INFO *res) {
    unsigned char *pos = data;
    int leftlen, retval, n;

    STEP(pos, *len-(pos-data), 4, ULONG, res->DatabaseId);
    STEP(pos, *len-(pos-data), 4, ULONG, res->CekId);
    STEP(pos, *len-(pos-data), 4, ULONG, res->CekVersion);
    STEP(pos, *len-(pos-data), 8, ULONGLONG, res->CekMDVersion);
    STEP(pos, *len-(pos-data), 1, BYTE, res->Count);

    n = res->Count;
    while (n-- > 0) {
        leftlen = *len - (pos - data);
        retval = US_VARBYTE_parse(pos, &leftlen, &(res->EncryptionKeyValue.EncryptedKey));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = *len - (pos - data);
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->EncryptionKeyValue.KeyStoreName));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = *len - (pos - data);
        retval = US_VARCHAR_parse(pos, &leftlen, &(res->EncryptionKeyValue.KeyPath));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);

        leftlen = *len - (pos - data);
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->EncryptionKeyValue.AsymmetricAlgo));
        if (retval < 0) return -1;
        STEP_N(pos, *len-(pos-data), leftlen);
    }

    *len = pos - data;
    return 0;
}



/*==================================================================================*/
/* 2.2.5.8节, token标志，类别根据第2.2.4节判断, 上面实现宏定义 */
#define ALTMETADATA_TOKEN 0x88   /* 1000 1000, 属于Variable Count Tokens */
#define ALTROW_TOKEN 0xD3        /* 1101 0011, 属于Zero Length Token */
#define COLMETADATA_TOKEN 0x81   /* 1000 0001, 属于Variable Count Tokens */
#define COLINFO_TOKEN 0xA5       /* 1010 0101, 属于Variable Length Tokens */
#define DONE_TOKEN 0xFD          /* 1111 1101, 属于Fixed Length Token */
#define DONEPROC_TOKEN 0xFE      /* 1111 1110, 属于Fixed Length Token */
#define DONEINPROC_TOKEN   0xFF  /* 1111 1111, 属于Fixed Length Token */
#define ENVCHANGE_TOKEN 0xE3     /* 1110 0011, 属于Variable Length Tokens */
#define ERROR_TOKEN 0xAA         /* 1010 1010, 属于Variable Length Tokens */
#define FEATUREEXTACK_TOKEN 0xAE /* 1010 1110, 属于Variable Length Tokens */
#define FEDAUTHINFO_TOKEN 0xEE   /* 1110 1110, 属于Variable Length Tokens */
#define INFO_TOKEN 0xAB          /* 1010 1011, 属于Variable Length Tokens */
#define LOGINACK_TOKEN 0xAD      /* 1010 1101, 属于Variable Length Tokens */
#define NBCROW_TOKEN    0xD2     /* 1101 0010, 属于Zero Length Token */
#define OFFSET_TOKEN 0x78        /* 0111 1000, 属于Fixed Length Token */
#define ORDER_TOKEN 0xA9         /* 1010 1001, 属于Variable Length Tokens */
#define RETURNSTATUS_TOKEN 0x79  /* 0111 1001, 属于Fixed Length Token */
#define RETURNVALUE_TOKEN 0xAC   /* 1010 1100, 属于Variable Length Tokens */
#define ROW_TOKEN 0xD1           /* 1101 0001, 属于Zero Length Token  */
#define SESSIONSTATE_TOKEN 0xE4  /* 1110 0100, 属于Variable Length Tokens */
#define SSPI_TOKEN   0xED        /* 1110 1101, 属于Variable Length Tokens */
#define TABNAME_TOKEN 0xA4       /* 1010 0100, 属于Variable Length Tokens */

#define TVP_ROW_TOKEN 0x01       /* 2.2.5.5.5.2节，0000 0001,  */

/*==================================================================================*/
/* 2.2.7.1节, ALTMETADATA_TOKEN
   <1>Describes the data type, length, and name of column data that result from a SQL
   statement that generates totals.
   <2>The token value is 0x88.
   <3>This token is used to tell the client the data type and length of the column data. It describes the
   format of the data found in an ALTROW data stream. The ALTMETADATA and corresponding ALTROW
   MUST be in the same result set.
   <4>All ALTMETADATA data streams are grouped.
   <5>A preceding COLMETADATA MUST exist before an ALTMETADATA token. There might be COLINFO and
   TABNAME streams between COLMETADATA and ALTMETADATA. */

/* TableName = US_VARCHAR ; (removed in TDS 7.2)
               /
              (NumParts
               1*PartName); (introduced in TDS 7.2) */
typedef struct {
    BYTE NumParts; /* introduced in TDS 7.2 */
    US_VARCHAR PartName; /* introduced in TDS 7.2, 'NumParts'个'PartName', 此处仅做缓存 */

    US_VARCHAR TableName; /* removed in TDS 7.2 */
}ALTMETADATA_TableName;

/* 'struct __ComputeData.op'的取值范围，代表sqlserver的一些聚合函数 */
#define AOPSTDEV 0x30  //Standard deviation (STDEV)
#define AOPSTDEVP 0x31 //Standard deviation of the population (STDEVP)
#define AOPVAR 0x32    //Variance (VAR)
#define AOPVARP 0x33   //Variance of population (VARP)
#define AOPCNT 0x4B    //Count of rows (COUNT)
#define AOPSUM 0x4D    //Sum of the values in the rows (SUM)
#define AOPAVG 0x4F    //Average of the values in the rows (AVG)
#define AOPMIN 0x51    //Minimum value of the rows (MIN)
#define AOPMAX 0x52    //Maximum value of the rows (MAX)

typedef struct {
    BYTE Op;        /* The type of aggregate operator */
    USHORT Operand; /* The column number, starting from 1, in the result set that is the operand to the aggregate operator. */
    ULONG UserType; /* UserType = USHORT/ULONG; (changed to ULONG in TDS 7.2). The user typeID of the data type of the column. */
    USHORT Flags;
    struct __TYPE_INFO TYPE_INFO; /* 列的类型 */
    ALTMETADATA_TableName TableName; /* 注意 - The TableName field is specified only if text, ntext, or image columns are included in the result set. */
    B_VARCHAR ColName;   /* The column name. */
}ALTMETADATA_ComputeData;

typedef struct {
    BYTE TokenType; /* =ALTMETADATA_TOKEN */
    USHORT Count;   /* The count of columns (number of aggregate operators) */
    USHORT Id;      /* The Id of the SQL statement to which the total column formats apply. */
    UCHAR ByCols;   /* The number of grouping columns in the SQL statement that generates totals. */
    USHORT ColNum[1024]; /* 注意 - specifying the column number as it appears in the COMPUTE clause. 'ColNum' appears 'ByCols' times. 这里只做缓存用 */
    ALTMETADATA_ComputeData ComputeData[1024]; /* 'ByCols'个ComputeData，但这里只做缓存用 */
} ALTMETADATA;
static ALTMETADATA ALTMETADATAres;

int ALTMETADATA_token_parse(unsigned char *token, int *tlen, ALTMETADATA *res, int isRPC) {
    unsigned char *pos = token;
    int leftlen, retval, i;

    res->TokenType = *pos++;
    if (ALTMETADATA_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Count);
    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Id);
    STEP(pos, *tlen-(pos-token), 1, UCHAR, res->ByCols);

    if (res->ByCols > sizeof(res->ColNum)) {
        TDS_PRINT_ERR_MSG("Fail to parse ALTMETADATA(ByCols too big)");
        return -1;
    }
    for (i=0; i<res->ByCols; i++) {
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->ColNum[i]);
    }

    if (res->Count > 1024) {
        TDS_PRINT_ERR_MSG("Fail to parse ALTMETADATA(Count too big)");
        return -1;
    }
    for (i=0; i<res->Count; i++) {
        STEP(pos, *tlen-(pos-token), 1, BYTE, res->ComputeData[i].Op);
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->ComputeData[i].Operand);
        if (TDS_VERSION <= TDS_V_7_2) {
            STEP(pos, *tlen-(pos-token), 4, ULONG, res->ComputeData[i].UserType);
        } else {
            STEP(pos, *tlen-(pos-token), 2, USHORT, res->ComputeData[i].UserType);
        }
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->ComputeData[i].Flags);

        leftlen = *tlen - (pos-token);
        retval = TYPE_INFO_parse(pos, &leftlen, &(res->ComputeData[i].TYPE_INFO), isRPC);
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);

        switch (res->ComputeData[i].TYPE_INFO.LENTYPE) {
            case TEXTTYPE:
            case IMAGETYPE:
            case NTEXTTYPE:
                if (TDS_VERSION <= TDS_V_7_2) {
                    STEP(pos, *tlen-(pos-token), 2, BYTE, res->ComputeData[i].TableName.NumParts);

                    leftlen = *tlen - (pos-token);
                    retval = US_VARCHAR_parse(pos, &leftlen, &(res->ComputeData[i].TableName.PartName));
                    if (retval < 0) return -1;
                    STEP_N(pos, *tlen-(pos-token), leftlen);
                } else {
                    leftlen = *tlen - (pos-token);
                    retval = US_VARCHAR_parse(pos, &leftlen, &(res->ComputeData[i].TableName.TableName));
                    if (retval < 0) return -1;
                    STEP_N(pos, *tlen-(pos-token), leftlen);
                }
                leftlen = *tlen - (pos-token);
                retval = B_VARCHAR_parse(pos, &leftlen, &(res->ComputeData[i].ColName));
                if (retval < 0) return -1;
                STEP_N(pos, *tlen-(pos-token), leftlen);
        }
    }

    *tlen = pos - token;
    return 0;
}

/*==================================================================================*/
/* 2.2.7.2节, ALTROW_TOKEN */
/* <1>Used to send a complete row of total data, where the data format is provided
      by the ALTMETADATA token (2.2.7.1节).
   <2>The ALTROW token is similar to the ROW_TOKEN, but also contains an Id field.
      This Id matches an Id given in ALTMETADATA (one Id for each SQL statement).
      This provides the mechanism for matching row data with correct SQL statements.*/
typedef struct  {
    BYTE TokenType; /* =ALTROW_TOKEN */
    USHORT Id; /* The Id of the SQL statement that generates totals to which the total column formats apply */
    TYPE_VARBYTE ComputeData[256]; /* The actual data for the column. The ComputeData element is repeated Count times (where Count is specified in ALTMETADATA_TOKEN). 此处暂做缓存用 */
} ALTROW;
static ALTROW ALTROWres;

int ALTROW_token_parse(unsigned char *token, int *tlen, ALTROW *res, ALTMETADATA *ALTMETADATAres) {
    unsigned char *pos = token;
    int leftlen, retval, i;

    res->TokenType = *pos++;
    if (ALTROW_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Id);
    if (res->Id != ALTMETADATAres->Id) {
        TDS_PRINT_ERR_MSG("res->Id != ALTMETADATAres->Id");
    }

    for (i=0; i<ALTMETADATAres->Count; i++) {
        leftlen = *tlen-(pos-token);
        retval = TYPE_VARBYTE_parse(pos, &leftlen, &(ALTMETADATAres->ComputeData[i].TYPE_INFO), &(res->ComputeData[i]));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    *tlen = pos - token;
    return 0;
}

/*==================================================================================*/
/* 2.2.7.3节, COLINFO_TOKEN */
/* <1>Describes the column information in browse mode (described in [MSDN-BROWSE]), sp_cursoropen,
   and sp_cursorfetch.
   <2>The TABNAME token contains the actual table name associated with COLINFO.*/

#define EXPRESSION 0x04 /* the column was the result of an expression */
#define KEY 0x08 /* the column is part of a key for the associated table */
#define HIDDEN 0x10 /* the column was not requested, but was added because it was part of a key for the associated table */
#define DIFFERENT_NAME 0x20 /* the column name is different than the requested column name in the case of a column alias */

typedef struct {
    BYTE ColNum;   /* The column number in the result set */
    BYTE TableNum; /* The number of the base table that the column was derived from. The value is 0 if the value of Status is EXPRESSION. */
    BYTE Status;   /* 取EXPRESSION / KEY / HIDDEN / DIFFERENT_NAME*/
    B_VARCHAR ColName; /* The base column name. This only occurs if DIFFERENT_NAME is set in Status.(这个要注意) */
}COLINFO_CpLProperty;

typedef struct {
    BYTE TokenType; /* COLINFO_TOKEN */
    USHORT Length;  /* The actual data length, in bytes, of the ColProperty stream. The length does not include token type and length field. */
    COLINFO_CpLProperty CpLProperty[256]; /* The ColInfo element is repeated for each column in the result set. */
}COLINFO;

int COLINFO_token_parse(unsigned char *token, int *tlen, COLINFO *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (COLINFO_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);
    STEP_N(pos, *tlen-(pos-token), res->Length); /* 直接跳过后面的'CpLProperty'域 */

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.4 */
/* Describes the result set for interpretation of following ROW data streams.
   This token is used to tell the client the data type and length of the column data. It describes the
format of the data found in a ROW data stream.
   All COLMETADATA data streams are grouped together.*/
typedef struct  {
    USHORT EkValueCount; /* The size of CekTable. It represents the number of entries in CekTable.*/
    EK_INFO ek_info;
} COLMETADATA_CekTable;

typedef struct {
    BYTE NumParts; /* PartName的个数 */
    US_VARCHAR PartName;
}COLMETADATA_TableName;

#define EncryptionAlgoType_DE 1 //Deterministic encryption
#define EncryptionAlgoType_RE 2 //Randomized encryption
/* This describes the encryption metadata for a column. */
typedef struct {
    USHORT Ordinal; /* Where the encryption key information is located in CekTable. Ordinal starts at 0. */
    ULONG UserType; /* 从TDS7.2开始是unsigned long了 */
    struct __TYPE_INFO BaseTypeInfo; /* TYPE_INFO类型 */
    BYTE EncryptionAlgo; /* A byte that describes the encryption algorithm that is used. */
    B_VARCHAR AlgoName; /* Algorithm name literal that is used to encrypt the plaintext value. Unicode编码*/
    BYTE EncryptionAlgoType; /* encryption algorithm type */
    BYTE NormVersion; /* The normalization version to which plaintext data MUST be normalized. Version numbering starts at 0x01.  */
} COLMETADATA_CryptoMetaData;

typedef struct {
    ULONG UserType; /* UserType = USHORT/ULONG; (Changed to ULONG in TDS 7.2). The user type ID of the data type of the column. 若是0xffff, 则没有后面的域 */
    USHORT Flags;
    struct __TYPE_INFO TYPE_INFO; /* 之处本token后面的token的数据类型 */
    /* The TableName element is specified only if text, ntext, or image(由前一个域TYPE_INFO指定)
       columns are included in the result set. */
    COLMETADATA_TableName TableName; /* TDS 7.2引入 */
    COLMETADATA_CryptoMetaData CryptoMetaData; /* TDS 7.4引入 */
    B_VARCHAR ColName; /* The column name, Unicode编码 */
} COLMETADATA_ColumnData;

/* Describes the result set for interpretation of following ROW data streams.
   This token is used to tell the client the data type and length of the column data. It describes the
   format of the data found in a ROW data stream */
typedef struct {
    BYTE TokenType; /* COLMETADATA_TOKEN */
    USHORT Count; /* The count of columns (number of aggregate operators) in the token stream */
    COLMETADATA_CekTable CekTable; /* TDS 7.4引入. A table of various encryption keys that are used to secure the plaintext data. */

    /* 若前2个字节是0xffff,则该部分就这一字节。否则就指向多个ColumnData结构*/
    COLMETADATA_ColumnData ColumnData[1024];
} COLMETADATA;
static COLMETADATA COLMETADATAres;

/* 'token'指向token块开头。
   'tlen'为该块数据最大长度，并用它返回该token的大小
   'res'保存感兴趣的解析参数
   成功返回0，失败返回-1 */
int COLMETADATA_token_parse(unsigned char *token, int *tlen, COLMETADATA *res, int isRPC) {
    unsigned char *pos = token;
    int leftlen, retval, i, max;

    STEP(pos, *tlen, 1, UCHAR, res->TokenType);
    if (COLMETADATA_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Count);
    /* In the event that the client requested no metadata to be returned (see section 2.2.6.6 for
       information about the OptionFlags parameter in the RPCRequest token), the value of Count will be
       0xFFFF. This has the same effect on Count as a zero value (for example, no ColumnData is sent). */
    if (0xffff == res->Count) res->Count = 0;

    if (TDS_VERSION >= TDS_V_7_4) {
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->CekTable.EkValueCount);

        leftlen = *tlen - (pos - token);
        retval = EK_INFO_parse(pos, &leftlen, &(res->CekTable.ek_info));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    if (0xff == *pos && 0xff == *(pos+1)) { /* 遇到NoMetaData */
        *tlen = pos-token+2;
        return 0;
    }

    max = (res->Count > 256) ? 256 : res->Count;
    for (i=0; i<max; i++) {
        if (TDS_VERSION >= TDS_V_7_2) {
            STEP(pos, *tlen-(pos-token), 4, ULONG, res->ColumnData[i].UserType);
        } else {
            STEP(pos, *tlen-(pos-token), 2, USHORT, res->ColumnData[i].UserType);
        }
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->ColumnData[i].Flags);

        leftlen = *tlen - (pos - token);
        retval = TYPE_INFO_parse(pos, &leftlen, &(res->ColumnData[i].TYPE_INFO), isRPC);
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);

        if (TDS_VERSION >= TDS_V_7_2) {
            if (TEXTTYPE == res->ColumnData[i].TYPE_INFO.LENTYPE
                || NTEXTTYPE == res->ColumnData[i].TYPE_INFO.LENTYPE
                || IMAGETYPE == res->ColumnData[i].TYPE_INFO.LENTYPE) {
                STEP(pos, *tlen-(pos-token), 1, BYTE, res->ColumnData[i].TableName.NumParts);
                while (res->ColumnData[i].TableName.NumParts-- > 0) {
                    leftlen = *tlen - (pos - token);
                    retval = US_VARCHAR_parse(pos, &leftlen, &(res->ColumnData[i].TableName.PartName));
                    if (retval < 0) return -1;
                    STEP_N(pos, *tlen-(pos-token), leftlen);
                }
            }
        }


        if (TDS_VERSION >= TDS_V_7_4) {
            STEP(pos, *tlen-(pos-token), 2, USHORT, res->ColumnData[i].CryptoMetaData.Ordinal);
            STEP(pos, *tlen-(pos-token), 4, ULONG, res->ColumnData[i].CryptoMetaData.UserType);

            leftlen = *tlen - (pos - token);
            retval = TYPE_INFO_parse(pos, &leftlen, &(res->ColumnData[i].CryptoMetaData.BaseTypeInfo), isRPC);
            if (retval < 0) return -1;
            STEP_N(pos, *tlen-(pos-token), leftlen);

            STEP(pos, *tlen-(pos-token), 1, BYTE, res->ColumnData[i].CryptoMetaData.EncryptionAlgo);

            leftlen = *tlen - (pos - token);
            retval = B_VARCHAR_parse(pos, &leftlen, &(res->ColumnData[i].CryptoMetaData.AlgoName));
            if (retval < 0) return -1;
            STEP_N(pos, *tlen-(pos-token), leftlen);

            STEP(pos, *tlen-(pos-token), 1, BYTE, res->ColumnData[i].CryptoMetaData.EncryptionAlgoType);
            STEP(pos, *tlen-(pos-token), 1, BYTE, res->ColumnData[i].CryptoMetaData.NormVersion);
        }

        leftlen = *tlen - (pos - token);
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->ColumnData[i].ColName));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    *tlen = pos - token;
    return 0;
}

/* 把'res'中有效信息填入'resultStr' */
int getDescriptionInCOLMETADATA(COLMETADATA *res, char *resultStr, int maxsize) {
    int i, tocopy, retval;
    char ColNameUCS2[1024], ColName[1024];

    for (i=0; i<res->Count; i++) {
        tocopy = (sizeof(ColNameUCS2)-1) > (res->ColumnData[i].ColName.wlen * 2) ?
                 (res->ColumnData[i].ColName.wlen * 2) : (sizeof(ColNameUCS2)-1);
        memcpy(ColNameUCS2, res->ColumnData[i].ColName.data, tocopy);
        retval = codeConv("UCS-2", TDS_CHARSET, ColNameUCS2, tocopy, ColName, sizeof(ColName)-1);
        if (retval < 0) return -1;
        if (0 != i && maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:s");
        if (maxsize-strlen(resultStr) > strlen(ColName)) strcat(resultStr, ColName);
    }
    if (maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:n");
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.5节
   <1>Indicates the completion status of a SQL statement.
   <2>This token is used to indicate the completion of a SQL statement. As multiple SQL statements can
be sent to the server in a single SQL batch, multiple DONE tokens can be generated. In this case,
all but the final DONE token will have a Status value with DONE_MORE bit set (details follow).
   <3>A DONE token is returned for each SQL statement in the SQL batch except variable declarations.
   <4>For execution of SQL statements within stored procedures, DONEPROC and DONEINPROC tokens
are used in place of DONE tokens.
*/
#define DONE_FINAL 0x00     /* This DONE is the final DONE in the request */
#define DONE_MORE 0x01      /*  This DONE message is not the final DONE message in the response */
#define DONE_ERROR 0x02     /* An error occurred on the current SQL statement. */
#define DONE_INXACT 0x04    /* A transaction is in progress */
#define DONE_COUNT 0x10     /* The DoneRowCount value is valid */
#define DONE_ATTN 0x20      /* The DONE message is a server acknowledgement of a client ATTENTION message */
#define DONE_RPCINBATCH 0x80
#define DONE_SRVERROR 0x100 /* Used in place of DONE_ERROR when an error occurred on the current SQL statement */

typedef struct {
    BYTE TokenType; /* =DONE_TOKEN */
    USHORT Status; /* 它的值是上面宏中几个的位与 */
    USHORT CurCmd; /* The token of the current SQL statement */
    /* The count of rows that were affected by the SQL statement.  */
    ULONGLONG DoneRowCount; /* DoneRowCount = LONG / ULONGLONG; (Changed to ULONGLONG in TDS 7.2) */
} DONE;

/* 'token'指向token块开头。
   'tlen'为该块数据最大长度，并用它返回该token的大小
   'res'保存感兴趣的解析参数
   成功返回0，失败返回-1 */
int DONE_token_parse(unsigned char *token, int *tlen, DONE *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (DONE_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Status);
    STEP(pos, *tlen-(pos-token), 2, USHORT, res->CurCmd);

    if (TDS_VERSION >= TDS_V_7_2) {
        STEP(pos, *tlen-(pos-token), 8, ULONGLONG, res->DoneRowCount);
    } else {
        STEP(pos, *tlen-(pos-token), 4, LONG, res->DoneRowCount);
    }

    *tlen = pos - token;
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.6节
   Indicates the completion status of a SQL statement within a stored procedure. */
typedef struct {
    BYTE TokenType; /* =DONEINPROC_TOKEN */
    USHORT Status;   /* 和DONE.Status的取值范围一样 */
    USHORT CurCmd;   /* The token of the current SQL statement */
    /* The count of rows that were affected by the SQL statement.  */
    ULONGLONG DoneRowCount; /* DoneRowCount = LONG / ULONGLONG; (Changed to ULONGLONG in TDS 7.2) */
} DONEINPROC;

/* 'token'指向token块开头。
   'tlen'为该块数据最大长度，并用它返回该token的大小
   'res'保存感兴趣的解析参数
   成功返回0，失败返回-1 */
int DONEINPROC_token_parse(unsigned char *token, int *tlen, DONEINPROC *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (DONEINPROC_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Status);
    STEP(pos, *tlen-(pos-token), 2, USHORT, res->CurCmd);
    if (TDS_VERSION >= TDS_V_7_2) {
        STEP(pos, *tlen-(pos-token), 8, ULONGLONG, res->DoneRowCount);
    } else {
        STEP(pos, *tlen-(pos-token), 4, LONG, res->DoneRowCount);
    }

    *tlen = pos - token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.7节
   Indicates the completion status of a SQL statement within a stored procedure. */
typedef struct {
    BYTE TokenType; /* =DONEPROC_TOKEN */
    USHORT Status; /* 和DONE.Status的取值范围一样 */
    USHORT CurCmd; /* The token of the current SQL statement */
    /* The count of rows that were affected by the SQL statement.  */
    ULONGLONG DoneRowCount; /* DoneRowCount = LONG / ULONGLONG; (Changed to ULONGLONG in TDS 7.2) */
} DONEPROC;

/* 'token'指向token块开头。
   'tlen'为该块数据最大长度，并用它返回该token的大小
   'res'保存感兴趣的解析参数
   成功返回0，失败返回-1 */
int DONEPROC_token_parse(unsigned char *token, int *tlen, DONEPROC *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (DONEPROC_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Status);
    STEP(pos, *tlen-(pos-token), 2, USHORT, res->CurCmd);

    if (TDS_VERSION >= TDS_V_7_2) {
        STEP(pos, *tlen-(pos-token), 8, ULONGLONG, res->DoneRowCount);
    } else {
        STEP(pos, *tlen-(pos-token), 4, LONG, res->DoneRowCount);
    }

    *tlen = pos - token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.8节，ENVCHANGE_TOKEN */
/* <1>A notification of an environment change (for example, database, language, and so on).
   <2>Includes old and new environment values.
   <3>Type 4 (Packet size) is sent in response to a LOGIN7 message. The server MAY send a value
different from the packet size requested by the client. That value MUST be greater than or equal
to 512 and smaller than or equal to 32767. Both the client and the server MUST start using this
value for packet size with the message following the login response message.
   <4>Type 13 (Database Mirroring) is sent in response to a LOGIN7 message whenever connection is
requested to a database that it is being served as primary in real-time log shipping. The
ENVCHANGE stream reflects the name of the partner node of the database that is being log
shipped.
   <5>Type 15 (Promote Transaction) is sent in response to transaction manager requests with requests
of type 6 (TM_PROMOTE_XACT).
   <6>Type 16 (Transaction Manager Address) is sent in response to transaction manager requests with
requests of type 0 (TM_GET_DTC_ADDRESS).
   <7>Type 20 (Routing) is sent in response to a LOGIN7 message when the server wants to route the
client to an alternate server. The ENVCHANGE stream returns routing information for the alternate
server. If the server decides to send the Routing ENVCHANGE token, the Routing ENVCHANGE
token MUST be sent after the LOGINACK token in the login response.*/

/* 'ENVCHANGE_EnvValueData.Type'的取值范围 */
#define Database 1
#define Language 2
#define Character_set 3
#define Packet_size 4
#define Unicode_data_sorting_local_id 5
#define Unicode_data_sorting_comparison_flags 6
#define SQL_Collation 7

#define Begin_Transaction 8
#define Commit_Transaction 9
#define Rollback_Transaction 10
#define Enlist_DTC_Transaction 11
#define Defect_Transaction 12
#define Real_Time_Log_Shipping 13
#define Promote_Transaction 15 /* 注意，没14 */
#define Transaction_Manager_Address 16
#define Transaction_ended 17
#define RESETCONNECTION_RESETCONNECTIONSKIPTRAN_Completion_Acknowledgement 18
#define Sends_back_name_of_user_instance_started_per_login_request 19

#define Sends_routing_information_to_client 20

/* 根据'Type'的不同，'NewValue'和'OldValue'分别指向不同的数据结构(暂时可以忽略) */
typedef struct {
    BYTE Type; /* The type of environment change */
    BYTE *NewValue;
    BYTE *OldValue;
}ENVCHANGE_EnvValueData;

typedef struct {
    BYTE TokenType; /* =ENVCHANGE_TOKEN */
    USHORT Length; /* The total length of the ENVCHANGE data stream (EnvValueData). */
    ENVCHANGE_EnvValueData EnvValueData;
}ENVCHANGE;

int ENVCHANGE_token_parse(unsigned char *token, int *tlen, ENVCHANGE *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (ENVCHANGE_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);
    STEP_N(pos, *tlen-(pos-token), res->Length);

    *tlen = pos - token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.9节，ERROR_TOKEN */
/* Used to send an error message to the client. */
typedef struct {
    BYTE TokenType; /* =ERROR_TOKEN */
    USHORT Length; /* The total length of the ERROR data stream, in bytes 不包括TokenType和Length*/
    LONG Number; /* The error number */
    BYTE State; /* The error state, used as a modifier to the error number */
    BYTE Class; /* The class (severity) of the error */
    US_VARCHAR MsgText; /* The message text length and message text */
    B_VARCHAR ServerName; /* The server name length and server name */
    B_VARCHAR ProcName; /* The stored procedure name length and the stored procedure name  */
    LONG LineNumber; /* LineNumber = USHORT / LONG; (Changed to LONG in TDS 7.2). The line number in the SQL batch or stored procedure that caused the error. TDS 7.2之前是unsigned short */
} ERROR;

int ERROR_token_parse(unsigned char *token, int *tlen, ERROR *res) {
    unsigned char *pos = token;
    int leftlen, retval;

    res->TokenType = *pos++;
    if (ERROR_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);
    STEP(pos, *tlen-(pos-token), 4, LONG, res->Number);
    STEP(pos, *tlen-(pos-token), 1, BYTE, res->State);
    STEP(pos, *tlen-(pos-token), 1, BYTE, res->Class);

    leftlen = *tlen - (pos - token);
    retval = US_VARCHAR_parse(pos, &leftlen, &(res->MsgText));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    leftlen = *tlen - (pos - token);
    retval = B_VARCHAR_parse(pos, &leftlen, &(res->ServerName));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    leftlen = *tlen - (pos - token);
    retval = B_VARCHAR_parse(pos, &leftlen, &(res->ProcName));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);
    if (TDS_VERSION >= TDS_V_7_2){
        STEP(pos, *tlen-(pos-token), 4, LONG, res->LineNumber);
    } else {
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->LineNumber);
    }

    *tlen = res->Length+3;
    return 0;
}

int getDescriptionInERROR(ERROR *res, char *resultStr, int maxsize) {
    int retval;
    char MsgText[1024];

    retval = codeConv("UCS-2", TDS_CHARSET, res->MsgText.data, res->MsgText.wlen, MsgText, sizeof(MsgText));
    if (retval < 0) return -1;
    snprintf(resultStr, maxsize, "info:%d, level:%d, state:%d, line number:%d,%s",
             res->Number, res->Class, res->State, res->LineNumber, MsgText);
    return 0;
}


/*===============================================================================*/
/* 2.2.7.10节，FEATUREEXTACK_TOKEN */
/* Used to send an optional acknowledge message to the client for features defined in FeatureExt. The
token stream is sent only along with the LOGINACK in a Login Response message. */

/* 在2.2.5.8节的FEATUREEXTACK_TOKEN定义处表明的，但该2.2.7.10节没有说 */
typedef struct {
    BYTE FeatureId; /* The unique identifier number of a feature. 是0xff,则没有后面的域了 */
    DWORD FeatureAckDataLen; /* The length of FeatureAckData, in bytes. */
    BYTE *FeatureAckData; /* Ack data of specific feature. */
} FEATUREEXTACK_FeatureAckOpt;

typedef struct {
    BYTE TokenType; /* =FEATUREEXTACK_TOKEN */
    FEATUREEXTACK_FeatureAckOpt FeatureAckOpt; /* 多个'FeatureAckOpt'。以0xFF结尾，此处只做缓存 */
} FEATUREEXTACK;

int FEATUREEXTACK_token_parse(unsigned char *token, int *tlen, FEATUREEXTACK *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (FEATUREEXTACK_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    while (pos < token+*tlen) {
        STEP(pos, *tlen-(pos-token), 1, BYTE, res->FeatureAckOpt.FeatureId);
        if (0xFF == res->FeatureAckOpt.FeatureId) {
            break;
        }
        STEP(pos, *tlen-(pos-token), 4, DWORD, res->FeatureAckOpt.FeatureAckDataLen);
        STEP_PL(pos, *tlen-(pos-token), res->FeatureAckOpt.FeatureAckData, res->FeatureAckOpt.FeatureAckDataLen);
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.11节，FEDAUTHINFO_TOKEN */
/* The federated authentication<39> information returned to the client to be used
   for generating a Federated Authentication Token during the login process. This
   token MUST be the only token in a Federated Authentication Information message
   and MUST NOT be included in any other message type. */
/* 'FedAuthInfo'的取值范围 */
#define Reserved 0x00 /* Reserved. */
#define STSURL 0x01 /* A Unicode string that represents the token endpoint URL from which to acquire a Federated Authentication Token. */
#define SPN 0x02 /* A Unicode string that represents the Service Principal Name (SPN) to use for acquiring a Federated Authentication Token. SPN is a string that represents the resource in a directory. */

typedef struct {
    BYTE FedAuthInfoID; /* The unique identifier number for the type of information. */
    DWORD FedAuthInfoDataLen; /* The length of FedAuthInfoData, in bytes. */
    DWORD FedAuthInfoDataOffset; /* The offset at which the federated authentication information data for FedAuthInfoID is present, measured from the address of CountOfInfoIDs. */
}FEDAUTHINFO_FedAuthInfoOpt;

typedef struct {
    BYTE TokenType; /* =FEDAUTHINFO_TOKEN */
    DWORD TokenLength; /* The length of the whole Federated Authentication Information token, not including the size occupied by TokenLength itself. */
    DWORD CountOfInfoIDs; /* The number of federated authentication information options that are sent in the token.  */
    FEDAUTHINFO_FedAuthInfoOpt FedAuthInfoOpt;
    BYTE *FedAuthInfoData; /* The actual information data as binary, with the length in bytes equal to FedAuthInfoDataLen.  */
} FEDAUTHINFO;

int FEDAUTHINFO_token_parse(unsigned char *token, int *tlen, FEDAUTHINFO *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (FEDAUTHINFO_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 4, DWORD, res->TokenLength);
    STEP_N(pos, *tlen-(pos-token), res->TokenLength);

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.12节，INFO_TOKEN */
/* Used to send an information message to the client. */
typedef struct {
    BYTE TokenType; /* =INFO_TOKEN */
    USHORT Length; /* The total length of the INFO data stream, in bytes. */
    LONG Number; /* The info number */
    BYTE State; /* The error state, used as a modifier to the info Number. */
    BYTE Class; /* The class (severity) of the error. A class of less than 10 indicates an informational message. */
    US_VARCHAR MsgText;
    B_VARCHAR ServerName;
    B_VARCHAR ProcName;
    ULONG LineNumber; /* LineNumber = USHORT/ULONG; (Changed to ULONG in TDS 7.2). The line number in the SQL batch or stored procedure that caused the error.  */
} INFO;

int INFO_token_parse(unsigned char *token, int *tlen, INFO *res) {
    unsigned char *pos = token;
    int leftlen, retval;

    res->TokenType = *pos++;
    if (INFO_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);
    STEP(pos, *tlen-(pos-token), 4, LONG, res->Number);
    STEP(pos, *tlen-(pos-token), 1, BYTE, res->State);
    STEP(pos, *tlen-(pos-token), 1, BYTE, res->Class);

    leftlen = *tlen - (pos - token);
    retval = US_VARCHAR_parse(pos, &leftlen, &(res->MsgText));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    leftlen = *tlen - (pos - token);
    retval = B_VARCHAR_parse(pos, &leftlen, &(res->ServerName));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    leftlen = *tlen - (pos - token);
    retval = B_VARCHAR_parse(pos, &leftlen, &(res->ProcName));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    if(TDS_VERSION >= TDS_V_7_2) {
        STEP(pos, *tlen-(pos-token), 4, ULONG, res->LineNumber);
    } else {
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->LineNumber);
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.13节，LOGINACK_TOKEN */
/* <1>Used to send a response to a login request (LOGIN7) to the client
   <2>If a LOGINACK is not received by the client as part of the login procedure, the login to the server
is unsuccessful.*/

/* 'struct __LOGINACK.Interface'的取值范围 */
#define SQL_DFLT 0
#define SQL_TSQL 1

typedef struct {
    BYTE MajorVer; /* The major version number (0-255) */
    BYTE MinorVer; /* The minor version number (0-255) */
    BYTE BuildNumHi; /* The high byte of the build number (0-255). */
    BYTE BuildNumLow; /* The low byte of the build number (0-255) */
}LOGINACK_ProgVersion;

typedef struct {
    BYTE TokenType; /* LOGINACK_TOKEN */
    USHORT Length; /* The total length, in bytes, of the following fields: Interface, TDSVersion, Progname, and ProgVersion. */
    BYTE Interface; /* The type of interface with which the server will accept client requests */
    DWORD TDSVersion; /* The TDS version being used by the server  */
    B_VARCHAR ProgName; /* The name of the server */
    LOGINACK_ProgVersion ProgVersion;
}LOGINACK;

int LOGINACK_token_parse(unsigned char *token, int *tlen, LOGINACK *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (LOGINACK_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);
    STEP_N(pos, *tlen-(pos-token), res->Length);

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.14节，NBCROW_TOKEN */
/* NBCROW, introduced in TDS 7.3.B, is used to send a row as defined by the COLMETADATA token to
the client with null bitmap compression. Null bitmap compression is implemented by using a single bit
to specify whether the column is null or not null and also by removing all null column values from the
row. Removing the null column values (which can be up to 8 bytes per null instance) from the row
provides the compression. The null bitmap contains one bit for each column defined in COLMETADATA.
In the null bitmap, a bit value of 1 means that the column is null and therefore not present in the row,
and a bit value of 0 means that the column is not null and is present in the row. The null bitmap is
always rounded up to the nearest multiple of 8 bits, so there might be 1 to 7 leftover reserved bits at
the end of the null bitmap in the last byte of the null bitmap. NBCROW is only used by TDS result set
streams from server to client. NBCROW MUST NOT be used in BulkLoadBCP streams. NBCROW MUST
NOT be used in TVP row streams.  */

typedef struct {
    /* 注意 - 'TextPointer'和'Timestamp'只有当text/ntext/image时才有。 */
    B_VARBYTE TextPointer; /* 可选，text pointer for Data */
    BYTE Timestamp[8]; /* 可选，The timestamp of a text/image column. */
    TYPE_VARBYTE Data; /* The actual data for the column. The TYPE_INFO information describing the data type of this data is given in the preceding COLMETADATA_TOKEN */
}NBCROW_ColumnData;

typedef struct {
    BYTE TokenType; /* =NBCROW_TOKEN */
    BYTE NullBitmap[256]; /* 多个字节，能包含上一个名为COLMETADATA的token的列数'Count'个位的字节数。即Count/8向上取整 */
    NBCROW_ColumnData AllColumnData[1024]; /* 多个'struct __ColumnData'，此处仅做缓存用 */
}NBCROW;
static NBCROW NBCROWres;

#define isNULL(map, i) (map[i>>3] & (1<<(i & 0x7)))

int NBCROW_token_parse(unsigned char *token, int *tlen, NBCROW *res, COLMETADATA *COLMETADATAres) {
    unsigned char *pos = token;
    int NullBitmapByteCount, leftlen, retval, i;

    res->TokenType = *pos++;
    if (NBCROW_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    NullBitmapByteCount = (COLMETADATAres->Count%8==0) ? COLMETADATAres->Count/8 : COLMETADATAres->Count/8+1;
    memcpy(NBCROWres.NullBitmap, pos, NullBitmapByteCount);
    STEP_N(pos, *tlen-(pos-token), NullBitmapByteCount); /* 跳过'NBCROW.NullBitmap' */

    for (i=0; i<COLMETADATAres->Count; i++) {
        if (isNULL(NBCROWres.NullBitmap, i)) {
            /* 必须保证NBCROWres在之前是置0的 */
            continue;
        }
        if (TEXTTYPE == COLMETADATAres->ColumnData[i].TYPE_INFO.LENTYPE
            || NTEXTTYPE == COLMETADATAres->ColumnData[i].TYPE_INFO.LENTYPE
            || IMAGETYPE == COLMETADATAres->ColumnData[i].TYPE_INFO.LENTYPE) {

            leftlen = *tlen - (pos - token);
            retval = B_VARBYTE_parse(pos, &leftlen, &(res->AllColumnData[i].TextPointer));
            if (retval < 0) return -1;
            STEP_N(pos, *tlen-(pos-token), leftlen);

            if ((*tlen - (pos - token)) >= 8) {
                memcpy(res->AllColumnData[i].Timestamp, pos, 8);
            } else {
                TDS_PRINT_ERR_MSG("short of length"); return -1;
            }
        }
        leftlen = *tlen - (pos - token);
        retval = TYPE_VARBYTE_parse(pos, &leftlen, &(COLMETADATAres->ColumnData[i].TYPE_INFO), &(res->AllColumnData[i].Data));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.15节，OFFSET_TOKEN */
/* Used to inform the client where in the client's SQL text buffer a particular keyword occurs. */
typedef struct{
    BYTE TokenType;   /* OFFSET_TOKEN */
    USHORT Identifier; /* The keyword to which OffSetLen refers. */
    USHORT OffSetLen;  /* The offset in the SQL text buffer received by the server of the identifier. */
} OFFSET;

int OFFSET_token_parse(unsigned char *token, int *tlen, OFFSET *res) {
    unsigned char *pos = token;

    res->TokenType = *pos;
    if (OFFSET_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    if (*tlen >= sizeof(OFFSET)) {
        *res = *(OFFSET *)pos;
        STEP_N(pos, *tlen-(pos-token), sizeof(OFFSET));
    } else {
        TDS_PRINT_ERR_MSG("short of length"); return -1;
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.16节，ORDER_TOKEN */
/* <1>Used to inform the client by which columns the data is ordered.
   <2>This token is sent only in the event that an ORDER BY clause is executed*/

typedef struct {
    BYTE TokenType; /* ORDER_TOKEN */
    USHORT Length; /* The total length of the ORDER data stream. */
    USHORT ColNum; /* The column number in the result set. 多个ColNum，此处只做缓存用The ColNum element is repeated once for each column within the ORDER BY clause. */
}ORDER;

int ORDER_token_parse(unsigned char *token, int *tlen, ORDER *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (ORDER_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);
    STEP_N(pos, *tlen-(pos-token), res->Length);

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.17节，RETURNSTATUS_TOKEN */
/* <1>Used to send the status value of an RPC to the client. The server also uses this
   token to send the result status value of a T-SQL EXEC query.
   <2>This token MUST be returned to the client when an RPC is executed by the server.*/

typedef struct {
    BYTE TokenType; /* RETURNSTATUS_TOKEN */
    LONG Value;     /* The return status value determined by the remote procedure. Return status MUST NOT be NULL. */
} RETURNSTATUS;

int RETURNSTATUS_token_parse(unsigned char *token, int *tlen, RETURNSTATUS *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (RETURNSTATUS_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 4, LONG, res->Value);

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.7.18节，RETURNVALUE_TOKEN */
/* <1>Used to send the return value of an RPC to the client. When an RPC is executed,
   the associated parameters might be defined as input or output (or "return")
   parameters. This token is used to send a description of the return parameter
   to the client. This token is also used to describe the value returned by a UDF
   when executed as an RPC.
   <2> Multiple return values can exist per RPC. There is a separate RETURNVALUE token sent for each
parameter returned.
   <3> Large Object output parameters are reordered to appear at the end of the stream. First the group
of small parameters is sent, followed by the group of large output parameters. There is no
reordering within the groups.
   <4> A UDF cannot have return parameters. As such, if a UDF is executed as an RPC there is exactly
one RETURNVALUE token sent to the client.*/

/* 'RETURNVALUE.Status'的取值范围 */
#define STORED_PROCEDURE_INVOCATION 0x01 /* If ReturnValue corresponds to OUTPUT parameter of a stored procedure invocation. */
#define USER_DEFINED_FUNTION 0x02 /* If ReturnValue corresponds to return value of User Defined Function. */

typedef struct {
    ULONG UserType; /* The user-defined data type of the column. */
    struct __TYPE_INFO BaseTypeInfo; /* TYPE_INFO for the unencrypted type. */
    BYTE EncryptionAlgo; /* A byte that describes the encryption algorithm that is used. */
    B_VARCHAR AlgoName; /* Algorithm name literal that is used to encrypt the plaintext value. */
    BYTE EncryptionAlgoType; /* A field that describes the encryption algorithm type. */
    BYTE NormVersion; /* The normalization version to which plaintext data MUST be normalized. */
} RETURNVALUE_CryptoMetadata;

typedef struct {
    BYTE TokenType; /* RETURNVALUE_TOKEN */
    USHORT ParamOrdinal; /* Indicates the ordinal position of the output parameter in the original RPC call. */
    B_VARCHAR ParamName; /*  */
    BYTE Status; /*  */
    ULONG UserType; /* UserType = USHORT/ULONG; (Changed to ULONG in TDS 7.2). The user-defined data type of the column. */
    USHORT Flags; /* All of these bit flags SHOULD be set to zero */
    struct __TYPE_INFO TypeInfo; /* The TYPE_INFO for the message */
    RETURNVALUE_CryptoMetadata CryptoMetadata; /* introduced in TDS 7.4, This describes the encryption metadata for a column. */
    TYPE_VARBYTE Value; /* The type-dependent data for the parameter (within TYPE_VARBYTE). */
} RETURNVALUE;

int RETURNVALUE_token_parse(unsigned char *token, int *tlen, RETURNVALUE *res, int isRPC) {
    unsigned char *pos = token;
    int leftlen, retval;

    res->TokenType = *pos++;
    if (RETURNVALUE_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->ParamOrdinal);

    leftlen = *tlen - (pos-token);
    retval = B_VARCHAR_parse(pos, &leftlen, &(res->ParamName));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    STEP(pos, *tlen-(pos-token), 1, BYTE, res->Status);

    if (TDS_VERSION >= TDS_V_7_2 ) {
        STEP(pos, *tlen-(pos-token), 4, ULONG, res->UserType);
    } else {
        STEP(pos, *tlen-(pos-token), 2, USHORT, res->UserType);
    }

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Flags);

    leftlen = *tlen - (pos-token);
    retval = TYPE_INFO_parse(pos, &leftlen, &(res->TypeInfo), isRPC);
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    if (TDS_VERSION >= TDS_V_7_4 ) {
        STEP(pos, *tlen-(pos-token), 4, ULONG, res->CryptoMetadata.UserType);

        leftlen = *tlen - (pos-token);
        retval = TYPE_INFO_parse(pos, &leftlen, &(res->CryptoMetadata.BaseTypeInfo), isRPC);
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);

        STEP(pos, *tlen-(pos-token), 1, BYTE, res->CryptoMetadata.EncryptionAlgo);

        leftlen = *tlen - (pos-token);
        retval = B_VARCHAR_parse(pos, &leftlen, &(res->CryptoMetadata.AlgoName));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);

        STEP(pos, *tlen-(pos-token), 1, BYTE, res->CryptoMetadata.EncryptionAlgoType);
        STEP(pos, *tlen-(pos-token), 1, BYTE, res->CryptoMetadata.NormVersion);
    }

    leftlen = *tlen - (pos-token);
    retval = TYPE_VARBYTE_parse(pos, &leftlen, &(res->TypeInfo), &(res->Value));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.19节，ROW_TOKEN */
typedef struct {
    /* 前2个只在text/image类型的列出现 */
    B_VARBYTE TextPointer; /* The length of the text pointer and the text pointer for data */
    BYTE Timestamp[8];     /* The timestamp of a text/image column. */

    TYPE_VARBYTE Data;     /* The actual data for the column. 注意:具体类型根据上一个token的TYPE_INFO的值确定 */
} ROW_ColumnData;

/* <1>The ColumnData element is repeated once for each column of data.
   <2>TextPointer and Timestamp MUST NOT be specified if the instance of type text/ntext/image is a NULL instance (GEN_NULL). */
typedef struct {
    BYTE TokenType; /* ROW_TOKEN */
    ROW_ColumnData AllColumnData[1024];
} ROW;
static ROW ROWres;

/* 'token'指向token块开头。
   'tlen'为该块数据最大长度，并用它返回该token的大小
   'res'保存感兴趣的解析参数
   'TYPE_INFO'是当前token的数据类型，ROW上一个COLMETADATA_TOKEN, ALTMETDATA_TOKEN 或 OFFSET_TOKEN的token中给出的
   'Count'是当前token的列数，ROW上个COLMETADATA_TOKEN, ALTMETDATA_TOKEN 或 OFFSET_TOKEN给出的
   成功返回0，失败返回-1 */
int ROW_token_parse(unsigned char *token, int *tlen, ROW *res, COLMETADATA *colmetadata) {
    unsigned char *pos = token;
    int leftlen, retval, i, max;

    res->TokenType = *pos++;
    if (ROW_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    max = (colmetadata->Count > 1024) ? 1024 : colmetadata->Count;
    for (i=0; i<max; i++) {
        if (TEXTTYPE==colmetadata->ColumnData[i].TYPE_INFO.LENTYPE
            || NTEXTTYPE==colmetadata->ColumnData[i].TYPE_INFO.LENTYPE
            || IMAGETYPE==colmetadata->ColumnData[i].TYPE_INFO.LENTYPE) {
            leftlen = *tlen - (pos - token);
            retval = B_VARBYTE_parse(pos, &leftlen, &(res->AllColumnData[i].TextPointer));
            if (retval < 0) return -1;
            STEP_N(pos, *tlen-(pos-token), leftlen);/* 跳过TextPointer */
            STEP_N(pos, *tlen-(pos-token), 8); /* 跳过Timestamp */
        }

        leftlen = *tlen - (pos - token);
        retval = TYPE_VARBYTE_parse(pos, &leftlen, &(colmetadata->ColumnData[i].TYPE_INFO), &(res->AllColumnData[i].Data));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.20节，SESSIONSTATE_TOKEN */
/* Used to send session state data to the client. The data format defined here
   can also be used to send session state data for session recovery during login
   and login response. */

/* TDS 7.4在本节中没有说版本问题，但SESSIONSTATE_TOKEN定义2.2.5.8节处标明了 */
typedef struct {
    BYTE StateId; /* The identification number of the session state. 0xFF is reserved. */
    /* The length, in bytes, of the corresponding StateValue.
       当'StateLen'在0x00-0xfe之间，没'StateLen2'.若'StateLen'=0xff,有'StateLen2' */
    BYTE StateLen;
    DWORD StateLen2;
    BYTE *StateValue; /* The value of the session state */
} SESSIONSTATE_SessionStateData;

typedef struct  {
    BYTE TokenType; /* =SESSIONSTATE_TOKEN */
    DWORD Length;   /* The length, in bytes, of the token stream (excluding TokenType and Length). */
    DWORD SeqNo;    /* The sequence number of the SESSIONSTATE token in the connection. */
    BYTE Status;    /* Status of the session StateId in this token. */
    SESSIONSTATE_SessionStateData SessionStateDataSet; /* 是多个struct __SessionStateData，此处做缓存用 */
}SESSIONSTATE;

int SESSIONSTATE_token_parse(unsigned char *token, int *tlen, SESSIONSTATE *res) {
    unsigned char *pos = token;

    res->TokenType = *pos++;
    if (SESSIONSTATE_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 4, DWORD, res->Length);
    STEP_N(pos, *tlen-(pos-token), res->Length);

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.21节，SSPI_TOKEN */
typedef struct {
    BYTE TokenType; /* SSPI_TOKEN */
    US_VARBYTE SSPIBuffer;
} SSPI;

int SSPI_token_parse(unsigned char *token, int *tlen, SSPI *res) {
    unsigned char *pos = token;
    int leftlen, retval;

    res->TokenType = *pos++;
    if (SSPI_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    leftlen = *tlen - (pos - token);
    retval = US_VARBYTE_parse(pos, &leftlen, &(res->SSPIBuffer));
    if (retval < 0) return -1;
    STEP_N(pos, *tlen-(pos-token), leftlen);

    *tlen = pos-token;
    return 0;
}


/*===============================================================================*/
/* 来自2.2.7.22节，TABNAME_TOKEN */
/* Used to send the table name to the client only when in browser mode or from sp_cursoropen. */

/* TableName = US_VARCHAR ; (removed in TDS 7.1 Revision 1)
               /
               (NumParts
               1*PartName) ; (introduced in TDS 7.1 Revision 1) */
typedef struct {
    BYTE NumParts; /* introduced in TDS 7.1 Revision 1 */
    US_VARCHAR PartName; /* introduced in TDS 7.1 Revision 1. 多个PartName，此处仅做缓存 */

    US_VARCHAR tablename; /* removed in TDS 7.1 Revision 1 */
}TABNAME_TableName;

typedef struct {
    BYTE TokenType; /* TABNAME_TOKEN */
    USHORT Length; /* The actual data length, in bytes, of the TABNAME token stream. The length does not include token type and length field. */
    TABNAME_TableName AllTableNames; /* The name of the base table referenced in the query statement. */
} TABNAME;

int TABNAME_token_parse(unsigned char *token, int *tlen, TABNAME *res) {
    unsigned char *pos = token;
    int leftlen, retval;

    res->TokenType = *pos++;
    if (TABNAME_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    STEP(pos, *tlen-(pos-token), 2, USHORT, res->Length);

    if (TDS_VERSION >= TDS_V_7_1) {
        STEP(pos, *tlen-(pos-token), 1, BYTE, res->AllTableNames.NumParts);

        leftlen = *tlen - (pos - token);
        retval = US_VARCHAR_parse(pos, &leftlen, &(res->AllTableNames.PartName));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    } else {
        leftlen = *tlen - (pos - token);
        retval = US_VARCHAR_parse(pos, &leftlen, &(res->AllTableNames.tablename));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 来自2.2.7.23节，TVP_ROW_TOKEN */
/* Used to send a complete table valued parameter (TVP) row, as defined by the
   TVP_COLMETADATA token from client to server. */

typedef struct {
    BYTE TokenType; /* =TVP_ROW_TOKEN */
    TYPE_VARBYTE AllColumnData; /* 多个TYPE_VARBYTE，仅做缓存用. The actual data for the TVP column. */
} TVP_ROW;

int TVP_ROW_token_parse(unsigned char *token, int *tlen, TVP_ROW *res, COLMETADATA *colmetadata) {
    unsigned char *pos = token;
    int leftlen, retval, i;

    res->TokenType = *pos++;
    if (TVP_ROW_TOKEN != res->TokenType) {TDS_PRINT_ERR_MSG("incorrect funtion"); return -1;}

    for (i=0; i<colmetadata->Count; i++) {
        leftlen = *tlen - (pos - token);
        retval = TYPE_VARBYTE_parse(pos, &leftlen, &(colmetadata->ColumnData[i].TYPE_INFO), &(res->AllColumnData));
        if (retval < 0) return -1;
        STEP_N(pos, *tlen-(pos-token), leftlen);
    }

    *tlen = pos-token;
    return 0;
}

/*===============================================================================*/
/* 2.2.6.1节 */
/* <1>Describes the format of bulk-loaded data through the "INSERT BULK" T-SQL statement. The format
   is a COLMETADATA token describing the data being sent, followed by multiple ROW tokens, ending
   with a DONE token. The stream is equivalent to that produced by the server if it were sending the
   same rowset on output.
   <2>Packet header type is 0x07
   <3>This message sent to the server contains bulk data to be inserted. The client MUST have
   previously notified the server where this data is to be inserted. */
typedef struct {
    COLMETADATA BulkLoad_METADATA;
    ROW BulkLoad_ROW; /* 有多个'ROW'，此处做缓存 */
    DONE BulkLoad_DONE;
} BulkLoadBCP;

int tdsRecombination(unsigned char *data, int len, unsigned char *recomData, int maxsize) {
    int leftLen, curPktLen;
    unsigned char *curPktStart;
    TDS_HEADERS tdsh;
    unsigned char *ptrb;

    curPktStart = data;
    leftLen = len;
    ptrb = recomData;
    while (curPktStart < data+len && ptrb < (recomData+maxsize)) {
        if (leftLen <= 8) {
            break;
        }

        tdsh = *(TDS_HEADERS *)curPktStart;
        big_2_host(&(tdsh.Length), 2);

        curPktLen = (tdsh.Length > leftLen) ? leftLen : tdsh.Length;
        if (curPktLen > (maxsize-(ptrb-recomData))) {
            curPktLen =  maxsize-(ptrb-recomData);
        }

        if (curPktStart == data) {
            memcpy(ptrb, curPktStart, curPktLen);
            ptrb += curPktLen;
        } else {
            memcpy(ptrb, curPktStart+8, curPktLen-8);
            ptrb += (curPktLen-8);
        }
        if (maxsize-(ptrb-recomData) <= 1) {
            break;
        }

        curPktStart += curPktLen;
        leftLen -= curPktLen;
    }
    return (ptrb-recomData);
}
int getDescriptionInRow(ROW *ROWres, COLMETADATA *COLMETADATAres, char *resultStr, int maxsize);

/* 2.2.6.1节 + 2.2.6.2节
   Describes the format of bulk-loaded data through the "INSERT BULK" T-SQL statement. The format
   is a COLMETADATA token describing the data being sent, followed by multiple ROW tokens, ending
   with a DONE token. The stream is equivalent to that produced by the server if it were sending the
   same rowset on output.*/
/* 'data'是TDS包的8字节包头后的数据部分, 'len'是该部分的字节长度
   成功返回0，失败返回-1
   参看2.2.6.1节 */
int BulkLoadBCP_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    int retval, leftlen, curPktLen = 0;
    unsigned char *pos=NULL, *curPktStart= NULL;
    TDS_HEADERS tdsh;

    if (len <= 8) {
        return 0;
    }

    memset(resultStr, 0, maxsize);

    tdsh = *(TDS_HEADERS *)data;
    big_2_host(&(tdsh.Length), 2);

    if (tdsh.Length < len) {
        curPktLen = tdsRecombination(data, len, tabularResultBuf, sizeof(tabularResultBuf));
        fprintf(stderr, "tdsRecombination len = %d\n", curPktLen);
        curPktStart = tabularResultBuf;
    } else {
        curPktStart = data;
        curPktLen = len;
    }

    pos = curPktStart + 8;
    leftlen  = curPktLen - 8;

    while (pos < curPktStart+curPktLen) {
#if TDS_DEBUG_OPEN
        fprintf(stderr, "DEBUG : <%s> : %d : Type=0x%x\n", __FILE__, __LINE__, *pos);
#endif
        if (COLMETADATA_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            memset(&COLMETADATAres, 0, sizeof(COLMETADATA));

            retval = COLMETADATA_token_parse(pos, &leftlen, &COLMETADATAres, 0);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
            getDescriptionInCOLMETADATA(&COLMETADATAres, resultStr, maxsize);
        } else if (ROW_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            memset(&ROWres, 0, sizeof(ROW));

            retval = ROW_token_parse(pos, &leftlen, &ROWres, &COLMETADATAres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
            getDescriptionInRow(&ROWres, &COLMETADATAres, resultStr+strlen(resultStr), maxsize-strlen(resultStr));

            if (maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:n");
        } else if (DONE_TOKEN == *pos) {
            DONE DONEres;
            leftlen = len - (pos - curPktStart);
            memset(&DONEres, 0, sizeof(DONE));

            retval = DONE_token_parse(pos, &leftlen, &DONEres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else {
            break;
        }

    }
    //fprintf(stderr, "%s\n", resultStr);
    return 0;
}

/*==================================================================================*/
/* 来自2.2.6.3节 */
typedef struct {
    DWORD DataLen; /* The total length of the data in the Federated Authentication Token message that follows this field. */
    L_VARBYTE FedAuthToken;    /*  the federated authentication token data */
    BYTE Nonce[32];
} FEDAUTH;

int FEDAUTH_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    FEDAUTH FEDAUTH_info;
    unsigned char *pos = data;
    int leftlen, retval;

    STEP(pos, len-(pos-data), 4, DWORD, FEDAUTH_info.DataLen);

    leftlen = len-(pos-data);
    retval = L_VARBYTE_parse(pos, &leftlen, &(FEDAUTH_info.FedAuthToken));
    if (retval < 0) return -1;
    STEP_N(pos, len-(pos-data), leftlen);

    leftlen = len-(pos-data);
    if (leftlen >= 32) {
        memcpy(FEDAUTH_info.Nonce, pos, 32);
    }
    if (leftlen != 32) {
        TDS_PRINT_ERR_MSG("encounter unexcepted bytes"); return -1;
    }
    return 0;
}

/*==================================================================================*/
/* 来自2.2.6.4节
   the authentication rules for use between client and server
   ib..是offset，单位是1字节，相对于LOGIN7包开头，即struct __LOGIN7.Data部分的前面的
   字节数，不包括TDS头。
   cch...是Length，单位是2字节
   上面是4.2节的分析结果 */
typedef struct {
    USHORT ibHostName;
    USHORT cchHostName;/* IbHostname & cchHostName: The client machine name */
    USHORT ibUserName;
    USHORT cchUserName; /* IbUserName & cchUserName: The client user ID */
    USHORT ibPassword;
    USHORT cchPassword; /* IbPassword & cchPassword: The password supplied by the client. */
    USHORT ibAppName;
    USHORT cchAppName; /* IbAppName & cchAppName: The client application name. */
    USHORT ibServerName;
    USHORT cchServerName; /* IbServerName & cchServerName: The server name. */

    /* ibExtension & cbExtension 和 ibUnused & cbUnused是互斥的 */
    USHORT ibExtension; /* (introduced in TDS 7.4) */
    USHORT cbExtension; /* (introduced in TDS 7.4). ibExtension & cbExtension: This points to an extension block. */

    //USHORT ibUnused;
    //USHORT cchUnused; /* ibUnused & cbUnused: These parameters were reserved until TDS 7.4. */

    USHORT ibCltIntName;
    USHORT cchCltIntName; /* ibCltIntName & cchCltIntName: The interface library name (ODBC or OLEDB) */
    USHORT ibLanguage;
    USHORT cchLanguage; /* ibLanguage & cchLanguage: The initial language (overrides the user ID's default language). */
    USHORT ibDatabase;
    USHORT cchDatabase; /* ibDatabase & cchDatabase: The initial database (overrides the user ID's default database). */
    BYTE ClientID[6]; /* The unique client ID (created used NIC address). */
    USHORT ibSSPI;
    USHORT cbSSPI; /* ibSSPI & cbSSPI: SSPI data. */
    USHORT ibAtchDBFile;
    USHORT cchAtchDBFile; /* ibAtchDBFile & cchAtchDBFile: The file name for a database that is to be attached during the connection process. */
#if __TDS_7_2
    USHORT ibChangePassword; /* (introduced in TDS 7.2) */
    USHORT cchChangePassword; /* (introduced in TDS 7.2) ibChangePassword & cchChangePassword: New password for the specified login. Introduced in TDS 7.2. */
    DWORD cbSSPILong; /* (introduced in TDS 7.2) Used for large SSPI data when cbSSPI==USHRT_MAX. Introduced in TDS7.2. */
#endif
} LOGIN7_OffsetLength;

typedef struct {
    BYTE FeatureId;      /* The unique identifier number of a feature. */
    DWORD FeatureDataLen;/* The length, in bytes, of FeatureData for the corresponding FeatureID. */
    BYTE *FeatureData;   /* Data of the feature. */
} LOGIN7_FeatureOpt;

typedef struct {
    DWORD Length;        /* The total length of the LOGIN7 structure. */
    DWORD TDSVersion;    /* The highest TDS version being used by the client */
    DWORD PacketSize;    /* The packet size being requested by the client. */
    DWORD ClientProgVer; /* The version of the interface library (for example, ODBC or OLEDB) being used by the client */
    DWORD ClientPID;     /* The process ID of the client application. */
    DWORD ConnectionID;  /* The connection ID of the primary Server. */
    BYTE OptionFlags1;   /*  */
    BYTE OptionFlags2;   /*  */
    BYTE TypeFlags;      /*  */
    BYTE OptionFlags3;   /*  */
    LONG ClientTimZone;  /* The time zone of the client machine */
    ULONG ClientLCID;    /* The language code identifier (LCID) value for the client collation. */
    LOGIN7_OffsetLength OffsetLength; /* The variable portion of this message. */
    BYTE *Data;         /* The actual variable-length data portion referred to by OffsetLength. */
    LOGIN7_FeatureOpt FeatureExt; /* (introduced in TDS 7.4) The data block that can be used to inform and/or negotiate features between client and server. 有多个，以0xff结尾。此处做缓存*/
} LOGIN7;

int LOGIN7_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    unsigned char *pos = data;
    LOGIN7 LOGIN7_info;
    char *cur;
    int maxleftlen, tmpbuflen;
    char HostName[256], UserName[256], AppName[256], ServerName[256],
         CltIntName[256], curLanguage[256], curDatabase[256];

    if (sizeof(LOGIN7) > len) {TDS_PRINT_ERR_MSG("sizeof(LOGIN7) > len"); return -1;}

    STEP(pos, len-(pos-data), 4, DWORD, LOGIN7_info.Length);
    STEP(pos, len-(pos-data), 4, DWORD, LOGIN7_info.TDSVersion);
    STEP(pos, len-(pos-data), 4, DWORD, LOGIN7_info.PacketSize);
    STEP(pos, len-(pos-data), 4, DWORD, LOGIN7_info.ClientProgVer);
    STEP(pos, len-(pos-data), 4, DWORD, LOGIN7_info.ClientPID);
    STEP(pos, len-(pos-data), 4, DWORD, LOGIN7_info.ConnectionID);
    STEP(pos, len-(pos-data), 1, BYTE, LOGIN7_info.OptionFlags1);
    STEP(pos, len-(pos-data), 1, BYTE, LOGIN7_info.OptionFlags2);
    STEP(pos, len-(pos-data), 1, BYTE, LOGIN7_info.TypeFlags);
    STEP(pos, len-(pos-data), 1, BYTE, LOGIN7_info.OptionFlags3);
    STEP(pos, len-(pos-data), 4, LONG, LOGIN7_info.ClientTimZone);
    STEP(pos, len-(pos-data), 4, ULONG, LOGIN7_info.ClientLCID);
    if (len-(pos-data) >= sizeof(LOGIN7_OffsetLength)) {
        LOGIN7_info.OffsetLength = *(LOGIN7_OffsetLength *)pos;
        STEP_N(pos, len-(pos-data), sizeof(LOGIN7_OffsetLength));
    } else {TDS_PRINT_ERR_MSG("short of length"); return -1;}
    LOGIN7_info.Data = (BYTE *)pos;

    cur = resultStr;
    maxleftlen = maxsize;
    sprintf(tmpbuf,
        "TDSVersion=%u;<br>ClientProgVer=%u;<br>ClientPID==%u;<br>ClientLCID=%u;<br>",
        LOGIN7_info.TDSVersion, LOGIN7_info.ClientProgVer, LOGIN7_info.ClientPID, LOGIN7_info.ClientLCID);
    tmpbuflen = strlen(tmpbuf);
    COPY_STR(cur, maxleftlen, tmpbuf, tmpbuflen);
    cur += tmpbuflen;
    maxleftlen -= tmpbuflen;

    memcpy(HostName, data+LOGIN7_info.OffsetLength.ibHostName, LOGIN7_info.OffsetLength.cchHostName*2);
    memcpy(UserName, data+LOGIN7_info.OffsetLength.ibUserName, LOGIN7_info.OffsetLength.cchUserName*2);
    memcpy(AppName, data+LOGIN7_info.OffsetLength.ibAppName, LOGIN7_info.OffsetLength.cchAppName*2);
    memcpy(ServerName, data+LOGIN7_info.OffsetLength.ibServerName, LOGIN7_info.OffsetLength.cchServerName*2);
    memcpy(CltIntName, data+LOGIN7_info.OffsetLength.ibCltIntName, LOGIN7_info.OffsetLength.cchCltIntName*2);
    memcpy(curLanguage, data+LOGIN7_info.OffsetLength.ibLanguage, LOGIN7_info.OffsetLength.cchLanguage*2);
    memcpy(curDatabase, data+LOGIN7_info.OffsetLength.ibDatabase, LOGIN7_info.OffsetLength.cchDatabase*2);
    CLR_ZERO_CHARS(HostName, LOGIN7_info.OffsetLength.cchHostName*2);
    CLR_ZERO_CHARS(UserName, LOGIN7_info.OffsetLength.cchUserName*2);
    CLR_ZERO_CHARS(AppName, LOGIN7_info.OffsetLength.cchAppName*2);
    CLR_ZERO_CHARS(ServerName, LOGIN7_info.OffsetLength.cchServerName*2);
    CLR_ZERO_CHARS(CltIntName, LOGIN7_info.OffsetLength.cchCltIntName*2);
    CLR_ZERO_CHARS(curLanguage, LOGIN7_info.OffsetLength.cchLanguage*2);
    CLR_ZERO_CHARS(curDatabase, LOGIN7_info.OffsetLength.cchDatabase*2);
    sprintf(tmpbuf,
            "HostName=%s;<br>UserName=%s;<br>AppName=%s;<br>ServerName=%s;<br>"
            "CltIntName=%s;<br>Language=%s;<br>Database=%s;<br>",
            HostName, UserName, AppName, ServerName, CltIntName, curLanguage, curDatabase);
    tmpbuflen = strlen(tmpbuf);
    COPY_STR(cur, maxleftlen, tmpbuf, tmpbuflen);
    cur += tmpbuflen;
    maxleftlen -= tmpbuflen;

    if (TDS_VERSION >= TDS_V_7_4 ) {
        //FeatureExt暂略
    }
    return 0;
}

#if 0
/*==================================================================================*/
/* 来自2.2.6.5节 */
/* struct __PRELOGIN_OPTION.PL_OPTION_TOKEN的取值范围 */
#define PL_OT_VERSION 0x00
#define PL_OT_ENCRYPTION 0x01
#define PL_OT_INSTOPT 0x02
#define PL_OT_THREADID 0x03
#define PL_OT_MARS 0x04
#define PL_OT_TRACEID 0x05
#define PL_OT_FEDAUTHREQUIRED 0x06
#define PL_OT_NONCEOPT 0x07
#define PL_OT_TERMINATOR 0xff

/* 当struct __PRELOGIN_OPTION.PL_OPTION_TOKEN取上面的几个值时，
   struct __PRELOGIN.PL_OPTION_DATA会相应的指向下面对应的结构 */
typedef struct {
    ULONG UL_VERSION; /* version of the sender */
    USHORT US_SUBBUILD; /* sub-build number of the sender */
} PL_OT_DATA_VERSION;

typedef struct {
    BYTE B_FENCRYPTION; /*  */
} PL_OT_DATA_ENCRYPTION;

typedef struct {
    BYTE *B_INSTVALIDITY; /* name of the instance of the database server that supports SQL or just 0x00 */
} PL_OT_DATA_INSTOPT;

typedef struct {
    ULONG UL_THREADID; /* client application thread id used for debugging purposes */
} PL_OT_DATA_THREADID;

typedef struct {
    BYTE B_MARS; /* sender requests MARS support */
} PL_OT_DATA_MARS;

typedef struct {
    BYTE GUID_CONNID[16]; /* client application trace id used for debugging purposes */
    BYTE ACTIVITYID[20]; /* client application activity id */
} PL_OT_DATA_TRACEID;

typedef struct {
    BYTE B_FEDAUTHREQUIRED; /* authentication library requirement of the sender when using Integrated Authentication identity */
} PL_OT_DATA_FEDAUTHREQUIRED;

typedef struct {
    BYTE NONCE[32]; /* nonce to be encrypted by using session key from federated authentication handshake */
} PL_OT_DATA_NONCEOPT;

typedef struct {
    BYTE PL_OPTION_TOKEN;    /* token value representing the option */
    USHORT PL_OFFSET;        /* 注意 - 是大端字节序 */
    USHORT PL_OPTION_LENGTH; /* 注意 - 是大端字节序 */
} PRELOGIN_OPTION;

/* N个PRELOGIN_OPTION，1个PL_OT_TERMINATOR结尾，然后是Data
   所有的PRELOGIN_OPTION.PL_OPTION_LENGTH相加，就是Data的字节数*/
typedef struct {
    PRELOGIN_OPTION PRELOGIN_option; /* 多个'struct __PRELOGIN_OPTION'，以0xff结尾 */
    BYTE *PL_OPTION_DATA; /* actual data for the option */
    BYTE *SSL_PAYLOAD;    /* SSL handshake raw payload */
} PRELOGIN;

int PRELOGIN_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    PRELOGIN PRELOGIN_info;
    unsigned char *pos = data;
    char *curprs = resultStr;
    char bufstr[1024] = {0}, tmpdata[1024]={0};
    PL_OT_DATA_VERSION VERSIONtmp;
    PL_OT_DATA_ENCRYPTION ENCRYPTIONtmp;
    PL_OT_DATA_THREADID THREADIDtmp;
    PL_OT_DATA_MARS MARStmp;
    PL_OT_DATA_FEDAUTHREQUIRED FEDAUTHREQUIREDtmp;

    switch (*(unsigned char *)pos) {
        case PL_OT_VERSION :
        case PL_OT_ENCRYPTION :
        case PL_OT_INSTOPT :
        case PL_OT_THREADID :
        case PL_OT_MARS :
        case PL_OT_TRACEID :
        case PL_OT_FEDAUTHREQUIRED :
        case PL_OT_NONCEOPT :
        case PL_OT_TERMINATOR :
            break;
        default:
            PRELOGIN_info.SSL_PAYLOAD = (BYTE *)pos;
            return 0;
    }

    while (*pos != PL_OT_TERMINATOR) {
        if (len - (pos - data) > sizeof(PRELOGIN_OPTION)) {
            PRELOGIN_info.PRELOGIN_option = *(PRELOGIN_OPTION *)pos;
            big_2_host(&(PRELOGIN_info.PRELOGIN_option.PL_OFFSET), 2);
            big_2_host(&(PRELOGIN_info.PRELOGIN_option.PL_OPTION_LENGTH), 2);

            if (PRELOGIN_info.PRELOGIN_option.PL_OFFSET+PRELOGIN_info.PRELOGIN_option.PL_OPTION_LENGTH > len) {
                TDS_PRINT_ERR_MSG("incomplete data");
                return -1;
            }
            /* 注意给tmpdata中的'\0'预留空位 */
            COPY_STR(tmpdata, sizeof(tmpdata)-1, data+PRELOGIN_info.PRELOGIN_option.PL_OFFSET, PRELOGIN_info.PRELOGIN_option.PL_OPTION_LENGTH);
            switch (PRELOGIN_info.PRELOGIN_option.PL_OPTION_TOKEN) {
                case PL_OT_VERSION:
                    VERSIONtmp = *(PL_OT_DATA_VERSION *)tmpdata;
                    sprintf(bufstr, "VERSION=%u;<br>SUBBUILD=%u;<br>", VERSIONtmp.UL_VERSION, VERSIONtmp.US_SUBBUILD);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
                case PL_OT_ENCRYPTION :
                    ENCRYPTIONtmp = *(PL_OT_DATA_ENCRYPTION *)tmpdata;
                    sprintf(bufstr, "FENCRYPTION=%u;<br>", ENCRYPTIONtmp.B_FENCRYPTION);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
                case PL_OT_INSTOPT :
                    tmpdata[PRELOGIN_info.PRELOGIN_option.PL_OPTION_LENGTH] = '\0';
                    sprintf(bufstr, "INSTVALIDITY=%s;<br>", tmpdata);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
                case PL_OT_THREADID :
                    THREADIDtmp = *(PL_OT_DATA_THREADID *)tmpdata;
                    sprintf(bufstr, "THREADID=%u;<br>", THREADIDtmp.UL_THREADID);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
                case PL_OT_MARS :
                    MARStmp = *(PL_OT_DATA_MARS *)tmpdata;
                    sprintf(bufstr, "MARS=%u;<br>", MARStmp.B_MARS);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
                case PL_OT_TRACEID :
                    /* debug用途，暂略 */
                    break;
                case PL_OT_FEDAUTHREQUIRED :
                    FEDAUTHREQUIREDtmp = *(PL_OT_DATA_FEDAUTHREQUIRED *)tmpdata;
                    sprintf(bufstr, "FEDAUTHREQUIRED=%u;<br>", FEDAUTHREQUIREDtmp.B_FEDAUTHREQUIRED);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
                case PL_OT_NONCEOPT :
                    tmpdata[32] = '\0';
                    sprintf(bufstr, "NONCEOPT=%s;<br>", tmpdata);
                    COPY_STR(curprs, maxsize-(curprs-resultStr), bufstr, strlen(bufstr));
                    curprs += strlen(bufstr);
                    break;
            }
        } else { TDS_PRINT_ERR_MSG("short of length"); return -1; }
        STEP_N(pos, len-(pos-data), sizeof(PRELOGIN_OPTION));
    }
    *curprs = '\0';
    pos++; /* 跳过0xFF，别丢了这个 */
    return 0;
}

/*==================================================================================*/
/* 来自2.2.6.6节 */

/* 当前两个字符是0xff 0xff时，后跟是'ProcID'。否则该结构只有'ProcName'域 */
typedef struct {
    /* 可选1 */
    US_VARCHAR ProcName;

    /* 可选2 */
    UCHAR ProcIDSwitch[2]; /*  =0xffff*/
    USHORT ProcID; /* The number identifying the special stored procedure to be executed. */
} RPCRequest_NameLenProcID;

typedef struct {
    B_VARCHAR B_VARCHARstr;
    BYTE StatusFlags;
    TVP_TYPE_INFO tvp_type_info;
    struct __TYPE_INFO TYPE_INFO;
} RPCRequest_ParamMetaData;

typedef struct {
    struct __TYPE_INFO TYPE_INFO;
    BYTE EncryptionAlgo;
    B_VARCHAR AlgoName;
    BYTE EncryptionType;
    BYTE *CekHash; /* ??文档中没有该域的定义 */
    BYTE NormVersion;
} RPCRequest_ParamCipherInfo;

typedef struct {
    RPCRequest_ParamMetaData ParamMetaData;
    TYPE_VARBYTE ParamLenData; /* 2.2.5.2.3节指定的TYPE_VARBYTE规则 */
    RPCRequest_ParamCipherInfo ParamCipherInfo;
} RPCRequest_ParameterData;

typedef struct {
    RPCRequest_NameLenProcID NameLenProcID;
    USHORT OptionFlags;
    RPCRequest_ParameterData ParameterData; /* 多个'struct __ParameterData'，此处做缓存 */
} RPCRequest_RPCReqBatch;

typedef struct {
    ALL_HEADERS all_headers; /* TDS 7.2引入 */
    RPCRequest_RPCReqBatch RPCReqBatch;
    RPCRequest_RPCReqBatch *pRPCReqBatch;
} RPCRequest;

/* Request to execute an RPC. */
int RPCRequest_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    unsigned char *pos = data;
    RPCRequest RPCRequest_info;
    int leftlen, retval;

    if (TDS_VERSION >= TDS_V_7_2 ) {
        leftlen = len;
        retval = ALL_HEADERS_parse(pos, &leftlen, &(RPCRequest_info.all_headers));
        if (retval < 0) return -1;
        STEP_N(pos, len-(pos-data), leftlen);
    }

    if (0xff==pos[0] && 0xff==pos[1]) {
        RPCRequest_info.RPCReqBatch.NameLenProcID.ProcIDSwitch[0] = 0xff;
        RPCRequest_info.RPCReqBatch.NameLenProcID.ProcIDSwitch[1] = 0xff;
        STEP_N(pos, len-(pos-data), 2);
        STEP(pos, len-(pos-data), 2, USHORT, RPCRequest_info.RPCReqBatch.NameLenProcID.ProcID);
    } else {
        leftlen = len - (pos - data);
        retval = US_VARCHAR_parse(pos, &leftlen, &RPCRequest_info.RPCReqBatch.NameLenProcID.ProcName);
        if (retval < 0) return -1;
        STEP_N(pos, len-(pos-data), leftlen);
    }

    STEP(pos, len-(pos-data), 2, USHORT, RPCRequest_info.RPCReqBatch.OptionFlags);

    return 0;
}
#endif

/*==================================================================================*/
/* 来自2.2.6.7节 */
typedef struct {
    ALL_HEADERS all_headers; /* TDS 7.2引入 */
    BYTE *SQLText; /* Unicode流，UCS-2，单位2字节 */
} SQLBatch;

int SQLBatch_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    unsigned char *pos = data;
    SQLBatch SQLBatch_info;
    int leftlen, retval, toCopyLen;

    if (TDS_VERSION >= TDS_V_7_2 ) {
        leftlen = len;
        retval = ALL_HEADERS_parse(pos, &leftlen, &(SQLBatch_info.all_headers));
        if (retval < 0) return -1;
        STEP_N(pos, len-(pos-data), leftlen);
    }

    SQLBatch_info.SQLText = (BYTE *)pos;
    toCopyLen = (len-(pos-data)) > (sizeof(tmpbuf)-1) ? (sizeof(tmpbuf)-1) : (len-(pos-data));
    memcpy(tmpbuf, pos, toCopyLen);
    retval = codeConv("UCS-2", TDS_CHARSET, tmpbuf, toCopyLen, resultStr, maxsize);
    resultStr[retval] = '\0';

    return 0;
}

/*==================================================================================*/
/* 来自2.2.6.8节 */

typedef struct {
    BYTE *SSPIData;
}SSPI_PKT;

int SSPI_parse(char *data, int len, char *resultStr, int maxsize) {
    return 0;
}

#if 0
/*==================================================================================*/
/* 来自2.2.6.9节 */

/* struct __TransMgrReq.RequestType的取值范围*/
#define  TM_GET_DTC_ADDRESS 0 /* Returns DTC network address as a result set with a singlecolumn, single-row binary value. */
#define  TM_PROPAGATE_XACT 1 /* Imports DTC transaction into the server and returns a local transaction descriptor as a varbinary result set. */

/* TDS 7.2 */
#define  TM_BEGIN_XACT 5 /* Begins a transaction and returns the descriptor in an ENVCHANGE type 8. */
#define  TM_PROMOTE_XACT 6 /* Converts an active local transaction into a distributed transaction and returns an opaque buffer in an ENVCHANGE type 15 */
#define  TM_COMMIT_XACT 7 /* Commits a transaction. Depending on the payload of the request, it can additionally request that another local transaction be started */
#define  TM_ROLLBACK_XACT 8 /*  Rolls back a transaction. Depending on the payload of the request, it can indicate that after the rollback, a local transaction is to be started*/
#define  TM_SAVE_XACT 9 /* Sets a savepoint within the active transaction. This request MUST specify a nonempty name for the savepoint. */

/* 对于不同的'RequestType'，'RequestPayload'指向不同的结构，但这里不关心他的内容 */
typedef struct {
    ALL_HEADERS all_headers; /* TDS 7.2引入 */
    USHORT RequestType; /* The types of transaction manager operations that are requested by the client  */
    BYTE *RequestPayload;
} TransMgrReq;

int TransMgrReq_parse(unsigned char *data, int len, char *resultStr, int maxsize) {
    TransMgrReq TransMgrReq_info;
    unsigned char *pos = data;

    if(TDS_VERSION >= TDS_V_7_2) {
        TransMgrReq_info.all_headers.TotalLength = *(unsigned long *)pos;
        STEP_N(pos, len-(pos-data), TransMgrReq_info.all_headers.TotalLength); /* 跳过ALL_HEADERS */
    }

    STEP(pos, len-(pos-data), 2, USHORT, TransMgrReq_info.RequestType);
    TransMgrReq_info.RequestPayload = (BYTE *)pos;
    return 0;
}
#endif

/* 'n'字节的字符序列可以表示整数，获得该数
   例如{0x01,0x02,0x03,0x04,0x00,0x05}可以获得0x050004030201
   该函数有问题*******************/
unsigned long getUnsignedLongFromStr(char *NumStr, int n) {
    int i;
    unsigned long num = 0, base=1;
    for (i=0; i<n; i++) {
        //fprintf(stderr, "base=%lu,num=%lu,i=%d,NumStr[i]=0x%x\n", base, num, i, (unsigned char)NumStr[i]);
        num += ((unsigned char)NumStr[i]  * base);
        base *= 256;
    }
    return num;
}

/*==================================================================================*/

/* 例如data[] = {0xc1, 0xca, 0xa1, 0x45, 0xb6, 0x33, 0x24, 0x40}
   表示IEEE754数 = 0x402433b645a1cac1
   二进制 = 0100 0000 0010 0100 0011 0011 1011 0110 0100 0101 1010 0001 1100 1010 1100 0001
   符号位F(1位) = 0(+)
   指数位Z(11位) = (100 0000 0010)2 = (1026)10
   尾数W(52位) = 0.0100 0011 0011 1011 0110 0100 0101 1010 0001 1100 1010 1100 0001
   该IEEE754数表示的十进制数 = (-1)^F * 2^(Z-1023) * (1+W)
   应该是10.101*/
double getFloat64IEEE754(char *data) {
    double result;
    int i, j;
    int F = data[7]>>7;
    int Z = (data[6]>>4) + (data[7]<<4);
    double base = 0.5;
    double W = 0.0;

    W += ((data[6]&0x08)>>3) * base; base *= 0.5;
    W += ((data[6]&0x04)>>2) * base; base *= 0.5;
    W += ((data[6]&0x02)>>1) * base; base *= 0.5;
    W += (data[6]&0x01) * base; base *= 0.5;

    for (i=5; i>=0; i--) {
        for (j=7; j>=0; j--) {
            W += (((data[i]&(0x01<<j))>>j) * base);
            base *= 0.5;
        }
    }

    result = W+1;
    for (i=0; i<(Z-1023); i++) {
        result *= 2;
    }
    if (1 == F) result = -result;
    return result;
}

/* data[] = {0xb6, 0x33, 0x24, 0x40}
   表示IEEE754数 = 0x402433b6
   二进制 = 0100 0000 0010 0100 0011 0011 1011 0110
   符号位F(1位) = 0(+)
   指数位Z(8位) = (100 0000 0)2 = (128)10
   尾数W(23位) = 0.0100100001100111011010100111001
   该IEEE754数表示的十进制数 = (-1)^F * 2^(Z-127) * (1+W)
   应该是2.565656 */
double getFloat32IEEE754(char *data) {
    double result;
    int i, j;
    int F = data[3]>>7;
    int Z = (data[2]>>7) + (data[3]<<1);
    double base = 0.5;
    double W = 0.0;

    W += ((data[2]&0x40)>>6) * base; base *= 0.5;
    W += ((data[2]&0x20)>>5) * base; base *= 0.5;
    W += ((data[2]&0x10)>>4) * base; base *= 0.5;
    W += ((data[2]&0x08)>>3) * base; base *= 0.5;
    W += ((data[2]&0x04)>>2) * base; base *= 0.5;
    W += ((data[2]&0x02)>>1) * base; base *= 0.5;
    W += (data[2]&0x01) * base; base *= 0.5;

    for (i=1; i>=0; i--) {
        for (j=7; j>=0; j--) {
            W += (((data[i]&(0x01<<j))>>j) * base);
            base *= 0.5;
        }
    }

    result = W+1;
    for (i=0; i<(Z-127); i++) {
        result *= 2;
    }
    if (1 == F) result = 0-result;
    return result;
}

void swap(unsigned char *a, unsigned char *b) {
    unsigned char c;
    c = *a;
    *a = *b;
    *b = c;
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
static double getFloat64IEEE754_1(unsigned char *data) {
    int F, Z, i;
    double W, base, n;

    data[0] -= 0x52;
    swap(&data[0], &data[7]);
    swap(&data[1], &data[6]);
    swap(&data[2], &data[5]);
    swap(&data[3], &data[4]);

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

static double getFloat32IEEE754_1(unsigned char *data) {
    int F, Z, i;
    double W, base, n;

    data[0] -= 0x52;
    swap(&data[0], &data[3]);
    swap(&data[1], &data[2]);

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


int parseValues(unsigned char *data, int tocopy, struct __TYPE_INFO *TYPE_INFOres, char *resultStr, int maxsize) {
    double moneyFltVal;
    double moneyFltValMoreSignificant, moneyFltValLessSignificant;
    int year, month, today, days, mins, hour, min, sec, msec, usec, timeSize;
    short timeOffset, hourOff, minOff;
    char flagOff;
    unsigned long ssecs;
    double FLTNTYPE_ret;
    double numericVal;
    int nuericIdx;
    struct __TYPE_INFO tmpTYPE_INFO;

    if (0 == tocopy) {
        strcpy(ColValAfter, "NULL");
    } else {
        switch (TYPE_INFOres->LENTYPE) {
            /* FIXEDLENTYPE范围 */
            case NULLTYPE:
                strcpy(ColValAfter, "NULL");
                break;

            case INT1TYPE:
            case BITTYPE:
                sprintf(ColValAfter, "%d", ColValBefore[0]);
                break;

            case INT2TYPE:
                sprintf(ColValAfter, "%d", *(short *)ColValBefore);
                break;

            case FLT4TYPE:
                FLTNTYPE_ret = getFloat32IEEE754_1(ColValBefore);
                sprintf(ColValAfter, "%lf", FLTNTYPE_ret);
                break;

            case INT4TYPE:
                sprintf(ColValAfter, "%d", *(int *)ColValBefore);
                break;

            case DATETIM4TYPE: /* 测试*/
                days = *(short *)ColValBefore;
                mins = *(short *)(ColValBefore+2);
                getDateFromDaysSince19000101(days, &year, &month, &today);
                getMinsFrom00_00_00(mins, &hour, &min, &sec);
                sprintf(ColValAfter, "%04d-%02d-%02d %02d:%02d:%02d",
                        year, month, today, hour, min, sec);
                break;

            case MONEY4TYPE:
                moneyFltVal = getUnsignedLongFromStr(ColValBefore, 4);
                moneyFltVal /= 10000;
                sprintf(ColValAfter, "%lf", moneyFltVal);
                break;

            case MONEYTYPE:
                moneyFltValLessSignificant = getUnsignedLongFromStr(ColValBefore, 4);
                moneyFltValLessSignificant /= 10000;
                moneyFltValMoreSignificant = getUnsignedLongFromStr(ColValBefore+4, 4);
                moneyFltValMoreSignificant /= 10000;
                sprintf(ColValAfter, "%lf.%lf", moneyFltValMoreSignificant, moneyFltValLessSignificant);
                break;

            case DATETIMETYPE:
                days = *(LONG *)ColValBefore;
                mins = *(ULONG *)(&ColValBefore[4]);
                if (days > 0) {
                    getDateFromDaysSince19000101(days, &year, &month, &today);
                } else {
                    getDateFromDaysSince17530101(-days, &year, &month, &today);
                }
                getThreeTundredthsOfASecondFrom00_00_00(mins, &hour, &min, &sec, &msec);
                sprintf(ColValAfter, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                        year, month, today, hour, min, sec, msec);
                break;

            case FLT8TYPE:
                FLTNTYPE_ret = getFloat64IEEE754_1(ColValBefore);
                sprintf(ColValAfter, "%lf", FLTNTYPE_ret);
                break;

            case INT8TYPE:
                sprintf(ColValAfter, "%lld", *(long long*)ColValBefore);
                break;

            /* VARLENTYPE范围 */
            case GUIDTYPE:
            case BINARYTYPE:
            case VARBINARYTYPE:
            case BIGBINARYTYPE:
            case BIGVARBINTYPE:
            case IMAGETYPE:
                binaryToHexVisible(ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                break;

            case INTNTYPE:
            case BITNTYPE:
                if (1 == tocopy) {
                    sprintf(ColValAfter, "%d", ColValBefore[0]);
                } else if (2 == tocopy) {
                    sprintf(ColValAfter, "%d", *((short *)ColValBefore));
                } else if (4 == tocopy) {
                    sprintf(ColValAfter, "%d", *((int *)ColValBefore));
                } else if (8 == tocopy) {
                    sprintf(ColValAfter, "%lld", *((long long *)ColValBefore));
                }
                break;
            case DECIMALTYPE:
            case NUMERICTYPE:
            case DECIMALNTYPE:
            case NUMERICNTYPE:
                numericVal = getUnsignedLongFromStr(ColValBefore+1, tocopy-1);
                for (nuericIdx=0; nuericIdx < TYPE_INFOres->scale; nuericIdx++) {
                    numericVal *= 0.1;
                }
                if (0 == ColValBefore[0]) {
                    ColValAfter[0] = '-'; /* 第一个字符是正整数标志 */
                    sprintf(ColValAfter+1, "%lf", numericVal);
                } else {
                    sprintf(ColValAfter, "%lf", numericVal);
                }
                break;
            case FLTNTYPE:
                if (8 == tocopy) {
                    FLTNTYPE_ret = getFloat64IEEE754_1(ColValBefore);
                    sprintf(ColValAfter, "%lf", FLTNTYPE_ret);
                } else if (4 == tocopy) {
                    FLTNTYPE_ret = getFloat32IEEE754_1(ColValBefore);
                    sprintf(ColValAfter, "%lf", FLTNTYPE_ret);
                } else {
                    binaryToHexVisible(ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                }
                break;
            case MONEYNTYPE:
                if (4 == tocopy) {
                    moneyFltVal = *(int *)ColValBefore;
                    moneyFltVal *= 0.0001;
                    sprintf(ColValAfter, "%lf", moneyFltVal);
                } else if (8 == tocopy) {
                    moneyFltValLessSignificant = *(int *)ColValBefore;
                    moneyFltValLessSignificant *= 0.0001;
                    moneyFltValMoreSignificant = *(int *)(ColValBefore+4);
                    moneyFltValMoreSignificant *= 0.0001;
                    sprintf(ColValAfter, "%lf", moneyFltValMoreSignificant+moneyFltValLessSignificant);
                } else {
                    binaryToHexVisible(ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                }
                break;
            case DATETIMNTYPE:
                if (4 == tocopy) {
                    days = *(USHORT *)ColValBefore;
                    mins = *(USHORT *)(&ColValBefore[2]);
                    getDateFromDaysSince19000101(days, &year, &month, &today);
                    getMinsFrom00_00_00(mins, &hour, &min, &sec);
                    sprintf(ColValAfter, "%04d-%02d-%02d %02d:%02d:%02d",
                            year, month, today, hour, min, sec);
                } else if(8 == tocopy) {
                    days = *(LONG *)ColValBefore;
                    mins = *(ULONG *)(&ColValBefore[4]);
                    if (days > 0) {
                        getDateFromDaysSince19000101(days, &year, &month, &today);
                    } else {
                        getDateFromDaysSince17530101(-days, &year, &month, &today);
                    }
                    getThreeTundredthsOfASecondFrom00_00_00(mins, &hour, &min, &sec, &msec);
                    sprintf(ColValAfter, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                            year, month, today, hour, min, sec, msec);
                    break;
                } else {
                    binaryToHexVisible(ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                }

                break;

            case DATENTYPE:
                days = (int)getUnsignedLongFromStr(ColValBefore, 3);
                getDateFromDaysSince00010101(days, &year, &month, &today);
                sprintf(ColValAfter, "%04d-%02d-%02d", year, month, today);
                break;
            case TIMENTYPE:
                ssecs = getUnsignedLongFromStr(ColValBefore, tocopy);
                getTimeFromScaleSecsSince12AM(ssecs, TYPE_INFOres->scale, &hour, &min, &sec, &msec, &usec);
                sprintf(ColValAfter, "%02d:%02d:%02d.%03d%03d", hour, min, sec, msec, usec);
                break;

            case DATETIME2NTYPE:
                if (TYPE_INFOres->scale >=0 &&
                    TYPE_INFOres->scale <= 2) {
                    timeSize = 3;
                } else if (TYPE_INFOres->scale >=3 &&
                    TYPE_INFOres->scale <= 4) {
                    timeSize = 4;
                } else {
                    timeSize = 5;
                }
                if (timeSize + 3 != tocopy) {
                    strcpy(ColValAfter, "NULL");
                    break;
                }
                ssecs = getUnsignedLongFromStr(ColValBefore, timeSize);
                days = (int)getUnsignedLongFromStr(ColValBefore + timeSize, 3);
                getDateFromDaysSince00010101(days, &year, &month, &today);
                getTimeFromScaleSecsSince12AM(ssecs, TYPE_INFOres->scale, &hour, &min, &sec, &msec, &usec);
                sprintf(ColValAfter, "%04d-%02d-%02d %02d:%02d:%02d.%03d%03d",
                        year, month, today, hour, min, sec, msec, usec);
                break;

            case DATETIMEOFFSETNTYPE:
                if (TYPE_INFOres->scale >=0 &&
                    TYPE_INFOres->scale <= 2) {
                    timeSize = 3;
                } else if (TYPE_INFOres->scale >=3 &&
                    TYPE_INFOres->scale <= 4) {
                    timeSize = 4;
                } else {
                    timeSize = 5;
                }
                if (timeSize + 3 + 2 != tocopy) {
                    strcpy(ColValAfter, "NULL");
                    break;
                }
                ssecs = getUnsignedLongFromStr(ColValBefore, timeSize);
                days = (int)getUnsignedLongFromStr(ColValBefore + timeSize, 3);
                getDateFromDaysSince00010101(days, &year, &month, &today);
                getTimeFromScaleSecsSince12AM(ssecs, TYPE_INFOres->scale, &hour, &min, &sec, &msec, &usec);
                timeOffset = (short)getUnsignedLongFromStr(ColValBefore + timeSize + 3, 2);
                flagOff = (timeOffset >= 0) ? '+' : '-';
                if (timeOffset < 0) timeOffset= -timeOffset;
                hourOff = timeOffset / 60;
                minOff = timeOffset % 60;
                sprintf(ColValAfter, "%04d-%02d-%02d %02d:%02d:%02d.%04d%04d %c%02d:%02d",
                        year, month, today, hour, min, sec, msec, usec, flagOff, hourOff, minOff);
                break;

            case NVARCHARTYPE:
            case NCHARTYPE:
            case NTEXTTYPE:
                 codeConv("UCS-2", TDS_CHARSET, ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                 break;

            case CHARTYPE:
            case VARCHARTYPE:
            case BIGVARCHRTYPE:
            case BIGCHARTYPE:
            case TEXTTYPE:
                memcpy(ColValAfter, ColValBefore, tocopy);
                ColValAfter[tocopy] = '\0';
                break;

            case SSVARIANTTYPE:
                memset(&tmpTYPE_INFO, 0, sizeof(tmpTYPE_INFO));
                tmpTYPE_INFO.LENTYPE = (unsigned char)ColValBefore[0];
                switch(tmpTYPE_INFO.LENTYPE) {
					case GUIDTYPE:
					case BITTYPE:
                    case INT1TYPE:
                    case INT2TYPE:
                    case INT4TYPE:
                    case INT8TYPE:
                    case DATETIMETYPE:
                    case DATETIM4TYPE:
                    case FLT4TYPE:
                    case FLT8TYPE:
                    case MONEYTYPE:
                    case MONEY4TYPE:
                    case DATENTYPE:
                        if (tocopy <= 2) break;
                        memmove(ColValBefore, ColValBefore+2, tocopy-2);
                        parseValues(data, tocopy-2, &tmpTYPE_INFO, resultStr, maxsize);
                        break;
                    case TIMENTYPE:
                    case DATETIME2NTYPE:
                    case DATETIMEOFFSETNTYPE:
                        if (tocopy <= 3) break;
                        tmpTYPE_INFO.scale = ColValBefore[2];
                        memmove(ColValBefore, ColValBefore+3, tocopy-3);
                        parseValues(data, tocopy-3, &tmpTYPE_INFO, resultStr, maxsize);
                        break;
                    case BIGVARBINTYPE:
                    case BIGBINARYTYPE:
                        if (tocopy <= 4) break;
                        memmove(ColValBefore, ColValBefore+4, tocopy-4);
                        parseValues(data, tocopy-4, &tmpTYPE_INFO, resultStr, maxsize);
                        break;
                    case NUMERICNTYPE:
                    case DECIMALNTYPE:
                        if (tocopy <= 4) break;
                        tmpTYPE_INFO.precision = ColValBefore[2];
                        tmpTYPE_INFO.scale = ColValBefore[3];
                        memmove(ColValBefore, ColValBefore+4, tocopy-4);
                        parseValues(data, tocopy-4, &tmpTYPE_INFO, resultStr, maxsize);
                        break;
                    case BIGVARCHRTYPE:
                    case BIGCHARTYPE:
                    case NVARCHARTYPE:
                    case NCHARTYPE:
                        if (tocopy <= 9) break;
                        memmove(ColValBefore, ColValBefore+9, tocopy-9);
                        parseValues(data, tocopy-9, &tmpTYPE_INFO, resultStr, maxsize);
                        break;
                    default:
                        binaryToHexVisible(ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                        break;
                }
                break;
            case XMLTYPE:
            case UDTTYPE:
            default:
                binaryToHexVisible(ColValBefore, tocopy, ColValAfter, sizeof(ColValAfter)-1);
                break;
        }
    }
    return 0;
}

int getDescriptionInRow(ROW *ROWres, COLMETADATA *COLMETADATAres, char *resultStr, int maxsize) {
    int i, tocopy;
    int plpIdx;

    for (i=0; i<COLMETADATAres->Count; i++) {
        memset(ColValAfter, 0, sizeof(ColValAfter));
        switch (COLMETADATAres->ColumnData[i].TYPE_INFO.typeFlag) {
            case PARTLENTYPE_USHORTMAXLEN:
            case PARTLENTYPE_USHORTMAXLEN_COLLATION:
            case PARTLENTYPE_XML_INFO:
            case PARTLENTYPE_UDT_INFO:
                if (0xFFFFFFFFFFFFFFFF == ROWres->AllColumnData[i].Data.plp_body.len) {
                    tocopy = 0;
                } else {
                    plpIdx = 0;
                    tocopy = 0;
                    while (ROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len != 0) {
                        memcpy(ColValBefore + tocopy, ROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].data, ROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len);
                        tocopy += (ROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len);
                        plpIdx++;
                    }
                }
                break;
            default:
                tocopy = (sizeof(ColValBefore)-1) > (ROWres->AllColumnData[i].Data.len.reallen) ?
                         (ROWres->AllColumnData[i].Data.len.reallen) : (sizeof(ColValBefore)-1);
                if (tocopy > 0) memcpy(ColValBefore, ROWres->AllColumnData[i].Data.data, tocopy);
                break;
        }

        parseValues(ColValBefore, tocopy, &(COLMETADATAres->ColumnData[i].TYPE_INFO), resultStr, maxsize);

        if (0 != i && maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:s");
        if (maxsize-strlen(resultStr) > strlen(ColValAfter)) strcat(resultStr, ColValAfter);
    }
    return 0;
}

int getDescriptionInNBCROW(NBCROW *NBCROWres, COLMETADATA *COLMETADATAres, char *resultStr, int maxsize) {
    int i, tocopy;
    int plpIdx;
    for (i=0; i<COLMETADATAres->Count; i++) {
        memset(ColValAfter, 0, sizeof(ColValAfter));
        switch (COLMETADATAres->ColumnData[i].TYPE_INFO.typeFlag) {
            case PARTLENTYPE_USHORTMAXLEN:
            case PARTLENTYPE_USHORTMAXLEN_COLLATION:
            case PARTLENTYPE_XML_INFO:
            case PARTLENTYPE_UDT_INFO:
                if (0xFFFFFFFFFFFFFFFF == NBCROWres->AllColumnData[i].Data.plp_body.len) {
                    tocopy = 0;
                } else {
                    plpIdx = 0;
                    tocopy = 0;
                    while (NBCROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len != 0) {
                        memcpy(ColValBefore + tocopy, NBCROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].data, NBCROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len);
                        tocopy += (NBCROWres->AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len);
                        plpIdx++;
                    }
                }
                break;
            default:
                tocopy = (sizeof(ColValBefore)-1) > (NBCROWres->AllColumnData[i].Data.len.reallen) ?
                         (NBCROWres->AllColumnData[i].Data.len.reallen) : (sizeof(ColValBefore)-1);
                if (tocopy > 0) memcpy(ColValBefore, NBCROWres->AllColumnData[i].Data.data, tocopy);
                break;
        }

        parseValues(ColValBefore, tocopy, &(COLMETADATAres->ColumnData[i].TYPE_INFO), resultStr, maxsize);

        if (0 != i && maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:s");
        if (maxsize-strlen(resultStr) > strlen(ColValAfter)) strcat(resultStr, ColValAfter);
    }
    return 0;
}

int getDescriptionInALTROW(ALTROW *ALTROWres, ALTMETADATA *ALTMETADATAres, char *resultStr, int maxsize) {
    int i, tocopy;
    int plpIdx;
    for (i=0; i<ALTMETADATAres->Count; i++) {
        memset(ColValAfter, 0, sizeof(ColValAfter));
        switch (ALTMETADATAres->ComputeData[i].TYPE_INFO.typeFlag) {
            case PARTLENTYPE_USHORTMAXLEN:
            case PARTLENTYPE_USHORTMAXLEN_COLLATION:
            case PARTLENTYPE_XML_INFO:
            case PARTLENTYPE_UDT_INFO:
                if (0xFFFFFFFFFFFFFFFF == ALTROWres->ComputeData[i].plp_body.len) {
                    tocopy = 0;
                } else {
                    plpIdx = 0;
                    tocopy = 0;
                    while (ALTROWres->ComputeData[i].plp_body.plp_chunk[plpIdx].len != 0) {
                        memcpy(ColValBefore + tocopy, ALTROWres->ComputeData[i].plp_body.plp_chunk[plpIdx].data, NBCROWres.AllColumnData[i].Data.plp_body.plp_chunk[plpIdx].len);
                        tocopy += ALTROWres->ComputeData[i].plp_body.plp_chunk[plpIdx].len;
                        plpIdx++;
                    }
                }
                break;
            default:
                tocopy = (sizeof(ColValBefore)-1) > (ALTROWres->ComputeData[i].len.reallen) ?
                         (ALTROWres->ComputeData[i].len.reallen) : (sizeof(ColValBefore)-1);
                if (tocopy > 0) memcpy(ColValBefore, ALTROWres->ComputeData[i].data, tocopy);
                break;
        }

        parseValues(ColValBefore, tocopy, &(ALTMETADATAres->ComputeData[i].TYPE_INFO), resultStr, maxsize);

        if (0 != i && maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:s");
        if (maxsize-strlen(resultStr) > strlen(ColValAfter)) strcat(resultStr, ColValAfter);
    }
    return 0;
}

/* 把'res'中有效信息填入'resultStr' */
int getDescriptionInALTMETADATA(ALTMETADATA *res, char *resultStr, int maxsize) {
    int i, tocopy, retval;
    char ColNameUCS2[1024], ColName[1024];

    for (i=0; i<res->Count; i++) {
        tocopy = (sizeof(ColNameUCS2)-1) > (res->ComputeData[i].ColName.wlen * 2) ?
                 (res->ComputeData[i].ColName.wlen * 2) : (sizeof(ColNameUCS2)-1);
        memcpy(ColNameUCS2, res->ComputeData[i].ColName.data, tocopy);
        retval = codeConv("UCS-2", TDS_CHARSET, ColNameUCS2, tocopy, ColName, sizeof(ColName)-1);
        if (retval < 0) return -1;
        if (0 != i && maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:s");
        if (maxsize-strlen(resultStr) > strlen(ColName)) strcat(resultStr, ColName);
    }
    if (maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:n");
    return 0;
}

/* 来自2.2.3.1.1节
   根据2.2.3.1.1节的第2个表，可以看出server发送的TDS数据，只有一种类型 -
   TYPE_Tabular_result
   即表格数据结果集，它们有1个或多个Token组成
   注意 - ;:s表示空格，
          ;:n表示回车，
          ;:r表示后跟只处理不可显字符的包
   注意 - 'data'是完整的包，没有去掉8字节TDS头 */
int ServerMgr_parse(unsigned char *data, int len, char *resultStr, int maxsize, int *line_num) {
    int retval, leftlen, curPktLen = 0;
    unsigned char *pos=NULL, *curPktStart= NULL;
    TDS_HEADERS tdsh;

    *line_num = 0; /* 2015-11-17: select语句返回的行数 */

    if (len <= 8) {
        return 0;
    }

    tdsh = *(TDS_HEADERS *)data;
    big_2_host(&(tdsh.Length), 2);

    if (tdsh.Length < len) {
        curPktLen = tdsRecombination(data, len, tabularResultBuf, sizeof(tabularResultBuf));
        fprintf(stderr, "tdsRecombination len = %d\n", curPktLen);
        curPktStart = tabularResultBuf;
    } else {
        curPktStart = data;
        curPktLen = len;
    }

    pos = curPktStart + 8;
    leftlen  = curPktLen - 8;

    while (pos < curPktStart+curPktLen) {
        #if TDS_DEBUG_OPEN
        fprintf(stderr, "DEBUG : <%s> : %d : Type=0x%x\n", __FILE__, __LINE__, *pos);
        #endif

        if (COLMETADATA_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);

            retval = COLMETADATA_token_parse(pos, &leftlen, &COLMETADATAres, 0);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
            getDescriptionInCOLMETADATA(&COLMETADATAres, resultStr, maxsize);
        } else if (ROW_TOKEN == *pos) {
            *line_num = *line_num + 1;
            leftlen = len - (pos - curPktStart);
            memset(&ROWres, 0, sizeof(ROW));

            retval = ROW_token_parse(pos, &leftlen, &ROWres, &COLMETADATAres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
            getDescriptionInRow(&ROWres, &COLMETADATAres, resultStr+strlen(resultStr), maxsize-strlen(resultStr));

            if (maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:n");
        } else if (NBCROW_TOKEN == *pos) {
            *line_num = *line_num + 1;
            leftlen = len - (pos - curPktStart);
            memset(&NBCROWres, 0, sizeof(NBCROW));

            retval = NBCROW_token_parse(pos, &leftlen, &NBCROWres, &COLMETADATAres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);

            getDescriptionInNBCROW(&NBCROWres, &COLMETADATAres, resultStr+strlen(resultStr), maxsize-strlen(resultStr));
            if (maxsize-strlen(resultStr) > 3) strcat(resultStr, ";:n");
        } else if (ORDER_TOKEN == *pos) {
            ORDER ORDERres;
            leftlen = len - (pos - curPktStart);

            retval = ORDER_token_parse(pos, &leftlen, &ORDERres );
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (INFO_TOKEN == *pos) {
            INFO INFOres;
            leftlen = len - (pos - curPktStart);

            retval = INFO_token_parse(pos, &leftlen, &INFOres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (ENVCHANGE_TOKEN == *pos) {
            ENVCHANGE ENVCHANGEres;
            leftlen = len - (pos - curPktStart);

            retval = ENVCHANGE_token_parse(pos, &leftlen, &ENVCHANGEres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (DONE_TOKEN == *pos) {
            DONE DONEres;
            leftlen = len - (pos - curPktStart);

            retval = DONE_token_parse(pos, &leftlen, &DONEres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (DONEPROC_TOKEN == *pos) {
            DONEPROC DONEPROCres;
            leftlen = len - (pos - curPktStart);

            retval = DONEPROC_token_parse(pos, &leftlen, &DONEPROCres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (DONEINPROC_TOKEN == *pos) {
            DONEINPROC DONEINPROCres;
            leftlen = len - (pos - curPktStart);

            retval = DONEINPROC_token_parse(pos, &leftlen, &DONEINPROCres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        }  else if (ERROR_TOKEN == *pos) {
            ERROR ERRORres;
            leftlen = len - (pos - curPktStart);

            retval = ERROR_token_parse(pos, &leftlen, &ERRORres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);

            getDescriptionInERROR(&ERRORres, resultStr+strlen(resultStr), maxsize-strlen(resultStr));
        } else if (ALTMETADATA_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);

            retval = ALTMETADATA_token_parse(pos, &leftlen, &ALTMETADATAres, 0);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
            getDescriptionInALTMETADATA(&ALTMETADATAres, resultStr, maxsize);
        } else if (ALTROW_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);

            retval = ALTROW_token_parse(pos, &leftlen, &ALTROWres, &ALTMETADATAres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
            getDescriptionInALTROW(&ALTROWres, &ALTMETADATAres, resultStr, maxsize);
        } else if (COLINFO_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            COLINFO COLINFOres;

            retval = COLINFO_token_parse(pos, &leftlen, &COLINFOres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (LOGINACK_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            LOGINACK LOGINACKres;

            retval = LOGINACK_token_parse(pos, &leftlen, &LOGINACKres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (OFFSET_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            OFFSET OFFSETres;

            retval = OFFSET_token_parse(pos, &leftlen, &OFFSETres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (RETURNSTATUS_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            RETURNSTATUS RETURNSTATUSres;

            retval = RETURNSTATUS_token_parse(pos, &leftlen, &RETURNSTATUSres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (RETURNVALUE_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            RETURNVALUE RETURNVALUEres;

            retval = RETURNVALUE_token_parse(pos, &leftlen, &RETURNVALUEres, 0);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (SESSIONSTATE_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            SESSIONSTATE SESSIONSTATEres;

            retval = SESSIONSTATE_token_parse(pos, &leftlen, &SESSIONSTATEres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (SSPI_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            SSPI SSPIres;

            retval = SSPI_token_parse(pos, &leftlen, &SSPIres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (TABNAME_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            TABNAME TABNAMEres;

            retval = TABNAME_token_parse(pos, &leftlen, &TABNAMEres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else if (TVP_ROW_TOKEN == *pos) {
            leftlen = len - (pos - curPktStart);
            TVP_ROW TVP_ROWres;

            retval = TVP_ROW_token_parse(pos, &leftlen, &TVP_ROWres, &COLMETADATAres);
            if (retval < 0) return -1;
            STEP_N(pos, curPktLen-(pos-curPktStart), leftlen);
        } else {
            break;
        }
    }
    return 0;
}

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

/* 查找'sqlStatement'里的第一次出现的，引号外面的分号。 */
char *findNextSemicolonOrLinebreak(char *sqlStatement) {
    char *pos;
    int singleQuoteFlag, doubleQuoteFlag;

    pos = sqlStatement;
    singleQuoteFlag = doubleQuoteFlag = 0;
    while (*pos != '\0') {
        if (0==singleQuoteFlag && 0==doubleQuoteFlag) {
            if (';'==*pos || '\n'==*pos)
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


/* level = 4:INSERT, DELETE, UPDATE CREATE, DROP, ALTER
   level = 3:SELECT
   level = 2:USE GRANT , DENY , REVOKE COMMIT, ROLLBACK, SET
   level = 1 OTHER */
#define LEVEL_MAX 4

#define LEVEL_INSERT 4
#define LEVEL_DELETE 4
#define LEVEL_UPDATE 4
#define LEVEL_CREATE 4
#define LEVEL_DROP 4
#define LEVEL_ALTER 4

#define LEVEL_SELECT 3

#define LEVEL_USE 2
#define LEVEL_SET 2
#define LEVEL_GRANT 2
#define LEVEL_DENY 2
#define LEVEL_REVOKE 2
#define LEVEL_COMMIT 2
#define LEVEL_ROLLBACK 2

#define LEVEL_OTHER 1

typedef struct {
    char action[32];
    int level;
    int returnFlag;
}SQL_TYPE_STR_FLAG_RET;

static SQL_TYPE_STR_FLAG_RET sfr[] = {
    {"insert", LEVEL_INSERT, insert_flag},
    {"delete", LEVEL_DELETE, delete_flag},
    {"update", LEVEL_UPDATE, update_flag},
    {"create", LEVEL_CREATE, create_flag},
    {"drop", LEVEL_DROP, drop_flag},
    {"alter", LEVEL_ALTER, alter_flag},

    {"select", LEVEL_SELECT, select_flag},

    {"use", LEVEL_USE, use_flag},
    {"set", LEVEL_SET, set_flag},
    {"grant", LEVEL_GRANT, grant_flag},
    {"deny", LEVEL_DENY, deny_flag},
    {"revoke", LEVEL_REVOKE, revoke_flag},
    {"commit", LEVEL_COMMIT, commit_flag},
    {"rollback", LEVEL_ROLLBACK, rollback_flag},
};

/* <1>多句sql组合，每句间用分号或换行分割
   <2>sql语句中有引号(", ''"), 找下一句时要跳过
   <3>开头，sql语句间，结尾有注释，要跳过注释*/
int getHighestLevelSqlAction(char *sqlStatement) {
    char *start, *pos, actionWord[64];
    int tocopy, sqlLen, levelNow, returnFlag, i;

    levelNow = 0;
    sqlLen = strlen(sqlStatement);
    pos = sqlStatement;
    returnFlag = other_sql_state_flag;
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

        for (i = 0; i < 14; i++) {
            if ((strncasecmp(actionWord, sfr[i].action, strlen(sfr[i].action)) == 0) && (levelNow < sfr[i].level)) {
                levelNow = sfr[i].level;
                returnFlag = sfr[i].returnFlag;
                if (levelNow >= LEVEL_MAX) {
                    return returnFlag;
                }
                break;
            }
        }

        pos = findNextSemicolonOrLinebreak(start);
        if (NULL == pos) break;
        pos++; /* 跳过分号(;),进入下一句sql */
    }
    if (levelNow > 0) {
        return returnFlag;
    }
    return other_sql_state_flag; /* 其他操作语句 */
}


int getFirstSqlAction(char *sqlStatement) {
    char *start, *pos, actionWord[64];
    int tocopy;

    pos = sqlStatement;
    while (isspace((int)*pos) && *pos!='\0') {
        pos++;
    }
    start = pos;

    pos = strchr(start, ' ');
    if (NULL == pos) {
        tocopy = (strlen(start) > (sizeof(actionWord)-1)) ? strlen(start) : (pos-start);
    } else {
        tocopy = ((pos-start) > (sizeof(actionWord)-1)) ? (sizeof(actionWord)-1) : (pos-start);
    }
    memcpy(actionWord, start, tocopy);
    actionWord[tocopy] = '\0';
    if (strncasecmp(actionWord, "select", 6) == 0) return select_flag;
    if (strncasecmp(actionWord, "update", 6) == 0) return update_flag;
    if (strncasecmp(actionWord, "delete", 6) == 0) return delete_flag;
    if (strncasecmp(actionWord, "insert", 6) == 0) return insert_flag;
    if (strncasecmp(actionWord, "create", 6) == 0) return create_flag;
    if (strncasecmp(actionWord, "drop", 4) == 0) return drop_flag;
    return other_sql_state_flag; /* 其他操作语句 */
}

/*==================================================================================*/
/* 来自2.2.3节 */
/* 解析函数入口
   成功返回数据类型的appid值, 失败返回-1
   注意 - 一对request/reponse包，类型由request包确定。reponse包一般都是TYPE_Tabular_result
   类型，本函数对于reponse包的appid，返回30(定的appid范围之外的一个数).*/
int TDS_parser(unsigned char *data, int len, char *resultStr, int maxsize, int *line_num) {
    TDS_HEADERS tdsh;
    unsigned char *pos;
    int leftlen, retval;
    resultStr[0] = '\0';
    *line_num = 0;

    if (len <= 8) return other_sql_state_flag;

    tdsh = *(TDS_HEADERS *)data;
    big_2_host(&(tdsh.Length), 2);
    pos = data + 8;
    leftlen = len - 8;

    #if TDS_DEBUG_OPEN
    fprintf(stderr, "tdsh.type = 0x%02x\n", tdsh.Type);
    #endif

    switch (tdsh.Type) {
        case TYPE_SQL_batch:
            retval = SQLBatch_parse(pos, leftlen, resultStr, maxsize-1);
            if (retval < 0) { return error_flag; }
            return getHighestLevelSqlAction(resultStr);

        case TYPE_Tabular_result:
            ServerMgr_parse(data, len, resultStr, maxsize-1, line_num); /* 看2.2.3.1.1节内容 */
            return other_sql_state_flag;
        case TYPE_Pre_Login:
            //PRELOGIN_parse(pos, leftlen, resultStr, maxsize-1);
            return Pre_Login_flag;

        case TYPE_Pre_TDS7_Login:
        case TYPE_TDS7_Login:
            return TDS7_Login_flag;

        case TYPE_Federated_Authentication_Token:
            return Federated_Authentication_Token_flag;

        case TYPE_Bulk_load_data:
            //BulkLoadBCP_parse(pos, leftlen, resultStr, maxsize-1);
            return Bulk_load_data_flag;

        case TYPE_RPC:
            //RPCRequest_parse(pos, leftlen, resultStr, maxsize-1);
            return RPC_flag;

        case TYPE_Attention_signal:
            return Attention_signal_flag;

        case TYPE_Transaction_manager_request:
            //TransMgrReq_parse(pos, leftlen, resultStr, maxsize-1);
            return Transaction_manager_request_flag;

        case TYPE_SSPI:
            return SSPI_flag;

        default:
            return other_sql_state_flag;
    }
    return error_flag;
}

#if 0
char peer1_0[] = {
0x04, 0x01, 0x03, 0xe1, 0x00, 0x36, 0x01, 0x00,
0xe3, 0x2b, 0x00, 0x01, 0x0a, 0x61, 0x00, 0x75,
0x00, 0x64, 0x00, 0x69, 0x00, 0x74, 0x00, 0x5f,
0x00, 0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74,
0x00, 0x0a, 0x61, 0x00, 0x75, 0x00, 0x64, 0x00,
0x69, 0x00, 0x74, 0x00, 0x5f, 0x00, 0x74, 0x00,
0x65, 0x00, 0x73, 0x00, 0x74, 0x00, 0xab, 0x5e,
0x00, 0x45, 0x16, 0x00, 0x00, 0x01, 0x00, 0x19,
0x00, 0xf2, 0x5d, 0x06, 0x5c, 0x70, 0x65, 0x6e,
0x63, 0x93, 0x5e, 0x0a, 0x4e, 0x0b, 0x4e, 0x87,
0x65, 0xf4, 0x66, 0x39, 0x65, 0x3a, 0x4e, 0x20,
0x00, 0x27, 0x00, 0x61, 0x00, 0x75, 0x00, 0x64,
0x00, 0x69, 0x00, 0x74, 0x00, 0x5f, 0x00, 0x74,
0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00, 0x27,
0x00, 0x02, 0x30, 0x0f, 0x58, 0x00, 0x50, 0x00,
0x2d, 0x00, 0x32, 0x00, 0x30, 0x00, 0x31, 0x00,
0x33, 0x00, 0x31, 0x00, 0x30, 0x00, 0x33, 0x00,
0x31, 0x00, 0x31, 0x00, 0x38, 0x00, 0x33, 0x00,
0x35, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xe3,
0x08, 0x00, 0x07, 0x05, 0x04, 0x08, 0xd0, 0x00,
0x00, 0x00, 0xfd, 0x01, 0x00, 0xe2, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81,
0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00,
0xef, 0x14, 0x00, 0x04, 0x08, 0xd0, 0x00, 0x00,
0x02, 0x61, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00,
0x00, 0x09, 0x00, 0x63, 0xfe, 0xff, 0xff, 0x7f,
0x04, 0x08, 0xd0, 0x00, 0x00, 0x02, 0x03, 0x00,
0x64, 0x00, 0x62, 0x00, 0x6f, 0x00, 0x0a, 0x00,
0x74, 0x00, 0x61, 0x00, 0x62, 0x00, 0x6c, 0x00,
0x65, 0x00, 0x54, 0x00, 0x65, 0x00, 0x73, 0x00,
0x74, 0x00, 0x41, 0x00, 0x02, 0x61, 0x00, 0x32,
0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x6c,
0x11, 0x12, 0x00, 0x02, 0x61, 0x00, 0x33, 0x00,
0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0xe7, 0x64,
0x00, 0x04, 0x08, 0xd0, 0x00, 0x00, 0x02, 0x61,
0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
0x00, 0xe7, 0xff, 0xff, 0x04, 0x08, 0xd0, 0x00,
0x00, 0x02, 0x61, 0x00, 0x35, 0x00, 0xd1, 0x14,
0x00, 0x61, 0x00, 0x31, 0x00, 0x31, 0x00, 0x31,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x10, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65, 0x78, 0x74,
0x70, 0x74, 0x72, 0x00, 0x00, 0x00, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00, 0x08, 0x00,
0x00, 0x00, 0x61, 0x00, 0x32, 0x00, 0x32, 0x00,
0x32, 0x00, 0x05, 0x01, 0x03, 0x00, 0x00, 0x00,
0x02, 0x00, 0x34, 0x00, 0x02, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1, 0x14,
0x00, 0x31, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30,
0x00, 0x30, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x10, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65, 0x78, 0x74,
0x70, 0x74, 0x72, 0x00, 0x00, 0x00, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00, 0x0a, 0x00,
0x00, 0x00, 0x32, 0x00, 0x30, 0x00, 0x30, 0x00,
0x30, 0x00, 0x30, 0x00, 0x05, 0x01, 0x30, 0x75,
0x00, 0x00, 0x0a, 0x00, 0x34, 0x00, 0x30, 0x00,
0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x0a, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00,
0x00, 0x00, 0x35, 0x00, 0x30, 0x00, 0x30, 0x00,
0x30, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
0xd1, 0x14, 0x00, 0x61, 0x00, 0x31, 0x00, 0x31,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x10,
0x64, 0x75, 0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65,
0x78, 0x74, 0x70, 0x74, 0x72, 0x00, 0x00, 0x00,
0x64, 0x75, 0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00,
0x06, 0x00, 0x00, 0x00, 0x61, 0x00, 0x32, 0x00,
0x32, 0x00, 0x05, 0x01, 0x21, 0x00, 0x00, 0x00,
0x06, 0x00, 0x61, 0x00, 0x34, 0x00, 0x34, 0x00,
0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x06, 0x00, 0x00, 0x00, 0x61, 0x00, 0x35, 0x00,
0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1, 0x14,
0x00, 0x61, 0x00, 0x31, 0x00, 0x31, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x10, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65, 0x78, 0x74,
0x70, 0x74, 0x72, 0x00, 0x00, 0x00, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00, 0x06, 0x00,
0x00, 0x00, 0x61, 0x00, 0x32, 0x00, 0x32, 0x00,
0x05, 0x01, 0x21, 0x00, 0x00, 0x00, 0x06, 0x00,
0x61, 0x00, 0x34, 0x00, 0x34, 0x00, 0x02, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00,
0xd1, 0x14, 0x00, 0x61, 0x00, 0x31, 0x00, 0x31,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x10,
0x64, 0x75, 0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65,
0x78, 0x74, 0x70, 0x74, 0x72, 0x00, 0x00, 0x00,
0x64, 0x75, 0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00,
0x06, 0x00, 0x00, 0x00, 0x61, 0x00, 0x32, 0x00,
0x32, 0x00, 0x05, 0x01, 0x21, 0x00, 0x00, 0x00,
0x06, 0x00, 0x61, 0x00, 0x34, 0x00, 0x34, 0x00,
0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x06, 0x00, 0x00, 0x00, 0x61, 0x00, 0x35, 0x00,
0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd2, 0x1e,
0x14, 0x00, 0x11, 0x62, 0x84, 0x76, 0x4b, 0x6d,
0xd5, 0x8b, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00,
0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0xd1, 0x14,
0x00, 0x31, 0x00, 0x30, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x10, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65, 0x78, 0x74,
0x70, 0x74, 0x72, 0x00, 0x00, 0x00, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00, 0x04, 0x00,
0x00, 0x00, 0x32, 0x00, 0x30, 0x00, 0x05, 0x01,
0x1e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x34, 0x00,
0x30, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x35, 0x00,
0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd1, 0x14,
0x00, 0x31, 0x00, 0x30, 0x00, 0x30, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20,
0x00, 0x20, 0x00, 0x20, 0x00, 0x10, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x20, 0x74, 0x65, 0x78, 0x74,
0x70, 0x74, 0x72, 0x00, 0x00, 0x00, 0x64, 0x75,
0x6d, 0x6d, 0x79, 0x54, 0x53, 0x00, 0x06, 0x00,
0x00, 0x00, 0x32, 0x00, 0x30, 0x00, 0x30, 0x00,
0x05, 0x01, 0x2c, 0x01, 0x00, 0x00, 0x06, 0x00,
0x34, 0x00, 0x30, 0x00, 0x30, 0x00, 0x06, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
0x00, 0x00, 0x35, 0x00, 0x30, 0x00, 0x30, 0x00,
0x00, 0x00, 0x00, 0x00, 0xfd, 0x10, 0x00, 0xc1,
0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00 };

int main() {
int ret;
char res[9999] = {0};
int line;

ret = TDS_parser(peer1_0, sizeof(peer1_0), res, sizeof(res)-1, &line);
printf("%s\n", res);
printf("%d\n", line);
return 0;
}
#endif
