
//#define _GNU_SOURCE	

#include <iconv.h>
#include "csp_policy.h"
#include "audit_ensemble.h"
#include "audit_release.h"
#include "redis_new_api.h"
#include "pthread_pool.h"
#include <time.h>

#include <sched.h>






#define SQL_BACKUP "/home/zuo/"
#define SQL_DEBUG  "/home/debug/"



#define SQLKEYWORDLEN  6
#define OPTKEYWORDLEN	8

#define SQL_SEARCH_OPT	 	 	0x4451
#define SQL_SEARCH_OPT_RP 		0X5051
#define SQL_MODIFY_OPT	  		0X4455
#define SQL_MODIFY_OPT_RP 		0X5055
#define STUDIO_SAVEFILE_OPT 	0X4249
#define STUDIO_OPT		  		0X4237

#define SEP_FLAG 	";:s"
#define ENTER_FLAG	";:n"



//char request_cmd_debug[PATH_LEN];
//char response_cmd_debug[PATH_LEN];
unsigned char fileContent1[MAX_THREAD_NUM][REQ_RES_LEN];
unsigned char fileContent2[MAX_THREAD_NUM][REQ_RES_LEN];

unsigned char from_segment[MAX_THREAD_NUM][REQ_RES_LEN];
unsigned char to_segment[MAX_THREAD_NUM][REQ_RES_LEN];
unsigned char segment_str[MAX_THREAD_NUM][REQ_RES_LEN];

unsigned char ensemble_payload[MAX_THREAD_NUM][REQ_RES_LEN];
//unsigned char datamonth[AUDIT_TIME_LEN];
unsigned long first_len[MAX_THREAD_NUM];
//int  runFlag;

#define TEST_EFF_DECLARED unsigned long st,fi;
#define TEST_FUN_EFF(B,A) st=get_usec_time(); A; fi=get_usec_time(); //printf("%s:%lu,%lu,%d\n",B,fi,st,fi-st);

char *sqlKeyWord[SQLKEYWORDLEN]={
	"Select", //6
	"Update",//7
	"Delete",//8
	"Insert",//9
	"Create",//10
	"Drop"//11
};

OPTKEYS optKeyWord[OPTKEYWORDLEN]={
	{"%Studio.ClassMgr","SaveDefinition"}, //save cls
	{"%Studio.ClassMgr","GetDefinition"},//open cls
	{"%Library.qccServer","DeleteClassDefinition"},//delete cls
	{"%Library.RoutineMgr","Delete"},//delete file
	{"%Library.RoutineMgr","CompileClass"}, //compile
	{"New",""},
	{"Code",""},//open or save
	{"Read",""}//导出类
	};



