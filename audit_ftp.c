#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "audit_api.h"
#include "audit_release.h"
#include "audit_database_sql.h"
#include<sys/types.h>


//21端口与非21端口


//主动模式:PORT
//PORT a,b,c,d,m,n
//data dst port == m*256+n
//被动模式:PASV
//227 Entering Passive Mode (202,38,97,197,117,20).
/*
int get_dataport(char * str)
{
	char strdst[50];
	int n1,n2,n3,n4,n5,n6;
	char *start =strstr(str,"("）;
	char * end = strstr(str,")");

	memset(strdst,0,sizeof(strdst));
	memcpy(strdst,start+1,end-start-1);
	sscanf( str, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]", n1,n2,n3,n4,n5,n6);
	
	return n5*256+n6;
}


*/
//data src ip ==202.38.97.197
// data src port==117*256+20 =29972




#define  	KEY_LOGON 		"230 "
#define  	KEY_LOGOUT 		"331 " 
#define  	KEY_BYE 		"221 "
#define  	KEY_BAD_CMD 	"503 "
#define 	KEY_DATAPORT   	"227 "
#define 	KEY_DATAEND 	"226 "
#define		KEY_GET 		"RETR"
#define		KEY_UPLOAD 		"STOR"	//STOU
#define  	KEY_USER 		"USER"
#define  	KEY_PASS 		"PASS"
#define 	KEY_PASV 		"PASV"
#define  	KEY_PORT 		"PORT"


#define 	RES_MAX_LEN		10000


extern pthread_mutex_t ftp_id_mutex;

extern pthread_rwlock_t tablesTime_lock;

extern Taudit_ftp_info ftp_info[THREADS_NUM][AUDIT_INDEX_NUM];

extern Taudit_file_info file_info[AUDIT_INDEX_NUM];

char str_sql[THREADS_NUM][RES_MAX_LEN];
FILE *fp_insert[THREADS_NUM];
static char filename[THREADS_NUM][1000];
static char tml_file[THREADS_NUM][100];
static char dst_filename[THREADS_NUM][1000];

/*
typedef struct {
	char user[AUDIT_ACCOUNT_LEN];
	char pass[AUDIT_ACCOUNT_LEN];
	Bool log_state:2,end_flag,mode_flag:2,trans_flag:2 ;
	u_int64_t id;
	u_int16_t cmd_id;
	int res_len;
	char cmd[AUDIT_ACCOUNT_LEN];
	char flnm[AUDIT_CMD_LEN];

}Taudit_ftp_info;

typedef struct {
	char *user;
	int port;
	char *file_cmd;
	char *srcip;
	char *dstip;
	char *srcmac;
	char *dstmac;
	int file_set;
	int res_len;
}Taudit_file_info

*/
int get_port(char * str)
{
	char strdst[50];
	int n1,n2,n3,n4,n5,n6;
	//char *start =strstr(str,"PORT ");
	

	memset(strdst,0,sizeof(strdst));
	sscanf( str, "%*s%s", strdst);
	//printf("port:%s\n",strdst);
	sscanf( strdst, "%d,%d,%d,%d,%d,%d", &n1,&n2,&n3,&n4,&n5,&n6);
	
	return n5*256+n6;
};

