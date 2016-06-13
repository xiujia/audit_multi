/*
	update 2014-12-10 新增定义  AuditWrite()函数

*/


#include "audit_api.h"
#include <iconv.h>




 MYSQL audit_mysql;

 


static char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";	

#define CHAR64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])
	
static signed char index_64[128] = {	
	    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,	
	    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,	
	    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,	
	    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,	
	    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,	
	    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,	
	    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,	
	    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1	
} ;

unsigned char *_base64_decode(const char *value, int *rlen,unsigned char *result)	
{		
	int c1, c2, c3, c4;	        	
        int vlen = strlen(value);	

	unsigned char *out = result;	
	
	*rlen = 0;
	
	while (1) {	
		if (value[0]==0) {
			*out = '\0' ; 
			return ;	
		}
	        c1 = value[0];	
                if (CHAR64(c1) == -1) goto base64_decode_error;
	             c2 = value[1];	
	             if (CHAR64(c2) == -1) goto base64_decode_error;
	             c3 = value[2];	
	             if ((c3 != '=') && (CHAR64(c3) == -1)) goto base64_decode_error;
	             c4 = value[3];	
	             if ((c4 != '=') && (CHAR64(c4) == -1)) goto base64_decode_error;	
                     value += 4;	
	             *out++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);	
	             *rlen += 1;	
	             if (c3 != '=') {	
	             	*out++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);	
	                *rlen += 1;	
	                if (c4 != '=') {	
	                	*out++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);	
	                        *rlen += 1;	
	                }
	             }	
	}	
	base64_decode_error:	
	        *result = 0;	
	        *rlen = 0;	
	        return ;	
}

static int length(char *s){
       int l=strlen(s);
           if(s[l-1]=='='){
                   if (s[l-2]=='=')
                        return ((l-2)*6/8+1);
                  else return ((l-1)*6/8+1);
          }else return l*6/8;

}

char * findstring(char *data,PRInt16 string){
	int i;
	for(i=0;i<1460;i++){
		if(get_u16(data,i)==htons(string))
		return data+i;
	}
	return NULL;
}

char *findstring_back(char *data,char *string){
	int i;
	for(i=0;i<strlen(data);i++){
		if(sunday_search(&data[strlen(data)-1-i],string))
			return &data[strlen(data)-1-i];
	}
	return NULL;
}

void delete_temp_file(char *file_path){
	char shell_command[200];
	memset(shell_command,0,sizeof(shell_command));
	sprintf(shell_command,"rm %s",file_path);
	system(shell_command);
}


char *find_key(char string[],char *key,int length){
	char *p;
	int i=0;
	for(i=0;i<length;i++){
		if((p=sunday_search(&string[i],key)))return p;
		else continue;
	}
	return NULL;
}


time_t _get_file_time(const char *filename)
{
        struct stat buf;
        if(stat(filename, &buf)<0)
        {
                return 0;
        }
        return buf.st_mtime;
}



char *strlink(char dest[],char *src){
	int len=strlen(dest);
	memcpy(dest+len,src,strlen(src));
	return dest;
}