int medtrakTest(unsigned char *data,unsigned int size){

	unsigned int  pLen;
	unsigned char *start;
	unsigned long readLen = 0;



	start = data;
	pLen = *((unsigned int *)start);
	if(pLen == 0) return -1;

	do{
		start = data+readLen;
		pLen = *((unsigned int *)start);
		if(pLen == 0){
			return -1;
		}
		if(pLen > size){
			return 0;
		}

		readLen += pLen;
	}while(readLen < size);

	if(readLen == size ) return 1;

	return 0;
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
		dst[srcLen]="\0";
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

//#define DEBUG_PRINT(s) fprintf(stderr, "<%s> : %d : %s\n", __FILE__, __LINE__, (s))
#define DEBUG_PRINT(s)
#if 0
int medtrakParser(char *data, int len, char *res, long *reslen) {
    char *pos = data + 56;
    int parseLen = 0, totalLen;
    unsigned short thisPartLen, flag;
    if (!data || len<0 || !res || !reslen) {
        DEBUG_PRINT("!data || len<0 || !res || !reslen");
        return -1;
    }
    totalLen = *(unsigned int *)data;
    if (totalLen != len) {
        DEBUG_PRINT("totalLen != len");
        return -1;
    }
    while (pos+2 <= data+len) {
        thisPartLen = *(unsigned short *)pos;
        pos += 2;
        if (pos + thisPartLen > data+len) {
            DEBUG_PRINT("pos + thisPartLen > data+len");
            return -1;
        }

        flag = *(unsigned short *)pos;
        pos += 2;
        thisPartLen -= 2;

        if (0x08 == flag) {
            int convLen = codeConv("UCS-2", "gbk", pos, thisPartLen, res+parseLen, *reslen-parseLen);
            if (convLen < 0) {
                DEBUG_PRINT("codeConv() failed");
                return -1;
            }
            parseLen += convLen;
            res[parseLen++] = ' ';
        } else if (0x0e == flag){
            memcpy(res+parseLen, pos, thisPartLen);
            parseLen += thisPartLen;
            res[parseLen++] = ' ';
        } else {
            DEBUG_PRINT("flag!=0x08 or 0x0e");
            return -1;
        }
        pos += thisPartLen;
    }
    *(res+parseLen) = '\0';
    *reslen = parseLen;
    return 0;
}
#endif
int medtrakParser(char *data, int len, char *res, long *reslen) {
    char *pos, tmpIdxStr[64];
    int parseLen, i, count;
    unsigned short thisPartLen, flag;
    if (*(unsigned int *)data != len) {
        DEBUG_PRINT("*(unsigned int *)data != len");
        strcpy(res, "NULL");
        *reslen = 4;
        return -1;
    }
    pos = data + 56;
    parseLen = count = 0;
    while (pos+2 <= data+len) {
        thisPartLen = *(unsigned short *)pos;
        pos += 2;
        if (pos + thisPartLen > data+len) { DEBUG_PRINT("pos + thisPartLen > data+len"); *(res+parseLen) = '\0', *reslen = parseLen; return -1; }

        flag = *(unsigned short *)pos;
        pos += 2;
        thisPartLen -= 2;

        #if 0
        sprintf(tmpIdxStr, " <%02d>", count++);
        strcpy(res+parseLen, tmpIdxStr);
        parseLen += strlen(tmpIdxStr);
        #else
        res[parseLen++] = ' ';
        #endif

        if (thisPartLen > 0) {
            if (0x08 == flag) {
                if (0x0 == pos[0]) {
                    strcpy(res+parseLen, "08NULL");
                    parseLen += 6;
                } else {
                    int convLen = codeConv("unicode", "gbk", pos, thisPartLen, res+parseLen, 10*1024*1024-parseLen);
                    if (convLen < 0) {
                        DEBUG_PRINT("codeConv() failed");
                        strcpy(res+parseLen, "codeCovNULL");
                        parseLen += 11;
                        return -1;
                    }
                    parseLen += convLen;
                }
            } else if (0x0e == flag){
                for (i=0; i<thisPartLen; i++) {
                    if (pos[i]<0x20 || pos[i]>0x7e) {
                        sprintf(res+parseLen, "0x%02x ", pos[i]);
                        parseLen += 5;
                    } else {
                        res[parseLen++] = pos[i];
                    }
                }
            } else {
                DEBUG_PRINT("flag!=0x08 or 0x0e");
                sprintf(res+parseLen, "flag=0x%04x:", flag);
                parseLen += 12;
                for (i=0; i<thisPartLen; i++) {
                    sprintf(res+parseLen, "0x%02x ", pos[i]);
                    parseLen += 5;
                }
            }
            pos += thisPartLen;
        } else {
            strcpy(res+parseLen, "NULL");
            parseLen += 4;
        }
    }
    *(res+parseLen) = '\0';
    *reslen = parseLen;
    return 0;
}


static int GetOperationType(char * resault){
	int i;
	for(i=0; i<OPTKEYWORDLEN; i++){
		if(i==5&&strcasestr(resault,optKeyWord[i].key1)){
			return 7;
		}
		if(i==6&&strcasestr(resault,optKeyWord[i].key1)){
			return 1;
		}
		if(i==7&&strcasestr(resault,optKeyWord[i].key1)){
			return 8;
		}
		if(strcasestr(resault,optKeyWord[i].key1)&&strcasestr(resault,optKeyWord[i].key2)){
			return (i+2);
		}
	}
	return 0;
}
static int GetAppType(char * resault){
	  int i=0;

	  for(i=0;i<6;i++){
		if(strcasestr(resault,sqlKeyWord[i])){
			return i+6;
/*
			switch(i){
				case 0:
					return 6;
				case 1:
					return 7;
				case 2:
					return 8;
				case 3:
					return 9;
				case 4:
					return 10;
				case 5:
					return 11;
				case 6:
					return 12;

			}*/
		}
	  }

	  return 0;
}
static int GetPolicyTime(unsigned long time){
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
static void SetMac(char *dest,char *src){
	int i,j;
	j=0;
	int len = 0;
	len = strlen(src);
	for(i=0;i<len;i++){
		dest[j]=src[i];

		j++;
		if(j%3==2){
			dest[j]='-';
			j++;
		}

	}
	dest[j-1]='\0';
}
static int IsOld(char *filePath, int secs) {
    time_t t;
	int retval;
    struct stat ft;

    retval = time(&t);
    if (retval < 0) return 0;

    retval = stat(filePath, &ft);
    if (retval < 0) return 0;

    if (t - ft.st_mtime > secs) return 1;
    return 0;
}

int AuditWrite(int Fd,char *psData,int Len){
	int writtenSuccsesBytes = 0;
	int writtenBytes = 0;
 	while(1){
		writtenSuccsesBytes = write(Fd,psData + writtenBytes,Len - writtenBytes );
		if (writtenSuccsesBytes == -1) {
			return -1;
			perror("write:");
		}
		writtenBytes += writtenSuccsesBytes;
		if (writtenBytes < Len) {
			continue;
		} else if (writtenBytes == Len) {
			break;
		}
	}
	return writtenBytes;
}

unsigned int AuditRead(int Fd,char *psData,int Len){
	ssize_t readSuccsesBytes = 0;
	unsigned int readBytes = 0;
	 do{
		readSuccsesBytes = read(Fd,psData + readBytes,Len - readBytes );
		if (readSuccsesBytes == -1) {
			return 0;
			perror("read:");
		}
		readBytes += readSuccsesBytes;
	 }while(readBytes < Len);
	 return readBytes;
}
static unsigned long GetFileSize(char *filename)
{
    struct stat buf;
    if(stat(filename, &buf)<0)
        {
        return 0;
    }
    return (unsigned long)buf.st_size;
}

time_t GetFileTime(char * filename){
	struct stat buf;
    if(stat(filename, &buf)<0)
        {
        return 0;
    }
	 return buf.st_mtime;
}

void GetValues(AUDIT_ENSEMBLE_REL *rel,char *filename){
	// /dev/shm/1/10_2887326051_000fe25c06a0_60100_168466306_80c16ef872cd_1433_0_2_1440906275885842_request
	unsigned short sport;
	unsigned short dport;
	unsigned short app_id;
	unsigned short user_id;
	unsigned int sip;
	unsigned int dip;
	unsigned int reqsize;
	unsigned int respsize;
	unsigned int seq;
	unsigned long time;
	int policy_time;
	char smac[20]={0},dmac[20]={0};
	int relval;
	relval = sscanf(filename, "/dev/shm/tmp/%hu/%hu_%u_%[^_]_%hu_%u_%[^_]_%hu_%hu_%u_%lu_request",
                    &rel->dir,&app_id, &sip, smac,&sport, &dip, dmac,&dport, &user_id,&seq , &time);

//	printf("%hu,%u,%s,%hu,%u,%s,%hu,%hu,%u,%lu\n", app_id, sip, smac,sport, dip, dmac,dport, user_id,seq , time);

	if(app_id == 100) rel->type=1;
	if(app_id == 110) rel->type=30;
	if(app_id == 40) rel->type=4;
	if(app_id>19 && app_id<30) rel->type=2;
	if(app_id ==10) rel->type=10;
//	sip = ntohl(sip);//网络转本机
//	dip = ntohl(dip);
	rel->id=app_id;
	rel->serport=dport;
	rel->cliport=sport;
	rel->userid=user_id;
	rel->times=time;
	TEST_EFF_DECLARED
	get_audit_time_3(rel->table);
	rel->userip = sip;//网络
	rel->desip = dip;
	//inet_ntop(AF_INET,(void*)(&sip),rel->userip,sizeof(rel->userip));
	//inet_ntop(AF_INET,(void*)(&dip),rel->desip,sizeof(rel->desip));
	SetMac(rel->srcmac,smac);
   // TEST_FUN_EFF("get_mac_str",get_mac_str(rel->userip,rel->srcmac);)
	SetMac(rel->desmac,dmac);
	policy_time = GetPolicyTime(time);
	rel->policytime=policy_time;
//	printf("policytime:%s\n",rel->policytime);
//	sprintf(rel->srcmac,"00-00-00-00-00-00");
//	sprintf(rel->desmac,"11-11-11-11-11-11");
	rel->saveflag = 0;
	rel->operation_len=0;
	rel->response_len=0;
	rel->request_size=0;
	rel->response_size=0;
	rel->line_num = 0;
	rel->sub_app_id=0;
	rel->request_type=0;
	rel->security_level=0;
	rel->request_time=0;
	rel->response_time=0;
	rel->interval_time=0;

	/*
	unsigned short dir;
		unsigned short request_type;
		unsigned short security_level;
		long operation_len;
		long response_len;
		int  type;
		int sub_app_id;
		char * request_data;
		char * response_data;
		unsigned long request_size;
		unsigned long response_size;
		int saveflag;
		time_t request_time;
		time_t response_time;
		time_t interval_time;
		int line_num;
		redisContext * conn;
#if MULTI_THREADS
		unsigned short thid;
		char * operation;
		char * response;
		unsigned short id;
		unsigned short userid;
		unsigned short cliport;
		unsigned short serport;
		unsigned long times;
		unsigned int userip;
		unsigned int desip;
		unsigned int policytime;
		char table[10];
		char srcmac[CSP_MAC_LEN];
		char desmac[CSP_MAC_LEN];
		AUDIT_ENSEMBLE_WEB_REL web;



	*/
}
int getfilename(char * fullpath,char *filename,char *filepath,char *dir,int len){
	int i=0,position=0,lastposition=0;
	int flen,j=0;
	int len1=0,len2=0;
	for(i=len-1;i>0;i--){
		if(fullpath[i]=='/'){
			j++;
			if(j==1){
				position=i;
				flen = len-position;
				if(flen < 0) return 0;
				len1  = flen-1;
				strncpy(filename,&fullpath[position+1],len1);
				len2 = len-len1;
				strncpy(filepath,fullpath,len2);
				lastposition = position;
			}
			if(j==2){
				position = i;
				flen = lastposition - position;
				if(flen < 0) return 0;
				len1=flen-1;
				strncpy(dir,&fullpath[position+1],len1);
				lastposition = position;
			}
		}

	}
	return 0;
}

unsigned int get_file_info(AUDIT_ENSEMBLE_REL *rel,char *fname,char *buf,int flag){
		unsigned int fsize =0;
		unsigned int readsize = 0;
		int Fd = 0;
	//	FILE *Fd=NULL;
		TEST_EFF_DECLARED
		TEST_FUN_EFF("GetFileSize",fsize = GetFileSize(fname);)
		if(fsize <=0){
			return 0;
		}
		if(fsize > REQ_RES_LEN){
			return 0;
		}
		Fd = open(fname,O_RDONLY);
	//	Fd = fopen(fname,"r");
		if(Fd < 0){ 
			perror("fname");
			return	0;
		}
		
		TEST_FUN_EFF("AuditRead",readsize = AuditRead(Fd,buf,fsize))
			
//		TEST_FUN_EFF("fread",readsize = fread(buf,1,fsize,Fd))
		close(Fd);
	//	fclose(Fd);	
		if(readsize != fsize){
			return 0;
		}

		if(flag == 2){
			rel->response_data = buf;
			rel->response_size = readsize;
		}
		else{
			rel->request_data = buf;
			rel->request_size = readsize;
		}
		return readsize;
}



int FileInit(char * request,char * response,AUDIT_ENSEMBLE_REL *rel){
//	char *data ;
	int ret1=0,ret2=0;
	TEST_EFF_DECLARED
TEST_FUN_EFF("2get_file_info",ret2 = get_file_info(rel,response,fileContent2[rel->thid],2);)
TEST_FUN_EFF("1get_file_info",ret1 = get_file_info(rel,request,fileContent1[rel->thid],1);)
	

	if(ret1==0 || ret2==0) {
		return -1;
	}

	return 0;
}




long GetEnsemblePayload(char *data,unsigned long  size,char *ensemble_payload,unsigned short * request_type,int flag,AUDIT_ENSEMBLE_REL * rel){
		long readLen=0;
		long len=0;
		long dataLen=0;
		AUDIT_ENSEMBLE_HEAD *head;
		int i=0;

		do{

			head = (AUDIT_ENSEMBLE_HEAD*)(data+readLen);
			len = head->plen;
			if(i==0&&flag ==1){
				*request_type=ntohs(head->opt_flg);

			}
			if(i == 0){
				first_len[rel->thid] = len;
			}

			if(len < 0 ||len > size){
				return -1;
			}
			memcpy(ensemble_payload+dataLen,(char *)head+14,len);
			dataLen += head->plen;
			readLen += (len +14);
			i++;
			if(readLen == 14){
				i=0;
			}
		}while(readLen < size);

		return dataLen;
}


unsigned int Change_encode_2(unsigned char *from,size_t fromLen,unsigned char *to,size_t *toLen){

	unsigned int i,j;
	i = 0;
	j = 0;

		while(j < fromLen){
			//from[j]!=0x00 ? (to[i++] = from[j++];):(j++);
			if(from[j]==0x00){
				j++;
				continue;
			}
			to[i] = from[j];
			j++;
			i++;
		}
		*toLen = i;
	return 1;
}


int code_convert(char * from_charset,char * to_charset,char  ** from_str, size_t  *from_len,char  ** to_str, size_t *to_len){
	 iconv_t   cd;
	 size_t unconvert_len;
	 unconvert_len = *to_len;
	 cd = iconv_open(to_charset,from_charset);
	  if   (cd==(iconv_t)-1)   return   -1;


	  if(iconv(cd,from_str,from_len,to_str,to_len)==-1) {
		perror("iconv");
		iconv_close(cd);
		return -1;
	  }
	  iconv_close(cd);

	  *to_len = unconvert_len - *to_len; 
	  return 0;

}

unsigned int Change_encode_3(char **from,size_t *fromLen, char **to, size_t *toLen){

	 char fromset[]="unicode";
	 char toset[]="gbk";

	if(code_convert(fromset,toset,from,fromLen,to,toLen)==-1){

		return 0;
	}


	return 1;
}
unsigned int Change_encode(int type,unsigned char *from_segment,size_t from_len,unsigned char *to_segment,size_t to_len){
	unsigned ret = 0;
	switch(type){
		case 0x01:
			memcpy(to_segment,from_segment,from_len);
			to_len = from_len;
			ret = 1;
			break;
		case 0x02:
			ret = Change_encode_3((char **)&from_segment,&from_len,(char **)&to_segment,&to_len);
			break;
		default:
			ret = Change_encode_2(from_segment, from_len,to_segment,&to_len);
			break;
	}
	if(ret == 1){
		return to_len;
	}

	return 0;
}

void GetSegmentValues(unsigned char *from,unsigned int fromLen,char * to,unsigned int * toLen){
	char type;
	unsigned int len;
	char zero[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

	*toLen = fromLen;
	len = *toLen;
	memcpy(to,from,len);
	memcpy(to+len,zero,8);
}

void DataReleaseSearchOptRequest(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL *rel){
	AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
	AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;

	unsigned char *start = NULL;
	unsigned short parmNum = 0;//参数个数
	unsigned short loopNum = 0;//循环次数
	int segCount = 0;//字段计数
	int segLen = 0;//字段长度
	size_t fromLen = 0;//字段内容长度
	size_t toLen  = 0;
	int count;
	long readLen = 0;
	long dataLen = 0;
	char segType;

	start = ensemblePayload;

	do{
		if(*start != 0x00){
			data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
			segLen = data_st->len;
			if(segLen == 1){
				fromLen = 0;
				readLen += segLen;
				start = ensemblePayload+readLen;
				continue;
			}
			else if(segLen > payloadLen){
				return ;
			}
			else if( segLen > 1	){
			//	data_type = data_st->type;
			}
			segType = data_st->type;
			readLen += segLen;
			fromLen = segLen -2;
			memcpy(from_segment[rel->thid],data_st->data,fromLen);
		}
		else{
            start++;
			data2_st = (AUDIT_ENSEMBLE_DATA2_ST *)start;
			segLen = data2_st->len;
            if(segLen > rel->request_size){
				return ;
			}
			segType = data2_st->type;
			readLen += (segLen +3);
			fromLen = segLen-1;
		//	printf("pthread %hu: fromLen = %u,readLen = %u",rel->thid,fromLen,readLen);

			memcpy(from_segment[rel->thid],data2_st->data,fromLen);

		}
		//memset(to_segment,0,REQ_RES_LEN);

		if(segCount == 1){ //sql query

			toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4);
	//		printf("dataLen = %d,toLen = %d\n",dataLen,toLen);	
			memcpy(rel->operation+dataLen,to_segment[rel->thid],toLen);
			dataLen+=toLen;
			rel->operation[dataLen]=' ';
			dataLen+=1;
			rel->operation_len = dataLen;
		}
		else if(segCount == 2){
			if(fromLen == 0){
				break;
			}
			else if(fromLen==1){
				parmNum = from_segment[rel->thid][0];
				loopNum = parmNum*2+3;
				count = segCount+loopNum;
				segCount++;
				start = ensemblePayload+readLen;
				continue;
			}
		}
		else if(segCount >= count){
			if(parmNum > 0){
				toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4);
				memcpy(rel->operation+dataLen,to_segment[rel->thid],toLen);
				dataLen+=toLen;
				rel->operation[dataLen]=' ';
				dataLen+=1;
				rel->operation_len= dataLen;
				parmNum--;
			}
		}
		if(loopNum > 0){
			loopNum--;
		}
		segCount++;
		start = ensemblePayload+readLen;
	}while(readLen < payloadLen);
	rel->sub_app_id = GetAppType(rel->operation);
	rel->saveflag = rel->sub_app_id+200;
}

void DataReleaseModifyOptRequest(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL *rel){
		AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
		AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;

		unsigned char *start = NULL;
		unsigned short parmNum = 0;//参数个数
		unsigned short loopNum = 0;//循环次数
		int segCount = 0;//字段计数
		int segLen = 0;//字段长度
		int fromLen = 0;//字段内容长度
		int toLen  = 0;
		int count;
		long readLen = 0;
		long dataLen = 0;
		char segType;
		start = ensemblePayload;

		do{
			if(*start != 0x00){
				data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
				segLen = data_st->len;
				if(segLen == 1){
					fromLen = 0;
					readLen += segLen;
					start = ensemblePayload+readLen;
					continue;
				}
				else if(segLen > payloadLen){
					return;
				}
				else if( segLen > 1 ){
				//	data_type = data_st->type;
				}
				segType = data_st->type;
				readLen += segLen;
				fromLen = segLen -2;
				memcpy(from_segment[rel->thid],data_st->data,fromLen);
			}
			else{
				data2_st = (AUDIT_ENSEMBLE_DATA2_ST *)(start+1);
				segLen = data2_st->len;
				if(segLen > rel->request_size){
					return;
				}
				segType = data2_st->type;
				readLen += (segLen +3);
				fromLen = segLen-1;
			//	printf("pthread %hu: fromLen = %u,readLen = %u",rel->thid,fromLen,readLen);

				memcpy(from_segment[rel->thid],data2_st->data,fromLen);

			}
			//	memset(to_segment,0,REQ_RES_LEN);
			if(segCount == 1){ //sql query
				toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4);
				memcpy(rel->operation+dataLen,to_segment[rel->thid],toLen);
				dataLen+=toLen;
				rel->operation[dataLen]=' ';
				dataLen+=1;
				rel->operation_len = dataLen;
			}
			else if(segCount == 2){
				if(fromLen == 0){
					break;
				}
				else if(fromLen==1){
					parmNum = from_segment[rel->thid][0];
					loopNum = parmNum*2+4;
					count = segCount+loopNum;
					segCount++;
					start = ensemblePayload+readLen;
					continue;
				}
			}
			else if(segCount > count){
				if(parmNum > 0){
					toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4);
					memcpy(rel->operation+dataLen,to_segment[rel->thid],toLen);
					dataLen+=toLen;
					rel->operation[dataLen]=' ';
					dataLen+=1;
					rel->operation_len= dataLen;
					parmNum--;
				}
			}
			if(loopNum > 0){
				loopNum--;
			}
			segCount++;
			start = ensemblePayload+readLen;
		}while(readLen < payloadLen);
			rel->sub_app_id = GetAppType(rel->operation);
			rel->saveflag = rel->sub_app_id+200;
}
void DataReleaseSearchOptRequestRp(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL *rel){

}
unsigned char * SearchSegmentStructRelease(unsigned char * segment,int count,long contentLen,unsigned char * segment_str,unsigned int *len,AUDIT_ENSEMBLE_REL * rel){
	int i=0;
	AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
	AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;
	unsigned char * start = NULL;
	unsigned int segLen = 0;
	unsigned int readLen = 0;
	unsigned int fromLen = 0;
	unsigned int toLen = 0;
	unsigned int dataLen=0;
	char segType;
	start = segment;
	do{
		if((*start)!=0x00){
			data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
			segLen = data_st->len;
			if(segLen == 1){
				fromLen = 0;
				readLen += segLen;
				start = segment+readLen;
				continue;
			}
			segType = data_st->type;
			fromLen = segLen -2;
			readLen += segLen;
			memcpy(from_segment[rel->thid],data_st->data,fromLen);
		}
		else{
			data2_st = (AUDIT_ENSEMBLE_DATA2_ST *)(start+1);
			segLen = data2_st->len;
			segType = data2_st->type;

			fromLen = segLen - 1;
			readLen += (segLen+3);
			
		//	printf("pthread %hu: fromLen = %u,readLen = %u",rel->thid,fromLen,readLen);
			memcpy(from_segment[rel->thid],data2_st->data,fromLen);
		}
	//	memset(to_segment,0,REQ_RES_LEN);
		switch(i%9){

			case 5:
				toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],REQ_RES_LEN);
				memcpy(segment_str+dataLen,to_segment[rel->thid],toLen);
				dataLen += toLen;
				segment_str[dataLen]='.';
				dataLen+=1;
				break;
			case 6:
				toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],REQ_RES_LEN);
				memcpy(segment_str+dataLen,to_segment[rel->thid],toLen);
				dataLen += toLen;
				segment_str[dataLen]='.';
				dataLen+=1;
				break;
			case 7:
				toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],REQ_RES_LEN);
				memcpy(segment_str+dataLen,to_segment[rel->thid],toLen);
				dataLen += toLen;
				strncpy(&segment_str[dataLen],SEP_FLAG,3);
				dataLen+=3;
				segment_str[dataLen]='\0';
				break;
			case 8:
				start = segment + readLen;//case i+=1;
				if((*start)!=0x00){
					data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
					segLen = data_st->len;
					fromLen = segLen -2;
					//readLen += segLen;
					memcpy(from_segment[rel->thid],data_st->data,fromLen);
					from_segment[rel->thid][fromLen]='\0';
					if(strlen(from_segment[rel->thid]) < fromLen){
						readLen+=segLen;
					//	start = segment + readLen;
					}
				}
				else{
					data2_st = (AUDIT_ENSEMBLE_DATA2_ST *)(start+1);
					segLen = data2_st->len;
					segType = data2_st->type;

					fromLen = segLen - 1;
					
					memcpy(from_segment[rel->thid],data2_st->data,fromLen);
					from_segment[rel->thid][fromLen]='\0';
					if(strlen(from_segment[rel->thid]) < fromLen){
						readLen+=segLen;
					//	start = segment + readLen;
					}
				}
				break;
			default:
				break;

		}

		start = segment + readLen;
		if(i/9 == count){
			readLen < first_len[rel->thid];
			start = segment+first_len[rel->thid];
			break;
		}
		i++;

	}while(readLen < contentLen);

	*len = dataLen;
	return start;
}