u_int32_t  audit_ftp(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char* payload)
{
		int p_num = p_info_hd->thrid;//线程号
        u_int32_t num = p_info_hd->hash%AUDIT_INDEX_NUM;
		int port_index ;
		char 	src_ip[32];
		char	dst_ip[32];
		char 	src_mac[64];
		char	des_mac[64];

if(0==p_info_hd->payload_len)
{
	return 0;
}

		
        if(p_info->isnew){                   //新流
        	//printf("新流\n");
			//printf("新流\n");
			//printf("新流\n");
        	p_info->audit_policy_stat = 1;         //初始化
			p_info->basic=1;
			
			pthread_mutex_lock(&ftp_id_mutex);
			
			ftp_info[p_num][num].id = ++ftp_id;
			pthread_mutex_unlock(&ftp_id_mutex);
			//ftp_info[p_num][num].cmd_id = 1; 
			ftp_info[p_num][num].end_flag=0;
			ftp_info[p_num][num].trans_flag =0;
			ftp_info[p_num][num].res_len = 0;
		//	memset(ftp_info[p_num][num].user,0,sizeof(ftp_info[p_num][num].user));
		//	memset(ftp_info[p_num][num].pass,0,sizeof(ftp_info[p_num][num].pass));
		//	memset(ftp_info[p_num][num].cmd,0,sizeof(ftp_info[p_num][num].cmd));
			//memset(ftp_info[p_num][num].flnm,0,sizeof(ftp_info[p_num][num].flnm));

        }
		
        
		
			if(p_info_hd->isin)
			{                        //入流
			
		
				if (memcmp(payload,KEY_BYE,strlen(KEY_BYE))==0)
				{                        //结束则策略状态改变
					p_info->audit_policy_stat = 0;
				//	printf("结束\n");
					
					return 0;
				}
				else
				{
					if(!ftp_info[p_num][num].trans_flag)
					{
						if(!ftp_info[p_num][num].log_state)
						{
							if (memcmp(payload,KEY_LOGON,strlen(KEY_LOGON))==0)
							{                   //用户已登录
								ftp_info[p_num][num].log_state = 1;   
							
								return 0;
							}
						}
						else
						{
							if (memcmp(payload,KEY_LOGOUT,strlen(KEY_LOGOUT))==0){                    //密码等待输入状态
								ftp_info[p_num][num].log_state = 0;
							//close_mysql(&ftp_cmd_mysql);
								return 0;
							}
							else
							{
								memset(filename[p_num],0,1000);
								memset(tml_file[p_num],0,100);
								memset(dst_filename[p_num],0,1000);
		
								sprintf(tml_file[p_num],"Sql_ftp_%lu_%d",p_info_hd->audit_time,p_num);
								sprintf(filename[p_num],"%s%s",SQL_TMP,tml_file[p_num]);
								sprintf(dst_filename[p_num],"%s%s",SQL_PATH,tml_file[p_num]);
							
								memset(src_ip,0,sizeof(src_ip));
								memset(src_mac,0,sizeof(src_mac));
								memset(des_mac,0,sizeof(des_mac));
								memset(dst_ip,0,sizeof(dst_ip));
								sprintf(src_ip,"%d.%d.%d.%d",(int)UCHARPTR(&p_info_hd->cli_ip)[0],(int)UCHARPTR(&p_info_hd->cli_ip)[1],(int)UCHARPTR(&p_info_hd->cli_ip)[2],(int)UCHARPTR(&p_info_hd->cli_ip)[3]);
								sprintf(dst_ip,"%d.%d.%d.%d",(int)UCHARPTR(&p_info_hd->ser_ip)[0],(int)UCHARPTR(&p_info_hd->ser_ip)[1],(int)UCHARPTR(&p_info_hd->ser_ip)[2],(int)UCHARPTR(&p_info_hd->ser_ip)[3]);
						
								sprintf(src_mac, "%02x-%02x-%02x-%02x-%02x-%02x",
								p_info_hd->cli_mac[0],
								p_info_hd->cli_mac[1],
								p_info_hd->cli_mac[2],
								p_info_hd->cli_mac[3],
								p_info_hd->cli_mac[4],
								p_info_hd->cli_mac[5]);
								sprintf(des_mac, "%02x-%02x-%02x-%02x-%02x-%02x",
								p_info_hd->ser_mac[0],
								p_info_hd->ser_mac[1],
								p_info_hd->ser_mac[2],
								p_info_hd->ser_mac[3],
								p_info_hd->ser_mac[4],
								p_info_hd->ser_mac[5]);
								
								ftp_info[p_num][num].res_len = p_info_hd->payload_len;
								
								pthread_rwlock_rdlock(&tablesTime_lock);
								sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n",p_info_hd->audit_time,tablesTime,5,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,ftp_info[p_num][num].user,ftp_info[p_num][num].cmd,ftp_info[p_num][num].res_len);
								pthread_rwlock_unlock(&tablesTime_lock);
								
								str_sql[p_num][strlen(str_sql[p_num])]='\0';
								
								fp_insert[p_num] = fopen(filename[p_num],"a+");
								if(NULL == fp_insert[p_num])
								{
									printf("The file doesn't exist!\n");
									return -1;
								}
								fwrite(str_sql[p_num],strlen(str_sql[p_num]),1,fp_insert[p_num]);
								fwrite(payload,ftp_info[p_num][num].res_len,1,fp_insert[p_num]);
								fwrite("\n",1,1,fp_insert[p_num]);
								fflush(fp_insert[p_num]);
								fclose(fp_insert[p_num]);//关闭文件
								fp_insert[p_num] = NULL;
								
								int ret;
								ret = rename(filename[p_num], dst_filename[p_num]);
						
								unlink(filename[p_num]);
								
								ftp_info[p_num][num].res_len = 0;
								return 0;
							}
						//其他数据
						
							return 0;
						}
					
					}
				
					else
					{
						if(!memcmp(payload,KEY_DATAEND,strlen(KEY_DATAEND)) )
						{
							ftp_info[p_num][num].end_flag = 1;
							ftp_info[p_num][num].trans_flag = 0;
							//file_info[ftp_info[p_num][num].port_index].file_set = 0;
							//file_info[ftp_info[p_num][num].port_index].res_len = 0;
							
						}
						return 0;
					}
						
				}
					
			}
			else
			{                                                //出流
				//match_user=strstr(payload,KEY_USER);
				if (memcmp(payload,KEY_USER,strlen(KEY_USER))==0){							//用户名获取
					//printf("用户\n");
					memset(ftp_info[p_num][num].user,0,sizeof(ftp_info[p_num][num].user));
					sscanf( payload, "%*s%s", ftp_info[p_num][num].user);
					
				    ftp_info[p_num][num].user[strlen(ftp_info[p_num][num].user)]='\0';                               
					return 0;
				}
				
                //match_pass=strstr(payload,KEY_PASS);            //密码获取
				if (memcmp(payload,KEY_PASS,strlen(KEY_PASS))==0){
				
					memset(ftp_info[p_num][num].pass,0,sizeof(ftp_info[p_num][num].pass));
					sscanf( payload, "%*s%s", ftp_info[p_num][num].pass);
					ftp_info[p_num][num].pass[strlen(ftp_info[p_num][num].pass)]='\0';  
					//strcpy(ftp_info[num].pass ,match_pass+(strlen(KEY_PASS)+1));
					//close_mysql(&ftp_cmd_mysql);
					return 0;
                                
				}
				

				if(ftp_info[p_num][num].log_state)
				{

					int match_port = !memcmp(payload,KEY_PORT,strlen(KEY_PORT));
					int match_pasv = !memcmp(payload,KEY_PASV,strlen(KEY_PASV));
					if(match_port)
					{
						ftp_info[p_num][num].mode_flag = 0;
						ftp_info[p_num][num].port = get_port(payload);
						
						//
						return 0;
					}
					if(match_pasv)
					{
						ftp_info[p_num][num].mode_flag = 1;
						//
						return 0;
					}

				
					int match_file = (!memcmp(payload,KEY_GET,strlen(KEY_GET)))||(!memcmp(payload,KEY_UPLOAD,strlen(KEY_UPLOAD)));
					if(match_file)
					{
						memset(ftp_info[p_num][num].cmd,0,sizeof(ftp_info[p_num][num].cmd));
						//memset(ftp_info[p_num][num].flnm,0,siezof(ftp_info[p_num][num].flnm));
						memcpy(ftp_info[p_num][num].cmd,payload,p_info_hd->payload_len);
						 ftp_info[p_num][num].cmd[strlen(ftp_info[p_num][num].cmd)]='\0'; 
						//sscanf( payload, "%[^ ]%s", ftp_info[p_num][num].flnm);
						
						if(!ftp_info[p_num][num].mode_flag )
						{
							//printf("port\n");
							ftp_info[p_num][num].trans_flag = 1;
							//port_index = get_port(payload);
							port_index = ftp_info[p_num][num].port;
							//printf("port =%d\n",port_index);
							pthread_rwlock_wrlock(&file_info[port_index].ftp_file_lock);
							
							file_info[port_index].f_info.file_set = 1;
							file_info[port_index].f_info.res_len = 0;
							pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
							
							
							
							pthread_rwlock_wrlock(&file_info[port_index].ftp_file_lock);
							file_info[port_index].f_info.user = ftp_info[p_num][num].user;
							file_info[port_index].f_info.file_cmd =ftp_info[p_num][num].cmd;
						
							
							//strcpy(file_info[port_index].f_info.user ,ftp_info[p_num][num].user);
							//strcpy(file_info[port_index].f_info.file_cmd ,ftp_info[p_num][num].cmd);
							
							/*
							strcpy(file_info[port_index].f_info.srcip ,src_ip);
							strcpy(file_info[port_index].f_info.dstip ,dst_ip);
							strcpy(file_info[port_index].f_info.srcmac ,src_mac);
							strcpy(file_info[port_index].f_info.dstmac ,des_mac);
							*/
							pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
							
						}
						else
						{
							ftp_info[p_num][num].trans_flag = 0;
							memset(ftp_info[p_num][num].cmd,0,sizeof(ftp_info[p_num][num].cmd));
							//memset(ftp_info[p_num][num].flnm,0,siezof(ftp_info[p_num][num].flnm));
							memcpy(ftp_info[p_num][num].cmd,payload,p_info_hd->payload_len);
							ftp_info[p_num][num].cmd[strlen(ftp_info[p_num][num].cmd)]='\0'; 
						}
						return 0;
					}
					else
					{
						ftp_info[p_num][num].trans_flag = 0;
						
						memset(ftp_info[p_num][num].cmd,0,sizeof(ftp_info[p_num][num].cmd));
						//memset(ftp_info[p_num][num].flnm,0,siezof(ftp_info[p_num][num].flnm));
						memcpy(ftp_info[p_num][num].cmd,payload,p_info_hd->payload_len);
						 ftp_info[p_num][num].cmd[strlen(ftp_info[p_num][num].cmd)]='\0'; 
						//其他命令(回放时用)
						return 0;
					}
					
					
					return 0;
					
				}
				return 0;

              
        }
        return 0;
}