PRUint32 serch(char s){

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


int base64_decode(char decoded[],char src[]){
	int decode_len=0,dest_len;
	char  *dest;
	char temp[AUDIT_TEXT_LEN];
	char *head,*trail;
	head=src;
	int i=0;
	memset(decoded,0,AUDIT_TEXT_LEN);
	if(sunday_search(head,"\r\n")){
		while(sunday_search(head,"\r\n")){
			trail=sunday_search(head,"\r\n");
			memset(temp,0,sizeof(temp));
			memcpy(temp,head,trail-head);
			dest_len=strlen(temp)*3/4+1;
			dest=(char *)malloc(dest_len);
			_base64_decode(temp,&decode_len,dest);
			memcpy(&decoded[i],dest,decode_len);
			i+=decode_len;
			free(dest);
			dest=NULL;
			head=trail+2;
		}
	}
	else if(sunday_search(head,"\r\n")==NULL){

		memset(temp,0,5000);
		memcpy(temp,src,strlen(src));
		dest_len=strlen(temp)*3/4+1;
//		pthread_rwlock_wrlock(&audit_malloc);
		dest=(char*)malloc(dest_len);
//		pthread_rwlock_unlock(&audit_malloc);
		_base64_decode(temp,&decode_len,dest);
		memcpy(decoded,dest,decode_len);
		i=decode_len;
//		pthread_rwlock_wrlock(&audit_malloc);
		free(dest);
//		pthread_rwlock_unlock(&audit_malloc);
	}
	return i;
}
/*
int check_audit_strings(PRUint32 index, char *string)
{
	TrieData	find_id = 0;
	PRUint16 pid;
	PRUint8 flag = 0;
	pid = share_audit_pro->audit_pro_cells[index].pid;
	flag = share_audit_pro->audit_pro_cells[index].pflag;
	
//	printf("check string:%s,flag===%u\n", string,flag);	

	if (flag == 0){
		return -1;
	}else if (flag == 1){
		return 0;
	}else if (flag == 2){
		pthread_rwlock_rdlock(&(audit_trie.rwlock));
		if(audit_trie.trie == NULL){
			pthread_rwlock_unlock(&(audit_trie.rwlock));
			return -1;
		}
		trie_retrieve(audit_trie.trie, string, &find_id);
		pthread_rwlock_unlock(&(audit_trie.rwlock));

		if(find_id != 0 && share_audit_pro->audit_pro_cells[index].pid == find_id)
			return 0;
		else
			return -1;
	}else
		return -1;
}
*/
	u_int64_t mail_id;
	u_int64_t attach_id;
	u_int64_t bbs_id;
	u_int64_t im_id;
	u_int64_t msg_id;
	u_int32_t file_id;
	u_int32_t im_qq_id;
	u_int64_t search_id;
	u_int64_t ftp_id;
	u_int64_t telnet_id;
	u_int64_t csp_id;



int select_max_mysql(MYSQL *m_mysql,const char *sql_select_command){
	MYSQL_RES * res;
	MYSQL_ROW row;
	unsigned long row_num;
/*	if(mysql_ping(m_mysql)!=0){
		while(connect_mysql(m_mysql)!=0)
			continue;
	}
*/
	if(connect_mysql(m_mysql)!=0) return 0;
	if(mysql_query(m_mysql,sql_select_command)){
	//	printf("Selected failed!\n");
		if(mysql_errno(m_mysql)){
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}
		return -1;
	}
	if((res = mysql_store_result(m_mysql))==NULL){
               fprintf(stderr, "mysql_store_result() error in do_flow_qos().\n");
               return -1;
       }
       while((row = mysql_fetch_row(res)) != NULL) {
                if(row[0]!=NULL)
                	return atoi(row[0]);
		  else return 0;
       }
	 return 0;
}

void audit_id_init(){
	char *sql_init_str;
	char path[]="/usr/inp/cfg/auditid.tmp";
	char con[10]={0};
	PRUint64 im_id1,im_id2;
	FILE *fp=NULL;
	mail_id=0;
	attach_id=0;
	bbs_id=0;
	im_id=0;
	msg_id=0;
	file_id=0;
	sql_init_str=AUDIT_INIT_FILEID;
   	file_id=select_max_mysql(&audit_mysql, sql_init_str);
	sql_init_str=AUDIT_INIT_IMID;
	im_id1=select_max_mysql(&audit_mysql, sql_init_str);
	sql_init_str=AUDIT_INIT_IMIDQQ;
	im_id2=select_max_mysql(&audit_mysql,sql_init_str);
	im_id = max(im_id1,im_id2);
	sql_init_str=AUDIT_INIT_POSTID;
	bbs_id=select_max_mysql(&audit_mysql, sql_init_str);
//	sql_init_str=AUDIT_INIT_MAILID;
//	mail_id=select_max_mysql(&audit_mysql, sql_init_str);
	sql_init_str=AUDIT_INIT_MSGID;
	msg_id=select_max_mysql(&audit_mysql, sql_init_str);
	sql_init_str=AUDIT_INIT_ATTACHID;
	attach_id=select_max_mysql(&audit_mysql, sql_init_str);
	sql_init_str=AUDIT_INIT_SEARCHID;
	search_id = select_max_mysql(&audit_mysql, sql_init_str);
	sql_init_str = AUDIT_INIT_TELNETID;
	telnet_id = select_max_mysql(&audit_mysql,sql_init_str);
	printf("telnet_id = %d\n",telnet_id);
	sql_init_str = AUDIT_INIT_FTPID;
	ftp_id = select_max_mysql(&audit_mysql,sql_init_str);
	im_qq_id = select_max_mysql(&audit_mysql, AUDIT_INIT_QQIM);
/*	
	fp=fopen(path,"r");
	if(fp){
		fread(con,1,10,fp);
		fflush(fp);
		fclose(fp);
		fp=NULL;
	}
	*/
//	mail_id=atoll(con)+1;
	sql_init_str = AUDIT_INIT_MAILID;
	mail_id = select_max_mysql(&audit_mysql,sql_init_str);
	system("mkdir -p /data/audit/eml/");
	system("mkdir -p /data/audit/bbs/");
	system("mkdir -p /data/audit/webmail/");
	system("mkdir -p /usr/inp/cfg/audit/");
	//printf("mail_id:%llu\n",mail_id);
}


int gbk_to_utf8(char  * gbk_str, unsigned long  gbk_len,char  * utf8_str, unsigned long utf8_len){
	 iconv_t   cd;   
	
	 cd = iconv_open("utf-8","gbk"); 
	  if   (cd==0)   return   -1; 
//	  printf("utf8_len===========%d",utf8_len);
	  memset(utf8_str,0,2*AUDIT_TEXT_LEN);
	  if(iconv(cd,&gbk_str,	&gbk_len,&utf8_str,&utf8_len)==-1) {
//	  	printf("\n---->%s\n",utf8_str);
		iconv_close(cd);	
		return -1;
	  }
	  iconv_close(cd);
	  return 0;

}

PRUint32 url_2_utf8(char utf8_code[],char url_code[], int length){
			int i=0, j;
			for(j=0;j<length;j++){					
				if(url_code[j]!='%')
				{
					if(url_code[j]=='+')
						url_code[j]=' ';
					utf8_code[i]=url_code[j];	
						i++;
				}
				else 
				{
					utf8_code[i] = 16 * serch(url_code[j+1]) + serch(url_code[j+2]); 
							j+=2;
							i++;
				}
			}if(sunday_search(utf8_code,"%25"))
				{
					char new_code[i];
										
					memset(new_code, 0, i);
					memcpy(new_code, utf8_code, i);
					memset(utf8_code, 0,i);
					url_2_utf8(utf8_code, new_code, i);
				}

				return i;		
}
unsigned long get_file_size_audit(char *filename)
{
	struct stat buf;
	if(stat(filename, &buf)<0)
		{
		return 0;
	}
	return (unsigned long)buf.st_size;
}
int get_db_line(MYSQL * mysql, char sql [ ], char line [ ]){
	MYSQL_RES *result; 
	MYSQL_ROW record;

	if(mysql_query(mysql,sql)){
		printf(mysql_error(mysql));
		return 1;
	}
	if((result=mysql_use_result(mysql))==NULL){
		printf( "mysql_use_result() in get db line error in .\n");
		return 2;
	}

	if((record=mysql_fetch_row(result))!=NULL){
		if(record[0]!=NULL){
			strcpy(line,record[0]);
			mysql_free_result(result);
			return 0;
		}
		else {
			printf("record==NULL\n");
		}
	}
	mysql_free_result(result);
	return 5;

}

void escape(char *from){
        char * ptr;
        ptr=from;
	if(ptr==NULL) return ;
        while(ptr=sunday_search(ptr,"'")){
                *ptr='`';
                ptr+=1;
        }

}
int code_convert(char * from_charset,char * to_charset,char  **input, unsigned long *input_len,char  ** output, unsigned long *output_len){
	 iconv_t   cd;   
	
	 cd = iconv_open(to_charset,from_charset); 
	  if(cd==(iconv_t)(-1))   return   -1; 
	//  memset(output,0,output_len);
	  if(iconv(cd,input,input_len,output,output_len)==-1) {
		iconv_close(cd);
		return -1;
	  }
	  iconv_close(cd);
	  return 0;

}
int NC_daemon_audit(void)
{   
	    pid_t pid;
		    int ret;
			    if ((pid = fork()) < 0)
					    {
							        return -1;
									    }
				    else if (pid != 0)
						    {
								        exit(0); /* parent goes bye-bye */
										    }
					    if((ret=setsid()) < 0) /* become session leader */
							    {
									        printf("unable to setsid.\n");
											    }
						    setpgrp();
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

//Taudit_mail_info mail_info[AUDIT_INDEX_NUM];
//u_int32_t mail_id_hash[AUDIT_INDEX_NUM];
//Taudit_bbs_info bbs_info[AUDIT_INDEX_NUM];
//u_int32_t bbs_id_hash[AUDIT_INDEX_NUM];
//Taudit_im_info im_info[AUDIT_INDEX_NUM];
//Taudit_desfp audit_desfp[MAX_FILE_FD];
//Taudit_csp_info csp_info[AUDIT_INDEX_NUM];
//u_int32_t csp_id_hash[AUDIT_INDEX_NUM];

//struct stream_stat_list streamList;