unsigned char * SearchSegmentContentRelease(unsigned char * segment,unsigned int count,long contentLen,unsigned char * segment_str,unsigned int *len,AUDIT_ENSEMBLE_REL * rel){
	int i=1;
	AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
	AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;
	unsigned char * start = NULL;
	unsigned int segLen = 0;
	unsigned int readLen = 0;
	unsigned int fromLen = 0;
	unsigned int toLen = 0;
	unsigned int dataLen=0;
	char type;
	void * dataType;
	char tmp[1000]={0};
	unsigned int tmpLen=0;
	start = segment;

	do{

		if((*start)!=0x00){
			data_st = (AUDIT_ENSEMBLE_DATA_ST*)start;
			segLen = data_st->len;
			if(segLen == 1){
				readLen += segLen;
				start = segment+readLen;
				i++;
				continue;
			}
			readLen += segLen;
			fromLen = segLen -2;
			memcpy(from_segment[rel->thid],data_st->data,fromLen);
			type = data_st->type;
		}
		else{
			data2_st = (AUDIT_ENSEMBLE_DATA2_ST*)(start+1);
			segLen = data2_st->len;
			fromLen = segLen -1;
			readLen += (segLen+3);
		//	printf("pthread %hu: fromLen = %u,readLen = %u",rel->thid,fromLen,readLen);
			memcpy(from_segment[rel->thid],data2_st->data,fromLen);
			type = data2_st->type;
		}
		GetSegmentValues(from_segment[rel->thid],fromLen,to_segment[rel->thid],&toLen);
		dataType = (void *)to_segment[rel->thid];
	//	memset(tmp,0,sizeof(tmp));
		switch(type){
			case 0x01:
				//字符串
				if(fromLen > 0){
					if(fromLen == 1 && to_segment[rel->thid][0] == 0x00){
						sprintf(tmp,"NULL%s",SEP_FLAG);
					}
					else{
					sprintf(tmp,"%s%s",(char *)dataType,SEP_FLAG);
					}
				}
				else{
					sprintf(tmp,"NULL%s",SEP_FLAG);
				}
				
				tmpLen = strlen(tmp);
				break;
			case 0x02:
			//	memset(to_segment,0,REQ_RES_LEN);
				toLen = Change_encode(type,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4);
				memcpy(tmp,(char *)dataType,toLen);
				memcpy(tmp+toLen,SEP_FLAG,3);
				//sprintf(tmp,"%s%s",(char *)dataType,SEP_FLAG);
				tmpLen = toLen + 3;
				break;
			case 0x04:
				if(toLen > 4){

					sprintf(tmp,"%llu%s",*((unsigned long * )dataType),SEP_FLAG);
				}
				else {

					sprintf(tmp,"%lu%s",*((unsigned int * )dataType),SEP_FLAG);
				}
				tmpLen = strlen(tmp);
				break;
			case 0x08:
				sprintf(tmp,"%f%s",*((double *)dataType),SEP_FLAG);		
				tmpLen = strlen(tmp);
				break;
			default:
				tmpLen = strlen(tmp);
				break;
		}


		memcpy(segment_str+(*len),tmp,tmpLen);
		*len += tmpLen;
		if(i%count==0){
			//换行
			rel->line_num++;
			strncpy(segment_str+(*len),ENTER_FLAG,3);
			*len += 3;
			segment_str[*len] = '\0';
		}
		i++;
		start = segment+readLen;
	}while(readLen < contentLen);

	return start;
}
void DataReleaseSearchOptResponse(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL * rel){
	AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
	AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;

	unsigned char *start = NULL;
	unsigned short parmNum = 0;//参数个数
	unsigned short loopNum = 0;//循环次数
	int segCount = 0;//字段计数
	unsigned int segNum = 0;//表字段个数
	int segLen = 0;//字段长度
	int segSegCount = 0;
	int fromLen = 0;//字段内容长度
	int toLen  = 0;
	int count;
	long readLen = 0;
	long dataLen = 0;
	unsigned int len = 0;
	char tmp_segLen[3]={0};

	const int segments_num = 10;

	unsigned int segment_str_len=0;
	start = ensemblePayload;

	data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
	segLen = data_st->len;
	first_len[rel->thid] -= segLen;
	if(segLen == 3){
		segNum = start[2];
	}
	else if(segLen == 4){
		tmp_segLen[0]=start[2];
		tmp_segLen[1]=start[3];
		tmp_segLen[2]='\0';
		segNum = *((unsigned short *)(void *)&tmp_segLen);
	}
	else
		return ;
	readLen += segLen;
	start += readLen;

	segCount++;

	start = SearchSegmentStructRelease(start,segNum,payloadLen-readLen,segment_str[rel->thid],&len,rel);

	memcpy(&(segment_str[rel->thid][len]),ENTER_FLAG,3);
	len += 3;
	memcpy(rel->response+rel->response_len,segment_str[rel->thid],len);
	rel->response_len += len;
	len =0;
	if(start!=NULL && (start - ensemblePayload >= payloadLen)){

		return ;
	}

	readLen = (start - ensemblePayload);
//	data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;

//	start+=data_st->len;
//	readLen += data_st->len;
	start = SearchSegmentContentRelease(start,segNum,payloadLen-readLen,segment_str[rel->thid],&len,rel);

	memcpy(rel->response+rel->response_len,segment_str[rel->thid],len);
 	rel->response_len += len;

}
void DataReleaseSearchOptResponseRp(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL * rel){
	rel->response[0]=0x0a;
	rel->response_len = 1;
}

