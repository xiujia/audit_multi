//#define _GNU_SOURCE

#include <unistd.h>
#include <signal.h>
#include "csp_redis.h"
#include "audit_database_sql.h"
#include "audit_release.h"
#include "qsort.h"
#include <zlib.h>
#include "chunk.h"
#include "gzip.h"
#include "csp_policy.h"
#include "redis_new_api.h"
#include "audit_release.h"
#include "audit_ensemble.h"
#define __USE_GNU

#include <string.h>
//#undef _GNU_SOURCE

/*
use dbmonitor;
insert into tablse()                                    
values
(),
(),

*/
//web_url_id	(SELECT  cfg_monitor_web_requesturl.id FROM (cfg_monitor_web_requesturl left join cfg_monitor_web_requesturl_post_relation on cfg_monitor_web_requesturl.id = cfg_monitor_web_requesturl_post_relation.url_id)	join cfg_monitor_web_post_content on cfg_monitor_web_requesturl_post_relation.post_id = cfg_monitor_web_post_content.id WHERE cfg_monitor_web_post_content.post_value = '%s'),



#define GETPATH(a,b,c) \
	sprintf((a),"%s%s",(b),(c));
//#define DATA_JAVASCRIPT_ALART 	"<script type=\"text/javascript\">window.alert=function(){};window.confirm=function(){};window.prompt=function(){};self.moveTo=function(){};self.resizeTo=function(){};window.open=function(){};</script>\n"
#define DATA_JAVASCRIPT_ALART 	""

//AUDIT_ENSEMBLE_REL *rel;
#if GA_TEST
#define HBASE_VALUES_FMT	"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%d|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%hu|colfam1:dst_port=%hu|colfam1:department=%s|colfam1:web_session=%s|colfam1:user_name=%s|colfam1:web_url=%s|colfam1:web_content=%s|colfam1:user_id=%s|colfam1:alarm_id=%u|colfam1:charset=%s|colfam1:security_level=%hu|colfam2:file_content=%d|colfam\n%s"
#define HBASE_VALUES		rel->times,rel->table,rel->type,userip,desip,rel->srcmac,rel->desmac,rel->cliport,rel->serport,rel->department,rel->session,rel->userName,rel->requestUrl,rel->webContent,rel->cspHead.userid,rel->alarm.id,rel->charset,rel->security_level,fLen,DATA_JAVASCRIPT_ALART
#else
#define HBASE_VALUES_FMT	"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%d|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%hu|colfam1:dst_port=%hu|colfam1:user_id=%hu|colfam1:department=%s|colfam1:web_session=%s|colfam1:user_name=%s|colfam1:web_url=%s|colfam1:web_content=%s|colfam1:charset=%s|colfam1:operation_sql=%s|colfam1:line_num=%d|colfam1:saveflag=%d|colfam1:interval_time=%d|"
#define HBASE_VALUES		rel->times,rel->table,rel->type,userip,desip,rel->srcmac,rel->desmac,rel->cliport,rel->serport,rel->userid,rel->web.department,rel->web.session,rel->web.userName,rel->web.requestUrl,rel->web.webContent,rel->web.charset,rel->web.request_sql,rel->line_num,rel->saveflag,rel->interval_time
#define HBASE_VALUES_FMT_FILE "colfam2:file_content=%lu|colfam\n%s"
#define HBASE_VALUES_FILE		fLen,DATA_JAVASCRIPT_ALART



#endif


#define OVERLONG	(u_int64_t)-1
#define CSPDEBUG 	if(0)

#define SQL_MAX_LEN 2*1024*1024  //2M
#define AUDIT_CONTENT_PATH  "/dev/shm/"

#define PORTAL_RESPONSE_KW  "this.window.document.getElementById(\'htmlResult\');"
#define PORTAL_RESPONSE_KW_LEN 51
//char sqlQuery[MAX_THREAD_NUM][SQL_MAX_LEN];

#define TEST_EFF_DECLARED unsigned long st,fi;
#define TEST_FUN_EFF(B,A) A;//st=get_usec_time(); A; //fi=get_usec_time(); //printf("%s:%lu,%lu,%d\n",B,fi,st,fi-st);



char httpContent[MAX_THREAD_NUM][CSP_FILE_MAX_LEN];
char httpUnChunked[MAX_THREAD_NUM][CSP_FILE_MAX_LEN];
char httpUzip[MAX_THREAD_NUM][CSP_FILE_MAX_LEN];

char * httpptr[MAX_THREAD_NUM];
unsigned long httplen[MAX_THREAD_NUM];


#define CspFree(a)  if((a)) free((a));
#define CspClose(a)  if((a)) fclose((a));

 unsigned  int serch(char s){

	switch (s) {
                case '0' ... '9':
                        return (int)(s - 48);
                        break;
                case 'a' ... 'f':
                       return (int)(s - 87);
                        break;
                case 'A' ... 'F':
                        return (int)(s - 55);
                        break;
                default:
                        return 0;
                break;	
	}
}

static unsigned int  url_2_asc(char asc_code[],char url_code[], int length){
			int i=0, j,len=0;
			for(j=0;j<length;j++){					
				if(url_code[j]!='%')
				{
					if(url_code[j]=='+')
						url_code[j]=' ';
					asc_code[i]=url_code[j];	
						i++;
				}
				else 
				{
					asc_code[i] = 16 * serch(url_code[j+1]) + serch(url_code[j+2]); 
							j+=2;
							i++;
				}
			}if(strstr(asc_code,"%25"))
				{
					len = i;
					char new_code[len+1];
					
					memset(new_code, 0, len+1);
					memcpy(new_code, asc_code, len);
					memset(asc_code, 0,len+1);
					i = url_2_asc(asc_code, new_code, len);
				}

				return i;		
}