void DataReleaseModifyOptResponse(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL * rel){
	rel->response[0]=0x0a;
	rel->response_len = 1;
}

void DataReleaseStudioOptRquest(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL * rel){
	AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
	AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;

	unsigned char *start = NULL;
	unsigned short parmNum = 0;//参数个数
	unsigned short loopNum = 0;//循环次数
	int segCount = 0;//字段计数
	int segLen = 0;//字段长度
	int fromLen = 0;//字段内容长度
	int toLen  = 0;
	int count;
	long readLen = 0;
	long dataLen = 0;
	char segType;
	
	TEST_EFF_DECLARED
	rel->operation_len = 0;
	start = ensemblePayload;
	do{
		if(*start != 0x00){
			data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
			segLen = data_st->len;

			if(segLen == 1){
				fromLen = 0;
				readLen += segLen;
				start = ensemblePayload+readLen;
				continue;
			}
			else if(segLen > payloadLen){
				return ;
			}
			else if( segLen > 1 ){
			//	data_type = data_st->type;
			}
			segType = data_st->type;
			readLen += segLen;
			fromLen = segLen -2;
			if(segType > 2){
				start = ensemblePayload+readLen;
				continue;
			}
			memcpy(from_segment[rel->thid],data_st->data,fromLen);

		}
		else{
			data2_st = (AUDIT_ENSEMBLE_DATA2_ST *)(start+1);
			segLen = data2_st->len;

			if(segLen > payloadLen){
				return ;
			}
			segType = data2_st->type;
			readLen += (segLen +3);
			fromLen = segLen-1;
			if(segType > 2){
				start = ensemblePayload+readLen;
				continue;
			}
			TEST_FUN_EFF("1memcpy",memcpy(from_segment[rel->thid],data2_st->data,fromLen);)

		}
		//TEST_FUN_EFF("memset",memset(to_segment,0,REQ_RES_LEN);)


		if(fromLen > 1){
			TEST_FUN_EFF("Change_encode",toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4))
			TEST_FUN_EFF("2memcpy",memcpy(rel->operation+rel->operation_len,to_segment[rel->thid],toLen);)
			dataLen += toLen;
			rel->operation[dataLen] = ' ';
			dataLen += 1;
			rel->operation_len = dataLen;
		}

		start = ensemblePayload+readLen;
		continue;

	}while(readLen < payloadLen);

	rel->sub_app_id = GetOperationType(rel->operation);
	rel->saveflag = rel->sub_app_id;
}

void DataReleaseStudioOptResponse(unsigned char * ensemblePayload,long payloadLen,AUDIT_ENSEMBLE_REL * rel){
//	DataReleaseStudioOptRquest(ensemblePayload,payloadLen,rel);
//	rel->response_len = rel->operation_len;
//	memcpy(rel->response,rel->operation,rel->response_len);
	AUDIT_ENSEMBLE_DATA_ST * data_st=NULL;
	AUDIT_ENSEMBLE_DATA2_ST * data2_st=NULL;

	unsigned char *start = NULL;
	unsigned short parmNum = 0;//参数个数
	unsigned short loopNum = 0;//循环次数
	int segCount = 0;//字段计数
	int segLen = 0;//字段长度
	int fromLen = 0;//字段内容长度
	int toLen  = 0;
	int count;
	long readLen = 0;
	long dataLen = 0;
	char segType;
	
	TEST_EFF_DECLARED
	rel->response_len = 0;
	start = ensemblePayload;
	do{
		if(*start != 0x00){
			data_st = (AUDIT_ENSEMBLE_DATA_ST *)start;
			segLen = data_st->len;

			if(segLen == 1){
				fromLen = 0;
				readLen += segLen;
				start = ensemblePayload+readLen;
				continue;
			}
			else if(segLen > payloadLen){
				return ;
			}
			else if( segLen > 1 ){
			//	data_type = data_st->type;
			}
			segType = data_st->type;
			readLen += segLen;
			fromLen = segLen -2;
			if(segType > 2){
				start = ensemblePayload+readLen;
				continue;
			}
			memcpy(from_segment,data_st->data,fromLen);

		}
		else{
			data2_st = (AUDIT_ENSEMBLE_DATA2_ST *)(start+1);
			segLen = data2_st->len;

			if(segLen > payloadLen){
				return ;
			}
			segType = data2_st->type;
			readLen += (segLen +3);
			fromLen = segLen-1;
			if(segType > 2){
				start = ensemblePayload+readLen;
				continue;
			}
			TEST_FUN_EFF("1memcpy",memcpy(from_segment,data2_st->data,fromLen);)

		}
		//TEST_FUN_EFF("memset",memset(to_segment,0,REQ_RES_LEN);)


		if(fromLen > 1){
			TEST_FUN_EFF("Change_encode",toLen = Change_encode(segType,from_segment[rel->thid],fromLen,to_segment[rel->thid],SEG_LEN*4))
			TEST_FUN_EFF("2memcpy",memcpy(rel->response+rel->response_len,to_segment[rel->thid],toLen);)
			dataLen += toLen;
			rel->response[dataLen] = ' ';
			dataLen += 1;
			rel->response_len = dataLen;
		}

		start = ensemblePayload+readLen;
		continue;

	}while(readLen < payloadLen);

//  rel->sub_app_id = GetOperationType(rel->operation);
//	rel->saveflag = rel->sub_app_id;
}

long sqldbx_studio_release(AUDIT_ENSEMBLE_REL *rel,int flag){
	long toLen = 0;
	long content_len = 0;
	unsigned long size;
	int apptype;
	char *data=NULL, *start=NULL;

	
	start = ensemble_payload[rel->thid];
	if(flag == 1){//request
		data = rel->request_data;
		size = rel->request_size;
		content_len = GetEnsemblePayload(data,size,ensemble_payload[rel->thid],&rel->request_type,flag,rel);
		if(content_len <= 0) return -1;
		switch(rel->request_type){
			case SQL_SEARCH_OPT:
				DataReleaseSearchOptRequest(ensemble_payload[rel->thid],content_len,rel);
				if(rel->saveflag == 200)return -1;
				break;
			case SQL_SEARCH_OPT_RP:
				return -1;
				DataReleaseSearchOptRequestRp(ensemble_payload[rel->thid],content_len,rel);
				break;
			case SQL_MODIFY_OPT:
				DataReleaseModifyOptRequest(ensemble_payload[rel->thid],content_len,rel);
				if(rel->saveflag == 200)return -1;
				break;
			case SQL_MODIFY_OPT_RP:
				return -1;
				break;
			case STUDIO_OPT:
			case STUDIO_SAVEFILE_OPT:
				DataReleaseStudioOptRquest(ensemble_payload[rel->thid],content_len,rel);	
				if(rel->saveflag == 0) return -1;
				break;
			default:
				return -1;
		}
	}

	if(flag == 2){//response
		data = rel->response_data;
		size = rel->response_size;
		content_len = GetEnsemblePayload(data,size,ensemble_payload[rel->thid],&rel->request_type,flag,rel);
		if(content_len < 0) return -1;
		if(content_len == 0){
			rel->response_len=0;
			return 0;
		}
		switch(rel->request_type){
			case SQL_SEARCH_OPT:
				DataReleaseSearchOptResponse(ensemble_payload[rel->thid],content_len,rel);
				break;
			case SQL_SEARCH_OPT_RP:
				DataReleaseSearchOptResponseRp(ensemble_payload[rel->thid],content_len,rel);
				break;
			case SQL_MODIFY_OPT:
				DataReleaseModifyOptResponse(ensemble_payload[rel->thid],content_len,rel);
				break;
			case SQL_MODIFY_OPT_RP:
				return -1;
				break;
			case STUDIO_OPT:
			case STUDIO_SAVEFILE_OPT:
				DataReleaseStudioOptResponse(ensemble_payload[rel->thid],content_len,rel);
				break;
			default:
				return -1;
				break;
		}
	}
	return 0;
}