int CspCreateDir(char  * sPathName){
{ 
    char DirName[256]; 
    strcpy(DirName, sPathName); 
    int i,len = strlen(DirName); 
    if(DirName[len-1]!='/') 
        strcat(DirName, "/"); 
 
    len = strlen(DirName); 
 
    for(i=1; i<len; i++) 
    { 
        if(DirName[i]=='/') 
        { 
            DirName[i] = '\0'; 
            if( access(DirName, 0755)!=0 ) 
            { 
                if(mkdir(DirName, 0755)==-1) 	
                { 
                    perror("mkdir error"); 
                    return -1; 
                } 
            } 
            DirName[i] = '/'; 
        } 
    } 
 
    return 0; 
}
}

/*
static int AuditWrite(int Fd,char *psData,int Len){
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
*/
static unsigned int AuditRead(int Fd,char *psData,int Len){
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


static unsigned short GetCspPortalRequestSql(unsigned char * p,unsigned char * sql){
	unsigned char * pos_keywd1,*pos_keywd3,*end,*pos;
	unsigned char url_sql[2048]={0};
//	unsigned char sql_tmp[2048]={0};
	unsigned int url_sql_len=0,sql_len=0;
	

	static char keywd1[]="WARG_8=";
	static char keywd2[]="&";
	static char keywd3[]="WARG_9";
	static char keywd4[]={0x04,0x00};

	if((pos_keywd1 = strstr(p,keywd1)) && (pos_keywd3 = strstr(pos_keywd1+=7,keywd3))){
		if((end = strstr(pos_keywd1,keywd2))){
			url_sql_len = end-pos_keywd1;
			if(url_sql_len > 2048){
				url_sql_len = 2048;
			}
			if(url_sql_len <= 0) return 0;
			strncpy(url_sql,pos_keywd1,url_sql_len);
			url_sql[url_sql_len]='\0';
			if((pos = strstr(url_sql,keywd4))){
				*pos = '\0';
			}
			sprintf(sql,"%s",url_sql);
		//	printf("sql_tmp:%s\n",sql_tmp);
		//	printf("sql:%s\n",sql);
/*
		 "206":"SELECT²Ù×÷",
        "207":"UPDATE",
        "208":"DELETE",
        "209":"INSERT",
        "210":"CREATE",
        "211":"DROP",*/
			if((pos = strstr(url_sql," "))){
				*pos = '\0';
				 if(!strcasecmp(url_sql,"select")){
					return 206;
				 }
				 if(!strcasecmp(url_sql,"update")){
					return 207;
				 }
				 if(!strcasecmp(url_sql,"delete")){
					return 208;
				 }
				 if(!strcasecmp(url_sql,"insert")){
					return 209;
				 }
				 if(!strcasecmp(url_sql,"create")){
					 return 210;
				 }
				 if(!strcasecmp(url_sql,"drop")){
					 return 211;
				 }
				 
			}
			else{
				return 0;
			}
			return 0;
		}
	}

	return 0;
	
}

static int CspPortalGetResponseLineNum(char * p,int plen,int * line_num){
		unsigned char * start,*end;
		char lnum[20]={0};
		if(plen > PORTAL_RESPONSE_KW_LEN){
			if(start = strstr(p,PORTAL_RESPONSE_KW)){
				start += PORTAL_RESPONSE_KW_LEN;
				if((start = strstr(start,"<p>"))){
					if((end = strstr(start+3," "))){
						memset(lnum,0,2);
						memcpy(lnum,start+3,end-start-3);
						*line_num = atoi(lnum);
						return 0;
					}
				}
			}
		}
		return -1;
}
static char * CspGetKeywordValue(IN char * str,IN char * key_start,IN char * key_end,OUT char * outstr){
	char *start,*end;
	unsigned int len = 0;
	start = str;

	if((end = strstr(start,key_start))){
		start = end + strlen(key_start);
	}else{
		return str;
	}

	if(strcmp(key_start,CSP_KEYWD_SESSION) == 0 || strcmp(key_start,CSP_KEYWD_SESSION_PORTAL) == 0|| strcmp(key_start,CSP_KEYWD_SESSION_DOC_PORTAL) == 0){
		if((end = strstr(start,"--"))){
			end+=2;
			if((len = end -start) > 0){
				strncpy(outstr,start,len);
				outstr[len]='\0';
				return end;
			}
			else return NULL;
		}
		else return NULL;
	}

	if((end = strstr(start,key_end))){
		if( (len = end -start) > 0){
			memcpy(outstr,start,len);
			outstr[len]='\0';
			return end+strlen(key_end);
		}
		else return str;
	}
	else{
		return str;
	}
	
}

static int CspKeyType(char key[]){
	if(strcasecmp(key,"insert") == 0){
		return CSP_KEY_TYPE_INSERT;
	}
	else if(strcasecmp(key,"delete") == 0){
		return CSP_KEY_TYPE_DELETE;
	}
	else if(strcasecmp(key,"update") == 0){
		return CSP_KEY_TYPE_UPDATE;
	}
	else if(strcasecmp(key,"TFORM") == 0){
		return CSP_KEY_TYPE_TFORM;
	}
	else if(strcasecmp(key,"WEVENT") ==0){
		return CSP_KEY_TYPE_WEVENT;
	}
	return 0;
}

static void CspGetKeyValue(char * str,char key[],char value[]){
	sscanf(str,"%[^=]=%s",key,value);
}



static char *CspGetPostBodyValue(AUDIT_ENSEMBLE_REL * rel){// over
	char *data=NULL, *start=NULL,*end=NULL,*webcontent = NULL;
	int len = 0,bodylen= 0;
	char url_content[CSP_WEBCONTENT_LEN];
	start = data = rel->request_data;
	end = strstr(data,CSP_KEYWD_END_4);
	if(end == NULL) return NULL;
	end+=strlen(CSP_KEYWD_END_4);
	len = end-start;
	if(len < 0) return NULL;

	bodylen = rel->request_size - len;
	if(bodylen > CSP_WEBCONTENT_LEN)
		bodylen = CSP_WEBCONTENT_LEN;
	memcpy(url_content,end,bodylen);
	url_content[bodylen] = '\0';
	len = url_2_asc( rel->web.webContent,url_content,bodylen);
	rel->web.webContent[len] = '\0';
	webcontent = rel->web.webContent;

	rel->saveflag=GetCspPortalRequestSql(rel->web.webContent,rel->web.request_sql);

	return webcontent;
}

static void CspMvFile(char src_path[],char des_path[],unsigned short dir,char name[],int num){
	char sname[CSP_PATH_LEN]={0},dname[CSP_PATH_LEN]={0};
	sprintf(sname,"%s%hu%s%lu",src_path,dir,name,num);
	sprintf(dname,"%s%hu%s%lu",des_path,dir,name,num);
//	printf("%s->%s\n",sname,dname);
	rename(sname,dname);
}
/*
int CspWriteSql(AUDIT_ENSEMBLE_REL * rel){
	char fullFilePath[CSP_PATH_LEN] = {0};
	char path[CSP_PATH_LEN]={0};

	char despath[CSP_PATH_LEN]={0};
	char * pos;
	static u_int64_t fnum = 0;
	u_int64_t * num ;
	char * times;
	static int  maxrow = 1;
	int fLen = 0;
	int SqlQueryLen = 0;
	fLen = strlen(DATA_JAVASCRIPT_ALART);
	fLen += httplen;
	

		num = &sqlNum;
		sprintf(path,"%s%s",SQL_TMP,CSP_SQL_FILE);
		sprintf(despath,"%s%s",SQL_PATH,CSP_SQL_FILE);
		times = sqltimes_csp;

		memset(sqlQuery,0,SQL_MAX_LEN);
		sprintf(sqlQuery,HBASE_VALUES_FMT,HBASE_VALUES);
		SqlQueryLen = strlen(sqlQuery);
		if(httplen > 0)
			memcpy(sqlQuery+SqlQueryLen,httpptr,httplen);
		SqlQueryLen +=httplen;
		sqlQuery[SqlQueryLen]='\n';
		SqlQueryLen+=1;
		if((*num) %maxrow== 0){
			fnum = *num;
			memset(times,0,CSP_TIMES_LEN);
			sprintf(times,"%lu",rel->times);
		}
		(*num)++;
		sprintf(fullFilePath,"%s%hu%s%lu",path,rel->dir,times,fnum);
		httpptr = NULL;
		httplen = 0;
		sqlFp = fopen(fullFilePath,"a+");
		if(!sqlFp) return -1;

	//fprintf(sqlFp,"%s",sqlQuery);
	fwrite(sqlQuery,1,SqlQueryLen,sqlFp);
	fflush(sqlFp);
	fclose(sqlFp);
	if((*num)%maxrow== 0){
		//mv file
		
		CspMvFile(path,despath,rel->dir,times,fnum);
	}
	
	#if CSP_RELEASE_DEBUG
	printf("sqlNum:%lu\n",sqlNum);
	printf("alarmSqlNum:%lu\n",alarmSqlNum);
	#endif
	
	return 0;

}
*/
static int web_sql(char *sql_file_path,AUDIT_ENSEMBLE_REL *rel){
	int fd;
	unsigned long len_from = 0,len_tmp=0,len=0;
	int rr;
	unsigned int sip,dip;
	char userip[16]={0},desip[16]={0};
	unsigned long fLen = 0,left_len=0;
	char tmp[2048]={0};
	fLen = httplen[rel->thid];
//	printf("sql_file_path:%s\n",sql_file_path);
	if(httplen[rel->thid] == 0){
		return -1;
	}
	sip = ntohl(rel->userip);
	dip = ntohl(rel->desip);
	
	inet_ntop(AF_INET,(void*)(&sip),userip,sizeof(userip));
	inet_ntop(AF_INET,(void*)(&dip),desip,sizeof(desip));
	
	fd = open(sql_file_path,O_RDWR | O_CREAT, 0666);

	if(fd < 0) {
		printf("fd < 2; %d\n",fd);
		perror("open");
		return -1;
	}

		sprintf(from_segment[rel->thid],HBASE_VALUES_FMT,HBASE_VALUES);
		len_from = strlen(from_segment[rel->thid]);
		sprintf(tmp,HBASE_VALUES_FMT_FILE,HBASE_VALUES_FILE);
		len_tmp = strlen(tmp);
		len = len_from+len_tmp+fLen+1;
		if(len>REQ_RES_LEN){
			left_len = len - REQ_RES_LEN ;
			fLen-=left_len;
			sprintf(tmp,HBASE_VALUES_FMT_FILE,HBASE_VALUES_FILE);
			len_tmp = strlen(tmp);
			memcpy(&(from_segment[rel->thid][len_from]),tmp,len_tmp);
			len_from+=len_tmp;
			memcpy(&(from_segment[rel->thid][len_from]),httpptr[rel->thid],fLen);
			len_from += fLen;
			from_segment[rel->thid][len_from]="\n";
			len = len_from+1;
		}
		else{
			strncpy(&(from_segment[rel->thid][len_from]),tmp,len_tmp);
			len_from+=len_tmp;
			memcpy(&(from_segment[rel->thid][len_from]),httpptr[rel->thid],fLen);
			len_from += fLen;
			from_segment[rel->thid][len_from] = '\n';
			len_from += 1;
			len  = len_from;
		}

	rr = AuditWrite(fd,from_segment[rel->thid],len);
	
	close(fd);

	return rr;
}



static int  release(char * data,int type,AUDIT_ENSEMBLE_REL * rel){//over
	int err;
	unsigned int sip;
	char userip[16]={0};
	sip = ntohl(rel->userip);
	inet_ntop(AF_INET,(void*)(&sip),userip,sizeof(userip));
	if(type  ==AUDIT_CSP_TYPE_HIS_LOGON){

			CspGetKeywordValue(data,CSP_KEYWD_SESSION,CSP_KEYWD_END_1,rel->web.session); 
			CspGetKeywordValue(data,CSP_KEYWD_DEPARTMENT,CSP_KEYWD_END_1,rel->web.department);
			CspGetKeywordValue(data,CSP_KEYWD_USERNAME,CSP_KEYWD_END_1,rel->web.userName);

			err = CspRedisOperation(REDIS_DB_2,OPERATION_SET,rel->web.session,rel->web.userName,rel->conn);
			#if CSP_RELEASE_DEBUG	
			if(err == -1) {	
				printf("SESSION REDIS_DB_1,OPERATION_SET ");
				printf("redis error.\n");
			}
			#endif
			CspRedisOperation(REDIS_DB_0,OPERATION_SET,userip,rel->web.userName,rel->conn);
			err = CspRedisOperation(REDIS_DB_6,OPERATION_SET,rel->web.session,rel->web.department,rel->conn);
			#if CSP_RELEASE_DEBUG	
			if(err == -1) {
				printf("SESSION  REDIS_DB_6,OPERATION_SET ");
				printf("redis error.\n");
			}
			#endif
			CspRedisOperation(REDIS_DB_6,OPERATION_SET,userip,rel->web.department,rel->conn);
			//return -1;
	}
	else if(type  ==AUDIT_CSP_TYPE_PORTAL_LOGON){

			CspGetKeywordValue(data,CSP_KEYWD_USERNAME_PORTAL,CSP_KEYWD_END_1,rel->web.userName);				
		//	CspGetKeywordValue(data,CSP_KEYWD_SESSION,CSP_KEYWD_END_1,rel->web.session); 
		//set redis in http packet
	}
	else if(type  ==AUDIT_CSP_TYPE_CLS){
			if(rel->type == AUDIT_CSP_TYPE_HIS){
				 CspGetKeywordValue(data,CSP_KEYWD_SESSION,CSP_KEYWD_END_2,rel->web.session);
				err = CspRedisOperation(REDIS_DB_6,OPERATION_GET,rel->web.session,rel->web.department,rel->conn);//redis-cli select 1  get 
				#if CSP_RELEASE_DEBUG
				if(err == -1) {
					printf("SESSION REDIS_DB_6,OPERATION_GET ");
					printf("redis error.\n");
				}
				#endif
				if(strlen(rel->web.department)==0){
					CspRedisOperation(REDIS_DB_6,OPERATION_GET,userip,rel->web.department,rel->conn);
				}

				err = CspRedisOperation(REDIS_DB_2,OPERATION_GET,rel->web.session,rel->web.userName,rel->conn);//redis-cli select 1  get 
					#if CSP_RELEASE_DEBUG
					if(err == -1) {
						printf("SESSION REDIS_DB_2,OPERATION_GET ");
						printf("redis error.\n");
					}
					#endif
					if(strlen(rel->web.userName) == 0){
						CspRedisOperation(REDIS_DB_0,OPERATION_GET,userip,rel->web.userName,rel->conn);
				}
			}
			else if(rel->type == AUDIT_CSP_TYPE_PORTAL){
				CspGetKeywordValue(data,CSP_KEYWD_SESSION_PORTAL,CSP_KEYWD_END_2,rel->web.session);
				CspGetKeywordValue(data,CSP_KEYWD_SESSION_DOC_PORTAL,CSP_KEYWD_END_2,rel->web.session);
			
				err = CspRedisOperation(REDIS_DB_3,OPERATION_GET,rel->web.session,rel->web.userName,rel->conn);//redis-cli select 1  get 
				#if CSP_RELEASE_DEBUG
				if(err == -1) {
					printf("SESSION REDIS_DB_2,OPERATION_GET ");
					printf("redis error.\n");
				}
				#endif
				if(strlen(rel->web.userName) == 0){
					CspRedisOperation(REDIS_DB_1,OPERATION_GET,userip,rel->web.userName,rel->conn);
				}
			}
	}
	else if(type == AUDIT_CSP_TYPE_CSP){
			if(rel->type == AUDIT_CSP_TYPE_HIS||rel->type == 30){
					 CspGetKeywordValue(data,CSP_KEYWD_SESSION,CSP_KEYWD_END_2,rel->web.session);
					err = CspRedisOperation(REDIS_DB_6,OPERATION_GET,rel->web.session,rel->web.department,rel->conn);//redis-cli select 1  get 
					#if CSP_RELEASE_DEBUG
					if(err == -1) {
						printf("SESSION REDIS_DB_6,OPERATION_GET ");
						printf("redis error.\n");
					}
					#endif
					if(strlen(rel->web.department)==0){
						CspRedisOperation(REDIS_DB_6,OPERATION_GET,userip,rel->web.department,rel->conn);
					}

					
					err = CspRedisOperation(REDIS_DB_2,OPERATION_GET,rel->web.session,rel->web.userName,rel->conn);//redis-cli select 1  get 
					#if CSP_RELEASE_DEBUG
						if(err == -1) {
							printf("SESSION REDIS_DB_2,OPERATION_GET ");
							printf("redis error.\n");
						}
					#endif
					if(strlen(rel->web.userName) == 0){
						CspRedisOperation(REDIS_DB_0,OPERATION_GET,userip,rel->web.userName,rel->conn);
					}
			}
			else if(rel->type == AUDIT_CSP_TYPE_PORTAL){
					CspGetKeywordValue(data,CSP_KEYWD_SESSION_PORTAL,CSP_KEYWD_END_2,rel->web.session);
					CspGetKeywordValue(data,CSP_KEYWD_SESSION_DOC_PORTAL,CSP_KEYWD_END_2,rel->web.session);
			
				err = CspRedisOperation(REDIS_DB_3,OPERATION_GET,rel->web.session,rel->web.userName,rel->conn);//redis-cli select 1  get 
					#if CSP_RELEASE_DEBUG
					if(err == -1) {
						printf("SESSION REDIS_DB_2,OPERATION_GET ");
						printf("redis error.\n");
					}
					#endif
				if(strlen(rel->web.userName) == 0){
					CspRedisOperation(REDIS_DB_1,OPERATION_GET,userip,rel->web.userName,rel->conn);
				}
			}

	}

	return 0;
}

static int CspRequestReleaseData(AUDIT_ENSEMBLE_REL * rel){
	char *data,*start,*end;
	char * pos;
	char requestUrl[CSP_URL_LEN] = {0};
	unsigned int len = 0;
	int err = 0;
	int requestType;
	data = rel->request_data;

	if((start = strstr(data,"POST "))){
		start += 5;
		requestType = CSP_REQUEST_TYPE_POST;
		CspGetPostBodyValue(rel);
	}
	else if((start = strstr(data,"GET "))){
		start+= 4;
		requestType = CSP_REQUEST_TYPE_GET;
	}
	else{
		return -1;
	}
	
	if((end = strstr(start," HTTP/1.1\r\n"))){
		len = end -start;
		if( len  < 0)
			return -1;
		
//		memset(requestUrl,0,sizeof(requestUrl));
		
//		memset(rel->web.requestUrl, 0,sizeof(rel->web.requestUrl));
		if(len < CSP_URL_LEN){
			memcpy(rel->web.requestUrl,start,len);
			rel->web.requestUrl[len]='\0';
			memcpy(requestUrl,start,len);
			requestUrl[len]='\0';
		}
		else{
			len = CSP_URL_LEN-1;
			memcpy(rel->web.requestUrl,start,len);
			rel->web.requestUrl[len]='\0';
			memcpy(requestUrl,start,len);
			requestUrl[len]='\0';
		}


		
	}
	else return -1;

	if(strncmp(requestUrl,CONGOUS_URL_PASSPORT,strlen(CONGOUS_URL_PASSPORT))==0){
		return -1;
	}

	if((pos = strstr(requestUrl,"?"))){
		*pos = '\0';
	}

	start = end;	

	if(requestType == CSP_REQUEST_TYPE_GET){

			if(strcasestr(requestUrl,".gif")){
				return -1;
			}
			else if(strcasestr(requestUrl,".png")){
				return -1;
			}
			else if(strcasestr(requestUrl,".jpg")){
				return -1;
			}
			else if(strcasestr(requestUrl,".js")){
				return -1;
			}
			else if(strcasestr(requestUrl,".jsp")){
				return -1;
			}
			else if(strcasestr(requestUrl,".css")){
				return -1;
			}
			else if(strcasestr(requestUrl,".dll")){
				return -1;
			}
			else if(strcasestr(requestUrl,".exe")){
				return -1;
			}
			

			
						
		/*
			if(strstr(requestUrl,"dhcvisanopwait")){
				return -1;
			}
			if(strstr(requestUrl,"dhcvispatwait")){
				return -1;
			}
	//		dhcrisappbill  dhcnuripexeclist

			if(strstr(requestUrl,"dhcrisappbill")){
				return -1;
			}

			if(strstr(requestUrl,"dhcnuripexeclist")){
				return -1;
			}
			if(strstr(requestUrl,"dhc.epr.messagetab.csp")){
				return -1;
			}
			if(strstr(requestUrl,"dhc.bdp.ext.sys.csp")){
				return -1;
			}	
			if(!strstr(requestUrl,".csp") &&!strstr(requestUrl,".cls"))
				return -1;
				*/
	}
	else if(requestType == CSP_REQUEST_TYPE_POST){
		return 1;
		if(!strstr(requestUrl,".csp") &&!strstr(requestUrl,".cls"))
			return -1;

	}

	if(memcmp(requestUrl,AUDIT_CSP_REQUEST_LOGON_HIS,strlen(AUDIT_CSP_REQUEST_LOGON_HIS) )== 0 ){
		rel->web.appId = AUDIT_CSP_TYPE_HIS_LOGON;
	}
	else if(memcmp(requestUrl,AUDIT_CSP_REQUEST_LOGON_PORTAL,strlen(AUDIT_CSP_REQUEST_LOGON_PORTAL)) == 0){
		rel->web.appId = AUDIT_CSP_TYPE_PORTAL_LOGON;
	}
	else if(strstr(requestUrl,".cls")){
		rel->web.appId = AUDIT_CSP_TYPE_CLS;
	}
	else if(strstr(requestUrl,".csp")){
		rel->web.appId = AUDIT_CSP_TYPE_CSP;
	}
	else{
		rel->web.appId = AUDIT_CSP_TYPE_OTHERS;
	}

	if(start == NULL)  return -1;

	return release(start,rel->web.appId,rel);
	 
}

static int CspResponseHeadRelease(AUDIT_ENSEMBLE_REL * rel){
	char * data,*start,*end;
	char keyValueStr[CSP_KEYVALUE_LEN]={0};
	char httpHead[CSP_HTTPHEAD_LEN]={0};
	char key[CSP_KEY_LEN]={0};
	char value[CSP_VALUE_LEN]={0};
	int len,httpHeadLen;
	unsigned int data_len;
	unsigned int sip;
	char userip[16]={0};
	sip = ntohl(rel->userip);
	inet_ntop(AF_INET,(void*)(&sip),userip,sizeof(userip));
	start = data = rel->response_data;
	data_len = rel->response_size;
	
	if(strncmp(start,"HTTP",4)!=0) return -2;
	start = strstr(start,"\r\n");
	if(start == NULL) return -1;
	start +=4;
	end = strstr(start,CSP_KEYWD_END_4);
	if(end == NULL) return -1;
	end += 4;
	httpHeadLen = end - data;
	if(httpHeadLen < 0) return -1;
	if(httpHeadLen > sizeof(httpHead)){
		return -2;
	}
//	printf("httpHeadLen = %d\n",httpHeadLen);
	strncpy(httpHead,data,httpHeadLen);
	httpHead[httpHeadLen]='\0';
	start = httpHead;

	rel->web.charset[0]='0';
	rel->web.charset[1]='\0';
	//printf("id:%d\n",csp->cspData.appId);
	if(rel->web.appId == AUDIT_CSP_TYPE_PORTAL_LOGON){
		if(!start) return -1;
		CspGetKeywordValue(data,CSP_KEYWD_SESSION_PORTAL,CSP_KEYWD_END_2,rel->web.session);	
		CspGetKeywordValue(data,CSP_KEYWD_SESSION_DOC_PORTAL,CSP_KEYWD_END_2,rel->web.session);
//		printf("username:%s\n",csp->cspData.userName);
//		printf("session:%s\n",csp->cspData.session);
		CspRedisOperation(REDIS_DB_3,OPERATION_SET,rel->web.session,rel->web.userName,rel->conn);
		CspRedisOperation(REDIS_DB_1,OPERATION_SET,userip,rel->web.userName,rel->conn);
	//	return -1;
	}

//	printf("%s\n",httpHead);
	if((start =strstr(httpHead,"charset="))){
		
		start +=8;
	//	printf("start:%s\n",start);
		if((end  = strstr(start,"\r\n"))){
		//	printf("end :%s\n",end);
			len = end -start;
			if(len > 0 &&len < 10){
				strncpy(rel->web.charset,start,len);
				rel->web.charset[len]='\0';
			}
		}
	}
//	printf("charset=%s\n",csp->cspHttpHead.charset);
	start = httpHead;
	len = 0;
	do{
		if((end = strstr(start,CSP_KEYWD_END_3))){
			len = end - start;
			if(len < 0) return -1;
			strncpy(keyValueStr,start,len);
			keyValueStr[len] = '\0';
			memset(key,0,sizeof(key));
			memset(value,0,sizeof(value));
			sscanf(keyValueStr,"%[^:]: %s",key,value);
			if(strcasecmp(key,CSP_HTTP_KW_CONTENT_ENCODING) == 0){
				sprintf(rel->web.contenEncode,"%s",value);
			}
			else if(strcasecmp(key,CSP_HTTP_KW_TRANS_ENCODING) == 0){
				sprintf(rel->web.transEncode,"%s",value);
			}
			else if(strcasecmp(key,CSP_HTTP_KW_CONTENT_LENGTH) == 0){
				sprintf(rel->web.contentLength,"%s",value);
			}
		}
		start = end + 2;
		if(start - httpHead == httpHeadLen -2) 
			break;
	}
	while(1);
	
	return 0;
}
static int CspGetTform(char *str,char *tform){
			char * tformpos,*end;
			int len;
			tformpos = strstr(str,"TFORM");
			if(tformpos == NULL  ){
				return  -1;
			}
			tformpos = strstr(tformpos,"VALUE=\"");
			if(tformpos == NULL){
				return -1;				
			}
			tformpos+=7;
			end = strstr(tformpos,"\"");
			if(!end){
				return -1;
			}
			if((len = end - tformpos)< 0 ) {
				return -1;
			}
			strncpy(tform,tformpos,len);
			tform[len]='\0';
			return 0;
}


static int CspResponseReleaseData(AUDIT_ENSEMBLE_REL * rel){
	char *data,*start, *body,*unzipstr,*unchunked,*end;
	int bodylen,headlen;
	int encodeStatus = 0;
	unsigned long unchunklen =0;
	unsigned long unziplen = 0;
	unsigned int data_len;

	static unsigned int ggg=0;
	
	if(rel->response_data == NULL) {
	//	printf("aaa\n");
		return -1;

	}
	data = rel->response_data;
	data_len = rel->response_size;
	start = data;
	end = strstr(data,"Content-Type:");
	if(!end) {
	//	printf("bbb\n");
		return -1;
	}
	end += strlen(CSP_KEYWD_END_3);
	end = strstr(end,CSP_KEYWD_END_4);
	if(!end)  {
	//	printf("ccc\n");
		return -1;
	}
	end+=strlen(CSP_KEYWD_END_4);
	headlen = end - start;
	if(headlen < 0) {
	//	printf("ddd\n");
		return -1;
	}
	bodylen = data_len - headlen;
	if(bodylen < 0){
	//	printf("eee\n");
		return -1;
	}
	httpptr[rel->thid] = NULL;
	httplen[rel->thid] = 0;
	
	if(strcasecmp(rel->web.transEncode,"chunked") == 0){
		encodeStatus += CSP_HTTP_ENCODING_STATUS_CHUNK;
	}

	if(strcasecmp(rel->web.contenEncode,"gzip") == 0){
		encodeStatus += CSP_HTTP_ENCODING_STATUS_GZIP;
	}

	//memset(httpContent,0,CSP_FILE_MAX_LEN*sizeof(char));
	//memset(httpUnChunked,0,CSP_FILE_MAX_LEN*sizeof(char));
	//memset(httpUzip,0,CSP_FILE_MAX_LEN*sizeof(char));
	if(bodylen > CSP_FILE_MAX_LEN*sizeof(char)){	
	//	printf("fff\n");
		return -1;
	}
	memcpy(httpContent[rel->thid],end,bodylen);
	httpContent[rel->thid][bodylen]='\0';
	switch(encodeStatus){
		case 0:
			httpptr[rel->thid] = httpContent[rel->thid];
			httplen[rel->thid] = bodylen;
			break;
		case CSP_HTTP_ENCODING_STATUS_CHUNK:
			if(dechunk(httpContent[rel->thid],httpUnChunked[rel->thid],&unchunklen,bodylen,CSP_FILE_MAX_LEN*sizeof(char)) == NULL){
		//		printf("ggg =%u\n",ggg++);
				
				return -1;
			}
			httpptr[rel->thid] = httpUnChunked[rel->thid];
			httplen[rel->thid] = unchunklen;
			break;
		case CSP_HTTP_ENCODING_STATUS_GZIP:
			unziplen = CSP_FILE_MAX_LEN*sizeof(char) -1;
			if(gzdecompress((unsigned char *)httpContent[rel->thid],bodylen,(unsigned char *)httpUzip[rel->thid],&unziplen) < 0){
	//			printf("hhh\n");

				return -1;
			}

			httpptr[rel->thid] = httpUzip[rel->thid];
			httplen[rel->thid]= unziplen;
			break;
		case CSP_HTTP_ENCODING_STATUS_CHUNK_GZIP:

			if(dechunk(httpContent[rel->thid],httpUnChunked[rel->thid],&unchunklen,bodylen,CSP_FILE_MAX_LEN*sizeof(char)) == NULL){
	//			printf("iii\n");
				return -1;
			}
			unziplen = CSP_FILE_MAX_LEN*sizeof(char) -1;
			if(gzdecompress((unsigned char *)httpUnChunked[rel->thid],unchunklen,(unsigned char *)httpUzip,&unziplen)<0){
	//			printf("jjj\n");
				return -1;
			}
			httpptr[rel->thid] = httpUzip[rel->thid];
			httplen[rel->thid] = unziplen;
			break;
		default:
			break;
	}

	#if HUAXI_TEST
	CspPortalGetResponseLineNum(httpptr[rel->thid],httplen[rel->thid],&rel->line_num);
	#endif
	return 0;
}

static int  CspRequestFilePro(AUDIT_ENSEMBLE_REL* rel){
		return CspRequestReleaseData(rel);
}

static int CspResponseFilePro( AUDIT_ENSEMBLE_REL * rel){
		int ret=0;
		TEST_EFF_DECLARED
		TEST_FUN_EFF("CspResponseHeadRelease",ret = CspResponseHeadRelease(rel);)
	//	printf("1ret = %d\n",ret);
		if(ret == 0){
			TEST_FUN_EFF("CspResponseReleaseData",ret = CspResponseReleaseData(rel);)
			
	//		printf("2ret = %d\n",ret);
			return ret;
		}
		return -1;
}
int CspDoubleFileInit(char * request,char * response,AUDIT_ENSEMBLE_REL *rel){
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


int NC_daemon_audit(void)
{  
	pid_t pid;
	int ret;
	if ((pid = fork()) < 0){
		return -1;
	}
	else if (pid != 0){
		exit(0); /* parent goes bye-bye */
	}
	if((ret=setsid()) < 0) /* become session leader */
	{
		printf("unable to setsid.\n");
	}
	 setpgrp();
	 return 0;
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

static int getfilename(char * fullpath,char *filename,char *filepath,int len){
	int i=0,position=0,lastposition=0;
	int flen;
	int j=0;
	int len1=0,len2=0;
	for(i=len-1;i>0;i--){
		if(fullpath[i]=='/'){
			j++;
			if(j == 1){
				position=i;
				lastposition = position;
				flen = len-position;
				if(flen < 0) return 0;
				len1=flen-1;
				strncpy(filename,&fullpath[position+1],len1);
				filename[len1]='\0';
				len2 = len-len1;
				strncpy(filepath,fullpath,len2);
				filepath[len2]='\0';
			}
			if(j == 2){
				position = i;
			}

			return 1;
		}		
	}
	return 0;
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




int web_process(AUDIT_ENSEMBLE_REL * rel){
		int argc = 2;
		int ret = 0;
		
	
		char tmp_sql_web_file[PATH_LEN]={0};
		char sql_web_file[PATH_LEN]={0};

		rel->web.session[0]='\0';
		rel->web.department[0]='\0';
		rel->web.userName[0]='\0';
		rel->web.requestUrl[0]='\0';
		rel->web.webContent[0]='\0';
		rel->web.transEncode[0]='\0';
		rel->web.contenEncode [0]='\0';
		rel->web.contentLength[0]='\0';
		rel->web.request_sql[0]='\0';
		rel->web.charset[0]='\0';
		rel->web.type=0;
		rel->web.appId=0;


	//	printf("request_file:%s\n",request_file);
	//	printf("request_file:%s\n",response_file);
		
		
		sprintf(tmp_sql_web_file,"/data/audit/sql_tmp/Sql_csp_%lu_%hu",rel->times,rel->dir);
		sprintf(sql_web_file,"/data/audit/sql/Sql_csp_%lu_%hu",rel->times,rel->dir);
/*		TEST_FUN_EFF("CspDoubleFileInit",ret = CspDoubleFileInit(request_file,response_file,rel);)
			rel->request_time = GetFileTime(request_file);
			rel->response_time = GetFileTime(response_file);
			rel->interval_time = rel->response_time - rel->request_time;
			if(argc == 2 ){
				TEST_FUN_EFF("unlink(response)",unlink(response_file);)
				TEST_FUN_EFF("unlink(request)",unlink(request_file);)
			}

			if(ret == -1){
				//printf("5\n");
				return 0;
			}
	*/		
			
		 	TEST_FUN_EFF("CspRequestFilePro",ret = CspRequestFilePro(rel);)
			if(ret < 0){

			//	printf("6\n");
				return 0;
			}

			//ÓÐresponse
		
			TEST_FUN_EFF("CspResponseFilePro",ret = CspResponseFilePro(rel);)
			if(ret < 0){
			//	printf("7\n");
				return 0;
			}
			
			ret = web_sql(sql_web_file,rel);
	//		TEST_FUN_EFF("CspWriteSql",ret = CspWriteSql(&cspFileInfo))
			if(ret < 0){
//				printf("sql_8\n");
				return 0;
			}
			if(argc == 2)
				rename(tmp_sql_web_file,sql_web_file);

		return 0;

}

#undef  __USE_GNU


#if 0
int main(int argc,char * argv[]){
//	if(argc ==2)
//		NC_daemon_audit();
	
	char *pos;
	int flag=0;
	int ret=0;
	unsigned long fsize;
	CACHE_POLICY_CONF * policy=NULL;
	httpptr=NULL;
	httplen=0;
	TEST_EFF_DECLARED
	

// 	/dev/shm/1/10_2887326051_000fe25c06a0_60100_168466306_80c16ef872cd_1433_0_2_1440906275885842_request

//	printf("after policy mem\n");
	sprintf(request_file,"%s",argv[1]);

	if(check_request_type(request_file) == -1){
		unlink(response_file);
		unlink(request_file);		
	//	printf("1\n");
		return 0;		
	}

	pos = strstr(request_file,"_request");
	*pos = '\0';
	sprintf(response_file,"%s_response",request_file);
	*pos='_';
	
	if((fsize = GetCspFileSize(response_file))<=100){
		unlink(response_file);
		unlink(request_file);
	//	printf("2\n");
		return 0;
	}

	memset(&cspFileInfo,0,sizeof(CSP_FILE_INFO));
	
	policy = (CACHE_POLICY_CONF*)get_audit_cache_policy_shm();
	if(!policy){
		return 0;
	}

//	TEST_FUN_EFF("GetValues",GetValues(&cspFileInfo,request_file);)
	
	TEST_FUN_EFF("GetValues",GetValues(&cspFileInfo,request_file);)

	
	if(cspFileInfo.type == 30){
		if(fsize < 600){
			unlink(response_file);
			unlink(request_file);
			return 0;
		}
		cspFileInfo.type = 1;
		TEST_FUN_EFF("policy_match",ret = policy_match(&cspFileInfo,policy);)
		cspFileInfo.type = 30;
	}
	else{
		TEST_FUN_EFF("policy_match",ret = policy_match(&cspFileInfo,policy);)
	}
	if(ret == 0){
		unlink(response_file);
		unlink(request_file);		
	//	printf("3\n");
		return 0;
	}
	signal(SIGINT,CspSigFun);
	signal(SIGTERM,CspSigFun);
    signal(SIGSEGV,CspSigFun);
		

		if(atoi(cspFileInfo.cspHead.id) ==40){
			; 
		}
		else if(atoi(cspFileInfo.cspHead.id) <100||atoi(cspFileInfo.cspHead.id) >=200) {
		//	printf("4\n");
			return 0;
		}

		
		if(access(response_file,0)!=0){
			flag = 0;
		}
		else{
			flag = 1;
		}
		
		TEST_FUN_EFF("CspDoubleFileInit",ret = CspDoubleFileInit(request_file,response_file,&cspFileInfo);)
			cspFileInfo.request_time = GetFileTime(request_file);
			cspFileInfo.response_time = GetFileTime(response_file);
			cspFileInfo.interval_time = cspFileInfo.response_time - cspFileInfo.request_time;
	//		cspFileInfo.interval_time = 100;
			if(argc == 2 ){
				TEST_FUN_EFF("unlink(response)",unlink(response_file);)
				TEST_FUN_EFF("unlink(request)",unlink(request_file);)
			}

			if(ret == -1){
			//	printf("5\n");
				return 0;
			}
			


			cspFileInfo.conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);	

			if(cspFileInfo.conn->err){
				printf("%s\n",cspFileInfo.conn->errstr);
				redisFree(cspFileInfo.conn);
				cspFileInfo.conn=NULL;
				cspFileInfo.conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
				if(cspFileInfo.conn->err){
					redisFree(cspFileInfo.conn);
					cspFileInfo.conn = NULL;
					printf("can not connect  to redis server.\n");
					return -1;					
				}
			}

			
		 	TEST_FUN_EFF("CspRequestFilePro",ret = CspRequestFilePro(&cspFileInfo);)
			if(ret < 0){
				redisFree(cspFileInfo.conn);
				cspFileInfo.conn = NULL;
			//	printf("6\n");
				return 0;
			}

			//ÓÐresponse
			if( flag == 1 ){
				TEST_FUN_EFF("CspResponseFilePro",ret = CspResponseFilePro(&cspFileInfo);)
				if(ret < 0){
					redisFree(cspFileInfo.conn);
					cspFileInfo.conn = NULL;
				//	printf("7\n");
					return 0;
				}
			}

			redisFree(cspFileInfo.conn);
			cspFileInfo.conn = NULL;
			TEST_FUN_EFF("CspWriteSql",ret = CspWriteSql(&cspFileInfo))
			if(ret < 0){
		//		printf("8\n");
				return 0;
			}
		//	finish = get_usec_time();
		//	printf("his runtime = %lu nanosec.\n",finish-start);
	//	printf("10\n");
		return 0;

#undef  __USE_GNU
}


#endif