int ensemble_sql(char *sql_file_path,AUDIT_ENSEMBLE_REL *rel,int flag){
	int fd;
	int len = 0;
	int rr;
	unsigned int sip,dip;
	char userip[16]={0},desip[16]={0};
//	printf("sql_file_path:%s\n",sql_file_path);
//	printf("flag = %d\n",flag);
	if(rel->operation_len == 0){
		return -1;
	}
	sip = ntohl(rel->userip);
	dip = ntohl(rel->desip);
	
	inet_ntop(AF_INET,(void*)(&sip),userip,sizeof(userip));
	inet_ntop(AF_INET,(void*)(&dip),desip,sizeof(desip));
	
	if(flag == 1)
		fd = open(sql_file_path,O_RDWR | O_CREAT, 0666);
	if(flag == 2)
		fd = open(sql_file_path,O_RDWR|O_APPEND, 0666);
	if(fd < 0) {
		printf("fd < 2; %d\n",fd);
		perror("open");
		return -1;
	}
//	memset(from_segment,0,sizeof(from_segment));
	if(flag == 1 ){
		sprintf(from_segment[rel->thid],MONGO_ENSEMBLE_REQUEST_VALUES,MONGO_ENSEMBLE_REQUEST_PARAMS);
		len = strlen(from_segment[rel->thid]);
		memcpy(&(from_segment[rel->thid][len]),rel->operation,rel->operation_len);
		len += rel->operation_len;
		from_segment[rel->thid][len] = '\n';
		len += 1;
	}
	if(flag == 2 ){
		sprintf(from_segment[rel->thid],MONGO_ENSEMBLE_RESPONSE_VALUES,MONGO_ENSEMBLE_RESPONSE_PARAMS);
		len = strlen(from_segment[rel->thid]);
		memcpy(&(from_segment[rel->thid][len]),rel->response,rel->response_len);
		len += rel->response_len;
		from_segment[rel->thid][len] = '\n';
		len += 1;
	}
	rr = AuditWrite(fd,from_segment[rel->thid],len);
	close(fd);
//	rename(sql_file_path,sql_path);
//	printf("written:%d\n",rr);
	return rr;
}
/*
void ensemble_packet_parse(char * request_path,char *response_path,char *sql_file_path,char * sql_path,AUDIT_ENSEMBLE_REL *rel,int flag){
	char request_cmd[500]={0};
	char response_cmd[500]={0};
	int isMedTrak;

	memset(request_cmd_debug,0,500);
	memset(response_cmd_debug,0,500);

//	long rel_flag=0;
	sprintf(request_cmd,"mv %s %s",request_path,SQL_BACKUP);
	sprintf(response_cmd,"mv %s %s",response_path,SQL_BACKUP);

	sprintf(request_cmd_debug,"mv %s %s",request_path,SQL_DEBUG);
	sprintf(response_cmd_debug,"mv %s %s",response_path,SQL_DEBUG);


	if(ensembleFileInit(request_path,response_path,rel) == -1){
		return ;
	}


	rel->request_time = GetFileTime(request_path);
	rel->response_time = GetFileTime(response_path);
	if(rel->request_time == 0 | rel->response_time == 0){
		rel->interval_time=0;
	}
	else{
		rel->interval_time = rel->response_time - rel->request_time;
	}

		// test if med or others
		isMedTrak = medtrakTest(rel->request_data,rel->request_size);
	//	printf("isMedTrak : %d\n",isMedTrak);
		if(isMedTrak){
				rel->saveflag = 300;
				if(medtrakParser(rel->data, rel->size,rel->operation, &rel->operation_len) == -1){
					return ;
				}
				if(ensemble_sql(sql_file_path,sql_path,rel,1) == -1){
					#if RUN_FLAG
							system(response_cmd);
							system(request_cmd);
					#else
							unlink(response_path);
							unlink(request_path);
					#endif

					return ;
				}

				if(flag == 1){
					if(FileInit(response_path,rel) == -1){
#if RUN_FLAG
						system(response_cmd);
						system(request_cmd);
#else
						unlink(response_path);
						unlink(request_path);
#endif

						rename(sql_file_path,sql_path);
						return ;
					}
					if(medtrakParser(rel->data, rel->size,rel->response, &rel->response_len)==-1){
#if RUN_FLAG
						system(response_cmd);
						system(request_cmd);
#else
						unlink(response_path);
						unlink(request_path);
#endif

						rename(sql_file_path,sql_path);
						return ;
					}
					if(ensemble_sql(sql_file_path,sql_path,rel,2) == -1){
						#if RUN_FLAG
								system(response_cmd);
								system(request_cmd);
						#else
								unlink(response_path);
								unlink(request_path);
						#endif

								rename(sql_file_path,sql_path);
						return ;
					}

				}
		}
		else if(isMedTrak == 0){
						if( FileRelease(rel,1) == -1 ){
#if RUN_FLAG
							system(response_cmd);
							system(request_cmd);
#else
							unlink(response_path);
							unlink(request_path);
#endif

							return ;
						}
						if(ensemble_sql(sql_file_path,sql_path,rel,1) == -1){
	#if RUN_FLAG
									system(response_cmd);
									system(request_cmd);
	#else
									unlink(response_path);
									unlink(request_path);
	#endif

							return ;
						}

				if(flag == 1){
					if(FileInit(response_path,rel) == -1){
#if RUN_FLAG
								system(response_cmd);
								system(request_cmd);
#else
								unlink(response_path);
								unlink(request_path);
#endif

						rename(sql_file_path,sql_path);
						return ;
					}




					if(FileRelease(rel,2)==-1){
#if RUN_FLAG
								system(response_cmd);
								system(request_cmd);
#else
								unlink(response_path);
								unlink(request_path);
#endif

						rename(sql_file_path,sql_path);
						return ;
					}
					if(ensemble_sql(sql_file_path,sql_path,rel,2) == -1){
#if RUN_FLAG
								system(response_cmd);
								system(request_cmd);
#else
								unlink(response_path);
								unlink(request_path);
#endif

						rename(sql_file_path,sql_path);
						return ;
					}
				}
		}
		else{
			#if RUN_FLAG
			system(response_cmd);
			system(request_cmd);
#else
			unlink(response_path);
			unlink(request_path);
#endif
		}


	rename(sql_file_path,sql_path);
#if RUN_FLAG
			system(response_cmd);
			system(request_cmd);
#else
			unlink(response_path);
			unlink(request_path);
#endif

}

*/



int check_cache_type(char * request_file){
			unsigned long fsize =0;
			unsigned int readsize = 0;
			char buf[40]={0};
			char request_url[5120]={0};
			char *start,*end,*pos;
			int len;
			
			FILE * Fp = NULL;

			Fp = fopen(request_file,"r+");
			if(!Fp){ 
		#if CSP_DEBUG
				printf("open %s file failed. \n",fname);
		#endif
				return	-1;
			}
			if(fgets(buf,30,Fp) == NULL ){
				fclose(Fp);
				return -1;
			}
			fclose(Fp);	

			fsize =  GetFileSize(request_file);
			if(fsize>28&&*buf==0x00&&*(buf+1)==0x00&&*(buf+2)==0x00&&*(buf+3)==0x00){
				pos = buf+14;
				if(*(pos+12)==0x42 && *(pos+13)==0x49 ){
					return 1;
				}
				if(*(pos+12)==0x42 && *(pos+13)==0x37 ){
					return 1;
				}
				if(*(pos+12)==0x44 && *(pos+13)==0x51 ){
					return 1;
				}
				if(*(pos+12)==0x50 && *(pos+13)==0x51 ){
					return 1;
				}
				if(*(pos+12)==0x44 && *(pos+13)==0x55 ){
					return 1;
				}
				if(*(pos+12)==0x50 && *(pos+13)==0x55 ){
					return 1;
				}
			}

			
			if( *(buf+7)==0x00 && *(buf+8)==0x00 && *(buf+9)==0x00 && *(buf+10)==0x00 && *(buf+11)==0x00 ){
				return 1;
			}
			else{
				if(*(buf+12)==0x42 && *(buf+13)==0x49 ){
					return 1;
				}
				if(*(buf+12)==0x42 && *(buf+13)==0x37 ){
					return 1;
				}

				if(*(buf+12)==0x44 && *(buf+13)==0x51 ){
					return 1;
				}

				if(*(buf+12)==0x50 && *(buf+13)==0x51 ){
					return 1;
				}
				if(*(buf+12)==0x44 && *(buf+13)==0x55 ){
					return 1;
				}
				if(*(buf+12)==0x50 && *(buf+13)==0x55 ){
					return 1;
				}
				return -1;
			}
}

int check_request_type(char * request_file){
			unsigned int fsize =0;
			unsigned int readsize = 0;
			char buf[1024]={0};
			char request_url[5120]={0};
			char *start=NULL,*end=NULL,*pos=NULL;
			int Fp=0;
			int len=0;
		//	FILE * Fp = NULL;
		
			fsize = GetFileSize(request_file); 
			if(fsize <=0){
				#if 0
				printf("fsize < 0 ,%s\n",request_file);
				#endif
				return -1;
			}

		//	Fp = fopen(fname,"r+");
			Fp = open(request_file,O_RDONLY);
			if(Fp<0){ 
		#if 0
				perror("open:");
				printf("open %s file failed. \n",request_file);
		#endif
				return	-1;
			}
			if(fsize > 999){
				len = 999;
			}else{
				len = fsize;
			}
			AuditRead(Fp,buf,len);
	//		readsize = fread(buf,1,fsize,Fp);
			close(Fp);
			//fclose(Fp);
			
			start = buf;
			if((end = strstr(start," HTTP/1.1\r\n"))){
				len = end -start;
				if( len  < 0){
					return -1;
				}
				if(len < 5119){
					memcpy(request_url,start,len);
				}
				else{
					len = 5119;
					memcpy(request_url,start,len);
				}
			}
			else return -1;
	//		printf("requesturl:%s\n",request_url);
			if(strncmp(request_url,CONGOUS_URL_PASSPORT,strlen(CONGOUS_URL_PASSPORT))==0){
				return -1;
			}

			if((pos = strstr(request_url,"?"))){
				*pos = '\0';
			}


			if(strcasestr(request_url,".gif")){
				return -1;
			}
			if(strcasestr(request_url,".png")){
				return -1;
			}
			if(strcasestr(request_url,".jpg")){
				return -1;
			}
			if(strcasestr(request_url,".js")){
				return -1;
			}
			if(strcasestr(request_url,".jsp")){
				return -1;
			}
			if(strcasestr(request_url,".css")){
				return -1;
			}
			if(strcasestr(request_url,".dll")){
				return -1;
			}
			if(strcasestr(request_url,".exe")){
				return -1;
			}
			if(strstr(request_url,"dhcvisanopwait")){
				return -1;
			}
			if(strstr(request_url,"dhcvispatwait")){
				return -1;
			}

			if(strstr(request_url,"dhcrisappbill")){
				return -1;
			}

			if(strstr(request_url,"dhcnuripexeclist")){
				return -1;
			}
			if(strstr(request_url,"dhc.epr.messagetab.csp")){
				return -1;
			}
			if(strstr(request_url,"dhc.bdp.ext.sys.csp")){
				return -1;
			}	
//			if(!strstr(request_url,".csp") &&!strstr(request_url,".cls")){
//				return -1;
//			}
	return 1;
}

/*
int BindCpu(int toBind){
	cpu_set_t mask;
	CPU_ZERO(&mask);    //置空 
	#if U2_DEV
		if(toBind >= 3){
			toBind++;
		}
	#endif
   	CPU_SET(toBind,&mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//设置线程CPU亲和力  
    {  
//       printf("warning: could not set CPU affinity!\n");  
	   return -1;
    } 
//	printf("set CPU affinity success!\n");
	return 0;
}
*/


#if MULTI_THREADS




void * ensemble_process(AUDIT_ENSEMBLE_REL * rel){
	int argc = 2;
	//return 0;

	char tmp_sql_ensemble_file[PATH_LEN]={0};
	char sql_ensemble_file[PATH_LEN]={0};

	int isMedTrak = 0;
	int ret=0;


	char *pos;
	int flag=0;

		



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////










//	signal(SIGINT,SigFun);
//	signal(SIGTERM,SigFun);
//    signal(SIGSEGV,SigFun);

/*
		int exist;
		if((exist=access(file_response,0))!=0){
			flag = 0;
		}
		else{
			flag = 1;
		}
*/
	TEST_EFF_DECLARED

#if MULTI_THREADS
sprintf(tmp_sql_ensemble_file,"/data/audit/sql_tmp/Sql_studio_%lu_%hu",rel->times,rel->dir);
sprintf(sql_ensemble_file,"/data/audit/sql/Sql_studio_%lu_%hu",rel->times,rel->dir);
#else
sprintf(tmp_sql_ensemble_file,"/data/audit/sql_tmp/Sql_studio_%s_%hu",rel->times,rel->dir);
sprintf(sql_ensemble_file,"/data/audit/sql/Sql_studio_%s_%hu",rel->times,rel->dir);
#endif

		//process files
/*		TEST_FUN_EFF("ensembleFileInit",ret = ensembleFileInit(file_request,file_response,rel))
		if(ret == -1){
			if(argc == 2){
				unlink(file_response);
				unlink(file_request);
			}
			
			//printf("return 4\n");
			return ;
		}
		rel->request_time = GetFileTime(file_request);
		rel->response_time = GetFileTime(file_response);
		
		if(argc == 2){
			unlink(file_response);
			unlink(file_request);
		}

		if((rel->request_time  | rel->response_time) == 0){
				rel->interval_time=0;
		}
		else{
			rel->interval_time = rel->response_time - rel->request_time;
		}
*/
		// test if med or others
		TEST_FUN_EFF("medtrakTest",isMedTrak = medtrakTest(rel->request_data,rel->request_size))
		if(isMedTrak == 1){
			rel->saveflag = 300;
			TEST_FUN_EFF("1isMedTrak medtrakParser",ret = medtrakParser(rel->request_data, rel->request_size,rel->operation, &rel->operation_len))
			if(ret == -1){
				
				//printf("return 5\n");
				return NULL;
			}
			TEST_FUN_EFF("1isMedTrak ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,1))
			if(ret == -1){	
				
				//printf("return 6\n");
				return NULL;
			}
			TEST_FUN_EFF("2isMedTrak medtrakParser",ret = medtrakParser(rel->response_data, rel->response_size,rel->response, &rel->response_len))
			if(ret == -1){

				//printf("return 7\n");
				unlink(tmp_sql_ensemble_file);
				return NULL;
			}
			
			TEST_FUN_EFF("2isMedTrak ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,2))
			if(ret == -1){	
				
				//printf("return 8\n");
				unlink(tmp_sql_ensemble_file);
				return NULL;
			}
		}
		else{
			TEST_FUN_EFF("1sqldbx_studio_release",ret = sqldbx_studio_release(rel,1));
			if(ret==-1){
				
				//printf("return 9\n");
				return NULL;
			}
			
			TEST_FUN_EFF("1ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,1))
			if(ret== -1){
				
				//printf("return 10\n");
				return NULL;
			}
			
			TEST_FUN_EFF("2sqldbx_studio_release",ret = sqldbx_studio_release(rel,2));
			if(ret ==-1){
				
				//printf("return 11\n");
				unlink(tmp_sql_ensemble_file);
				return NULL;
			}
			
			TEST_FUN_EFF("2ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,2))
			if(ret == -1){
				
				//printf("return 12\n");
				unlink(tmp_sql_ensemble_file);
				return NULL;
			}
		}

		if(argc == 2)
			rename(tmp_sql_ensemble_file,sql_ensemble_file);

		
		//printf("return 100\n");
		return NULL;

}

void * myprocess(char * fname,AUDIT_MULTI_THREAD_PARM * arg){
		char file_request[PATH_LEN]={0};
		char file_response[PATH_LEN]={0}; 
		char *pos;
		int ret;
		unsigned short thid = arg->mp->thread_id;
		AUDIT_ENSEMBLE_REL * rel = arg->mp->rel;
		rel->conn = arg->mp->conn;
		rel->thid = thid;
		CACHE_POLICY_CONF * policy;
		unsigned long fsize;
		static unsigned long count=0;
		static unsigned long ensemble_count=0,web_count=0;
		TEST_EFF_DECLARED
		sprintf(file_request,"%s",fname);
	//	printf("fname:%s\n",fname);
		//memset(file_response,0,PATH_LEN);
	//	printf("myprocess thread %u thid %u\n",pthread_self (),thid);
		pos = strstr(file_request,"_request");
		*pos = '\0';
		sprintf(file_response,"%s_response",file_request);
		
		*pos = '_';
		
		fsize = GetFileSize(file_response);

	

	//	memset(rel,0,sizeof(*rel));

		TEST_FUN_EFF("GetValues",GetValues(rel,file_request))
		policy = arg->policy;
		if(rel->type == 30){
			if(fsize < 600){
				unlink(file_response);
				unlink(file_request);
				return NULL;
			}
			rel->type =1;
			TEST_FUN_EFF("policy_match_ensemble",ret = policy_match_ensemble(rel,policy))
			rel->type =30;
		}
		else if(rel->type == 10){
			ret=1;
		}
		else if(rel->type == 1||rel->type == 4){
			if(fsize < 100){
		//		printf("fsize < 100 appid = %hu count = %lu\n",rel->type,count++);
				
				unlink(file_response);
				unlink(file_request);
				return NULL;
			}
			if(check_request_type(file_request) == -1){
				unlink(file_response);
				unlink(file_request);		
				return NULL;		
			}		
			TEST_FUN_EFF("policy_match_ensemble",ret = policy_match_ensemble(rel,policy))
		}
		else{
			if(fsize < 1000){
				unlink(file_response);
				unlink(file_request);
				return NULL;
			}
			if(check_cache_type(file_request) == -1){
				unlink(file_response);
				unlink(file_request);	
				return NULL;
			}
			TEST_FUN_EFF("policy_match_ensemble",ret = policy_match_ensemble(rel,policy))
		}
		if( ret == 0){
			//printf("%u thread policy not match.\n",rel->thid);
			unlink(file_response);
			unlink(file_request);
			
			//printf("return 3\n");
			return NULL;
		}

		TEST_FUN_EFF("ensembleFileInit",ret = FileInit(file_request,file_response,rel))
		if(ret == -1){
				unlink(file_response);
				unlink(file_request);
			//printf("return 4\n");
			return NULL;
		}
		rel->request_time = GetFileTime(file_request);
		rel->response_time = GetFileTime(file_response);
		
		unlink(file_response);
		unlink(file_request);
	

		if(rel->request_time==0 || rel->response_time == 0){
				rel->interval_time=0;
		}
		else{
			rel->interval_time = rel->response_time - rel->request_time;
		}

		
		switch(rel->type){
			case 2:
				ensemble_process(rel);
	//			ensemble_count++;
				break;
			case 1:
			case 4:
			case 30:
				web_process(rel);
//				web_count++;
				break;
			case 10:
	//			sql_server(rel,policy);
				break;
			default:
				break;
		}
	//	printf("ensemble_procee run %lu times.\n",ensemble_count);
	//	printf("web_procee run %lu times.\n",web_count);
	

}


#else
int main(int argc,char ** argv){

	//return 0;
	

	char tmp_sql_ensemble_file[PATH_LEN]={0};
	char sql_ensemble_file[PATH_LEN]={0};

	char strdname[CSP_FILENAME_LEN]={0};
	char yyyymm_time[CSP_TIMES_LEN]={0};
	char day_time[CSP_TIMES_LEN]={0};
	int pathLen=0;
	int isMedTrak = 0;
	int ret=0;


	char *pos;
	int flag=0;
	unsigned int policyNum,j;
	unsigned long i =0;
	AUDIT_ENSEMBLE_HEAD *ensembleHead;
	AUDIT_ENSEMBLE_REL * rel = &ensembleRel;

	CACHE_POLICY_CONF * policy=NULL;
	int  httpResponseFd;
	int  ismatch;
	TEST_EFF_DECLARED
		
//	TEST_FUN_EFF("filecontent1",unsigned char fileContent1[REQ_RES_LEN]={0})
//	TEST_FUN_EFF("filecontent2",unsigned char fileContent2[REQ_RES_LEN]={0})
	//TEST_FUN_EFF("1memset",memset(fileContent1,0,REQ_RES_LEN);)
	//TEST_FUN_EFF("2memset",memset(fileContent2,0,REQ_RES_LEN);)



	sprintf(file_request,"%s",argv[1]);
	//memset(file_response,0,PATH_LEN);
	pos = strstr(file_request,"_request");
	*pos = '\0';
	sprintf(file_response,"%s_response",file_request);
	
	*pos = '_';
	/*
	if(GetFileSize(file_response)<=1000){
		unlink(file_response);
		unlink(file_request);
		return 0;
	}
*/
	if(check_cache_type(file_request) == -1){
		unlink(file_response);
		unlink(file_request);		
		
		//printf("return 2\n");
		return 0;
	}
	memset(rel,0,sizeof(*rel));
	TEST_FUN_EFF("GetValues",GetValues(rel,file_request))
	//bind cpu
	#if U2_DEV
	BindCpu(rel->dir%7);
	#else
	BindCpu(rel->dir%3);
	#endif
	//policy match
	policy = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm();
	if(!policy){
		//printf("return 1\n");
		return 0;
	}
	TEST_FUN_EFF("policy_match_ensemble",ret = policy_match_ensemble(rel,policy))
	if( ret == 0){
		unlink(file_response);
		unlink(file_request);
		
		//printf("return 3\n");
		return 0;
	}

	signal(SIGINT,SigFun);
	signal(SIGTERM,SigFun);
    signal(SIGSEGV,SigFun);

/*
		int exist;
		if((exist=access(file_response,0))!=0){
			flag = 0;
		}
		else{
			flag = 1;
		}
*/
		sprintf(tmp_sql_ensemble_file,"/data/audit/sql_tmp/Sql_studio_%s_%hu",rel->times,rel->dir);
		sprintf(sql_ensemble_file,"/data/audit/sql/Sql_studio_%s_%hu",rel->times,rel->dir);

		//process files
		TEST_FUN_EFF("ensembleFileInit",ret = ensembleFileInit(file_request,file_response,rel))
		if(ret == -1){
			if(argc == 2){
				unlink(file_response);
				unlink(file_request);
			}
			
			//printf("return 4\n");
			return ;
		}
		rel->request_time = GetFileTime(file_request);
		rel->response_time = GetFileTime(file_response);
		
		if(argc == 2){
			unlink(file_response);
			unlink(file_request);
		}

		if((rel->request_time  | rel->response_time) == 0){
				rel->interval_time=0;
		}
		else{
			rel->interval_time = rel->response_time - rel->request_time;
		}

		// test if med or others
		TEST_FUN_EFF("medtrakTest",isMedTrak = medtrakTest(rel->request_data,rel->request_size))
		if(isMedTrak == 1){
			rel->saveflag = 300;
			TEST_FUN_EFF("1isMedTrak medtrakParser",ret = medtrakParser(rel->request_data, rel->request_size,rel->operation, &rel->operation_len))
			if(ret == -1){
				
				//printf("return 5\n");
				return 0;
			}
			TEST_FUN_EFF("1isMedTrak ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,1))
			if(ret == -1){	
				
				//printf("return 6\n");
				return 0;
			}
			TEST_FUN_EFF("2isMedTrak medtrakParser",ret = medtrakParser(rel->response_data, rel->response_size,rel->response, &rel->response_len))
			if(ret == -1){

				//printf("return 7\n");
				unlink(tmp_sql_ensemble_file);
				return 0;
			}
			
			TEST_FUN_EFF("2isMedTrak ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,2))
			if(ret == -1){	
				
				//printf("return 8\n");
				unlink(tmp_sql_ensemble_file);
				return 0;
			}
		}
		else{
			TEST_FUN_EFF("1sqldbx_studio_release",ret = sqldbx_studio_release(rel,1));
			if(ret==-1){
				
				//printf("return 9\n");
				return 0;
			}
			
			TEST_FUN_EFF("1ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,1))
			if(ret== -1){
				
				//printf("return 10\n");
				return 0;
			}
			
			TEST_FUN_EFF("2sqldbx_studio_release",ret = sqldbx_studio_release(rel,2));
			if(ret ==-1){
				
				//printf("return 11\n");
				unlink(tmp_sql_ensemble_file);
				return  0;
			}
			
			TEST_FUN_EFF("2ensemble_sql",ret = ensemble_sql(tmp_sql_ensemble_file,rel,2))
			if(ret == -1){
				
				//printf("return 12\n");
				unlink(tmp_sql_ensemble_file);
				return 0;
			}
		}

		if(argc == 2)
			rename(tmp_sql_ensemble_file,sql_ensemble_file);

		
		//printf("return 100\n");
		return 0;

}
#endif


