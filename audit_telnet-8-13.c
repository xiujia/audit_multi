
//update 2015/1/27
//cg



#include "audit_api.h"
#include "audit_release.h"
#include "audit_database_sql.h"
#include "op_db.h"
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

#include <time.h>
#include <mysql/mysql.h>
#include<sys/types.h>
//#include "sunday.h"

#define RES_MAX_LEN     1000000
#define MAX_SQL_NUM		1
//define MAX_SQL_NUM	2
#define AUDIT_MAC_LEN	20
#define KEY_LOGIN 		"login:" 
#define KEY_LOGIN3		"Username:"
#define KEY_PASS 		"Password:"
#define KEY_PASS1		"password:"


#define KEY_QUIT 		"quit"
#define KEY_ENTER 		"\r"
#define KEY_TAB			"\t"
//空格0x20
#define KEY_CMD  		"\r\r\n"
#define KEY_N 			"?"
#define KEY_BACK		"\b"
#define KEY_UP			"\033[A"
#define KEY_DOWN		"\033[B"
#define KEY_RIGHT		"\033[C"
#define KEY_LEFT		"\033[D"
#define KEY_PAGEUP		"\033[5~"
#define KEY_PAGEDOWN		"\033[6~"




static int sql_num[THREADS_NUM] = {0};
static int file_flag[THREADS_NUM] = {0};
static char filename[THREADS_NUM][1000];
static char tml_file[THREADS_NUM][100];
static char dst_filename[THREADS_NUM][1000];
static char flnm_res[THREADS_NUM][100];

char str_sql[THREADS_NUM][RES_MAX_LEN];

FILE *fp_insert[THREADS_NUM];
FILE *fp_res[THREADS_NUM];
//char cmd_tmp[THREADS_NUM][AUDIT_CMD_LEN];

extern pthread_mutex_t telnet_id_mutex;
extern pthread_rwlock_t tablesTime_lock;


extern Taudit_telnet_info telnet_info[THREADS_NUM][AUDIT_INDEX_NUM];
extern char tablesTime[AUDIT_TIME_LEN];


char tmp_res[THREADS_NUM][RES_MAX_LEN];


//Bool match_quit( char *s1, char *s2);
static int get_time(char *cur_time, int len, int flag)
{
    time_t t1;
    struct tm *t2;
    int ret;

    if (time(&t1) < 0) {
        return -1;
    }

    t2 = gmtime(&t1);
    if (0 == flag) {
        ret = strftime(cur_time, len, "%G_%m", t2);
    } else if (1 == flag) {
        ret = strftime(cur_time, len, "%G%m%d%H%M%S", t2);
    } else if (2 == flag) {
        ret = strftime(cur_time, len, "%G-%m-%d %H:%M:%S", t2);
    }
    return ret;
};


u_int32_t audit_telnet(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * payload)
{


int p_num = p_info_hd->thrid;//线程号

u_int32_t num=p_info_hd->hash%AUDIT_INDEX_NUM;

char str_mode[20];

char str_insert[1000];



char mv_tmp[1000];

char  	time_now[AUDIT_TIME_LEN];

char 	tmp[AUDIT_CMD_LEN];
//time_t start,end;
//struct tm *timeinfo;
char cur_time[32];

char 	src_ip[32];
char	dst_ip[32];
char 	src_mac[64];
char	des_mac[64];


const char KEY_LOGIN1[] = {0xd3,0xc3,0xbb,0xa7,0xc3,0xfb,0x3a}; 			//用户名：
const char KEY_PASS2[] = {0xc3,0xdc,0xc2,0xeb,0xfb,0x3a};                  //密码：
const char KEY_LOGIN2[]= {0xc7,0xeb,0xca,0xe4,0xc8,0xeb,0xd5,0xca,0xba,0xc5};//请输入账号
//const char KEY_LOGIN2[]={0xc7,0xeb,0xca,0xe4,0xc8,0xeb,0xb4,0xfa,0xba,0xc5 };//请输入代号
//const char KEY_PASS2[]={0xc7,0xeb,0xca,0xe4,0xc8,0xeb,0xc3,0xdc,0xc2,0xeb};//请输入密码
const char KEY_LOGON[]={0xff,0xfd,0x1f};
const char KEY_LOGON1[]={0xff,0xfd,0x18};
const char KEY_WIN_LOGON[2] = {0xff,0xfd};
//const char KEY_DROP[2]={0xff};
/*
#define KEY_LOGON		0xfffd1f
#define KEY_LOGIN1		0xc7ebcae4c8ebd5cabac5  
#define KEY_LOGIN2		0xc7ebcae4c8ebb4fabac5 
#define KEY_PASS2		"0xc70xeb0xca0xe40xc80xeb0xc30xdc0xc20xeb"	//请输入密码
*/

/*
char insert_head[100];
char insert_tail[100];

sprintf(insert_head ,"use dbmonitor;\nset autocommit=0;\n");
sprintf(insert_tail ,"commit;\n");
insert_head[strlen(insert_head)]='\0';
insert_tail[strlen(insert_tail)]='\0';

*/

if(0==p_info_hd->payload_len)
{
	if(p_info_hd->fin)
	{
		//printf("close!\n");
		p_info->audit_policy_stat = 0;
		return 0;
	}
	return 0;
}
//新流
if (p_info->isnew)
	{
		
		p_info->basic=1;
		p_info->audit_policy_stat = 1;
		pthread_mutex_lock(&telnet_id_mutex);
		telnet_info[p_num][num].id = ++telnet_id; 
		pthread_mutex_unlock(&telnet_id_mutex);
		telnet_info[p_num][num].user_state=0;
		telnet_info[p_num][num].pos=0;
		telnet_info[p_num][num].index=0;
		telnet_info[p_num][num].cmd_flag = 0;
		telnet_info[p_num][num].end_flag = 0;
		telnet_info[p_num][num].mode_flag = 1;

		
		//p_info->out_seq =p_info_hd->seq;
		
	}

//旧流
	//printf("dir==%d---len==%d\n",p_info_hd->isin,p_info_hd->payload_len);
	//入流
	if (p_info_hd->isin)
	{
	
		
		if(!p_info->isnew&&( p_info_hd->seq <= p_info->in_seq ))
		{
			return 0;
		}
		p_info->in_seq =p_info_hd->seq;
		//p_info->in_len = p_info_hd->payload_len;
	
		char *  match_login;
		char * match_login1,*match_login2,* match_login3;
		//match_login=strstr(payload,KEY_LOGIN);
		match_login1=strstr(payload,KEY_LOGIN1);
		match_login2=strstr(payload,KEY_LOGIN2);
		match_login3=strstr(payload,KEY_LOGIN3);
		if(!telnet_info[p_num][num].cmd_flag)
		{
		
			if(p_info_hd->payload_len > strlen(KEY_LOGIN)){
				int offset = p_info_hd->payload_len - strlen(KEY_LOGIN)-1;
				match_login=strstr(payload+offset,KEY_LOGIN);
			
			}
			else
				match_login=strstr(payload,KEY_LOGIN);
		
			if(!telnet_info[p_num][num].log_state&&(match_login3||match_login1||match_login)){
			//if(!telnet_info[num].log_state&&(match_login||match_login1||match_login2||match_login3)){ //用户名输入状态
				//printf("match_login\n");
			memset(telnet_info[p_num][num].user,0,sizeof(telnet_info[p_num][num].user));
			telnet_info[p_num][num].cmd_id = 1; 
			telnet_info[p_num][num].log_state = 1;
			telnet_info[p_num][num].cmd_state = 0;
			return 0;
			}
			char * match_pass=strstr(payload,KEY_PASS);		
			char * match_pass1=strstr(payload,KEY_PASS1);
			char * match_pass2=strstr(payload,KEY_PASS2);
			if(match_pass||match_pass1||match_pass2){ //密码输入状态
				//printf("match_pass\n");
				memset(telnet_info[p_num][num].pass,0,sizeof(telnet_info[p_num][num].pass));
				telnet_info[p_num][num].cmd_state = 0;
				telnet_info[p_num][num].pass_state = 1;
				return 0;
			}
			char * match_cmd_done=strstr(payload,KEY_CMD);
			/*
			if(match_cmd_done){//match_key_r//命令输入完毕（"\r\r\n"）
				//printf("match_cmd_done\n");
				telnet_info[num].cmd_state = 1;//登陆状态或是命令输入完毕状态
			
				memset(telnet_info[num].cmd,0,sizeof(telnet_info[num].cmd));
				return 0;
			}
			*/
			char * match_cmd_logon=strstr(payload,KEY_LOGON);
			char * match_cmd_logon1=strstr(payload,KEY_LOGON1);
			//char * match_cmd_logon2 =strstr(payload,">");
			
			
			//if((telnet_info[num].log_state==2)&&(match_cmd_logon||match_cmd_logon1||!memcmp(">",payload+p_info_hd->payload_len-1,1)))
			if((telnet_info[p_num][num].log_state==2)&&(!memcmp(">",payload+p_info_hd->payload_len-1,1)))
			{
				//printf("match_cmd_logon\n");
				telnet_info[p_num][num].cmd_state = 1;
				memset(telnet_info[p_num][num].cmd,0,sizeof(telnet_info[p_num][num].cmd));
				//memset(telnet_info[p_num][num].res_content,0,sizeof(telnet_info[p_num][num].res_content));
				telnet_info[p_num][num].res_len = 0;
				return 0;
			}
		
		
			return 0;
			
		}
		else
		{
			
		
			if(strlen(payload)>2)
			{
					memset(str_mode,0,20);
					memcpy(str_mode,payload+p_info_hd->payload_len-3,3);
					if (!memcmp(">",str_mode+2,1))
					{
						if(!memcmp(">",str_mode+1,1))
						{
							//printf("mode2\n");
							telnet_info[p_num][num].mode_flag = 0 ;
							if(isdigit(str_mode[0]))
							{
								//printf("not end\n");
								telnet_info[p_num][num].end_flag = 0;
							}
							else
							{
								telnet_info[p_num][num].end_flag = 1;
							}
							
						}
						else
						{
							if(!memcmp("<",payload,1))
							{
								telnet_info[p_num][num].end_flag = 0;
							}
							else
							{
								telnet_info[p_num][num].mode_flag = 1 ;
								telnet_info[p_num][num].end_flag = 1 ;
							}
						}
					}
					else
					{
						telnet_info[p_num][num].end_flag = 0;
					}
				}
				else
				{
					return 0;
				}
		
			if(telnet_info[p_num][num].res_len + p_info_hd->payload_len < (RES_MAX_LEN-1000))
			
			{
				if(NULL==fp_res[p_num])
				{
					
					memset(flnm_res[p_num],0,100);
					sprintf(flnm_res[p_num],"/dev/shm/telnet_tmp/Res_terminal_%lu_%d",p_info_hd->hash,p_num);
					fp_res[p_num] = fopen(flnm_res[p_num],"a+");
					if(NULL == fp_res[p_num])
					{
						printf("The file doesn't exist!\n");	
						return -1;
					}
					
				}
						/*
							lseek(cspFd,p_info->offset, SEEK_SET);
							lseek(cspFd,p_info_hd->seq - p_info->out_seq, SEEK_CUR);
							*/	
				fwrite(payload,p_info_hd->payload_len,1,fp_res[p_num]);
				
				//fflush(fp_res[p_num]);
				// res_len = strlen(telnet_info[p_num][num].res_content);
			
			
				//memcpy(telnet_info[p_num][num].res_content+telnet_info[p_num][num].res_len,payload,p_info_hd->payload_len);
				telnet_info[p_num][num].res_len += p_info_hd->payload_len;
			}
				
				
// 
				if(!telnet_info[p_num][num].end_flag)
				{
					
					return 0;
				}
				
				if(strlen(telnet_info[p_num][num].cmd)>0)
				{
					//printf("res_content:%s\n",res_content);
					
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
					
					
					
					//数据库语句cmd
							//if(0 == file_flag[p_num])						//文件名初始化
							//{
									
									
									
									//time(&start);
									//timeinfo =localtime(&start);
									memset(filename[p_num],0,1000);
									memset(tml_file[p_num],0,100);
									memset(dst_filename[p_num],0,1000);
									
									
									//#if REL_HBASE
									//memset(cur_time, 0, sizeof(cur_time));
									//get_time(cur_time, 32, 1);
									sprintf(tml_file[p_num],"Sql_terminal_%lu_%d",p_info_hd->audit_time,p_num);
									
									//#else
									//sprintf(tml_file[p_num],"Sql_terminal_%s_%d",time_info,p_num);
									//#endif
									sprintf(filename[p_num],"%s%s",SQL_TMP,tml_file[p_num]);
									sprintf(dst_filename[p_num],"%s%s",SQL_PATH,tml_file[p_num]);
									//filename[strlen(filename)]='\0';
									//dst_filename[strlen(filename)]='\0';
									
									
									fp_insert[p_num] = fopen(filename[p_num],"a+");
									if(NULL == fp_insert[p_num])
									{
										printf("The file doesn't exist!\n");	
										return -1;
									}
							
							//		file_flag[p_num] = 1;
									
							//}
								
					if(sql_num[p_num]==MAX_SQL_NUM-1)
					{
					
						
						
						//memset(str_sql[p_num],0,RES_MAX_LEN);
						
						//memset(tmp_res[p_num],0,RES_MAX_LEN);
						
						if(!REL_HBASE)
						{
							memset(time_now,0,sizeof(time_now));
							get_audit_time(time_now);
						
							pthread_rwlock_rdlock(&tablesTime_lock);
							sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n%s\n",p_info_hd->audit_time,tablesTime,3,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,telnet_info[p_num][num].user,telnet_info[p_num][num].cmd,telnet_info[p_num][num].res_len,telnet_info[p_num][num].res_content);
							pthread_rwlock_unlock(&tablesTime_lock);
						}
						else
						{
							
							fseek(fp_res[p_num],0,SEEK_SET);
							fread(tmp_res[p_num],telnet_info[p_num][num].res_len,1,fp_res[p_num]);
							tmp_res[p_num][telnet_info[p_num][num].res_len]='\0';
							fclose(fp_res[p_num]);
							fp_res[p_num] = NULL;
							unlink(flnm_res[p_num]);
							
							pthread_rwlock_rdlock(&tablesTime_lock);
							//sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n%s\n",p_info_hd->audit_time,tablesTime,3,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,telnet_info[p_num][num].user,telnet_info[p_num][num].cmd,telnet_info[p_num][num].res_len,tmp_res[p_num]);
							sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n",p_info_hd->audit_time,tablesTime,3,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,telnet_info[p_num][num].user,telnet_info[p_num][num].cmd,telnet_info[p_num][num].res_len);
							pthread_rwlock_unlock(&tablesTime_lock);
						}
						str_sql[p_num][strlen(str_sql[p_num])]='\0';
						fwrite(str_sql[p_num],strlen(str_sql[p_num]),1,fp_insert[p_num]);
						fwrite(tmp_res[p_num],telnet_info[p_num][num].res_len,1,fp_insert[p_num]);
						fwrite("\n",1,1,fp_insert[p_num]);
						fflush(fp_insert[p_num]);
						
						fclose(fp_insert[p_num]);//关闭文件
						fp_insert[p_num] = NULL;
						
						
						//mv
						int ret;
						ret = rename(filename[p_num], dst_filename[p_num]);
						
						unlink(filename[p_num]);
						
						/*
						char buffer[100];
						memset(buffer,0,sizeof(buffer));
						
						if (ret < 0)
						{
							unlink(filename[p_num]);
							
							sprintf(buffer, "rm -f %s", filename[p_num]);
							system(buffer);
							
							
						}
						*/
					/*//if(!REL_HBASE)
						memset(mv_tmp,0,sizeof(mv_tmp));
						sprintf(mv_tmp,"mv %s %s",tml_file,SQL_PATH);
						system(mv_tmp);
						*/
								/*				
						memset(filename[p_num],0,1000);
						memset(tml_file[p_num],0,100);
						memset(dst_filename[p_num],0,1000);
						//time(&start);
						//timeinfo =localtime(&start);	
						//memset(cur_time, 0, sizeof(cur_time));
						//get_time(cur_time, 32, 1);
						sprintf(tml_file[p_num],"Sql_terminal_%lu_%d",p_info_hd->audit_time,p_num);
						sprintf(filename[p_num],"%s%s",SQL_TMP,tml_file[p_num]);
						sprintf(dst_filename[p_num],"%s%s",SQL_PATH,tml_file[p_num]);
						//filename[strlen(filename)]='\0';
						//dst_filename[strlen(filename)]='\0';
						
						fp_insert[p_num] = fopen(filename[p_num],"a+");
						if(NULL == fp_insert[p_num])
								
						{
							printf("The file doesn't exist!\n");
							return -1;
						}
							*/
					/*	//if(!REL_HBASE)
						memset(str_insert,0,sizeof(str_insert));
						fwrite(insert_head,strlen(insert_head),1,fp_insert[p_num]);
						pthread_rwlock_rdlock(&tablesTime_lock);
						sprintf(str_insert,"insert into cache_monitor_data_%s(capture_time, app_id,src_ip,dst_ip,src_mac,dst_mac,src_port,dst_port,user_name,operation_command,response_content) VALUES",tablesTime);
						pthread_rwlock_unlock(&tablesTime_lock);
						str_insert[strlen(str_insert)]='\0';
						fwrite(str_insert,strlen(str_insert),1,fp_insert[p_num]);
						fflush(fp_insert[p_num]);
					*/
						sql_num[p_num] = 0;
						telnet_info[p_num][num].res_len =0;
			
					}
					else
					{	
					
						sql_num[p_num]++;
						memset(str_sql[p_num],0,RES_MAX_LEN);
						memset(tmp_res[p_num],0,RES_MAX_LEN);
						//appid =3;
						if(!REL_HBASE)
						{
						memset(time_now,0,sizeof(time_now));
						get_audit_time(time_now);
						
							pthread_rwlock_rdlock(&tablesTime_lock);
							sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n%s\n",p_info_hd->audit_time,tablesTime,3,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,telnet_info[p_num][num].user,telnet_info[p_num][num].cmd,telnet_info[p_num][num].res_len,telnet_info[p_num][num].res_content,tmp_res[p_num]);
							pthread_rwlock_unlock(&tablesTime_lock);
						}
						else
						{
							
							fseek(fp_res[p_num],0,SEEK_SET);
							fread(tmp_res[p_num],telnet_info[p_num][num].res_len,1,fp_res[p_num]);
							tmp_res[p_num][telnet_info[p_num][num].res_len]='\0';
							fclose(fp_res[p_num]);
							fp_res[p_num] = NULL;
							unlink(flnm_res[p_num]);
							
							pthread_rwlock_rdlock(&tablesTime_lock);
							//sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n%s\n",p_info_hd->audit_time,tablesTime,3,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,telnet_info[p_num][num].user,telnet_info[p_num][num].cmd,telnet_info[p_num][num].res_len,tmp);
							sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n",p_info_hd->audit_time,tablesTime,3,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,telnet_info[p_num][num].user,telnet_info[p_num][num].cmd,telnet_info[p_num][num].res_len);
							pthread_rwlock_unlock(&tablesTime_lock);
						}
						//str_sql[strlen(str_sql)]='\0';
						str_sql[p_num][strlen(str_sql[p_num])]='\0';
						fwrite(str_sql[p_num],strlen(str_sql[p_num]),1,fp_insert[p_num]);
						fwrite(tmp_res[p_num],telnet_info[p_num][num].res_len,1,fp_insert[p_num]);
						fwrite("\n",1,1,fp_insert[p_num]);
						fflush(fp_insert[p_num]);
						
						
					}
					
					memset(telnet_info[p_num][num].cmd,0,sizeof(telnet_info[p_num][num].cmd));
					//memset(telnet_info[p_num][num].res_content,0,sizeof(telnet_info[p_num][num].res_content));
					telnet_info[p_num][num].cmd_id++;
					telnet_info[p_num][num].end_flag = 0;
					telnet_info[p_num][num].res_len =0;
					
					return 0;
				}
				else
				{
					memset(telnet_info[p_num][num].cmd,0,sizeof(telnet_info[p_num][num].cmd));
					//memset(telnet_info[p_num][num].res_content,0,sizeof(telnet_info[p_num][num].res_content));
					telnet_info[p_num][num].end_flag =0;
					telnet_info[p_num][num].res_len =0;
					return 0;
				}

				return 0;
			
			
		}
		
	}
	
	//出流
	else
	{
		//if(!p_info_hd->isin){
	
		if(!p_info->isnew&&(p_info_hd->seq <= p_info->out_seq ))
		{
			return 0;
		}
		p_info->out_seq = p_info_hd->seq;
		//p_info->out_len = p_info_hd->payload_len;
		//#if 0
		if(payload[0] == 0xff)
		{
			return 0;
		}
		//#endif
		
		char * match_enter=strstr(payload,KEY_ENTER);
		
		//printf("55555555555555556\n");
		//printf("telnet_info[num].cmd_state = %d\n",telnet_info[num].cmd_state);
		if(!telnet_info[p_num][num].cmd_state)   //未登录
		{
			//printf("telnet_info[num].log_state = %d\n",telnet_info[num].log_state);
			if (telnet_info[p_num][num].log_state == 1)                //用户名输入状态
			{
				if (!match_enter){                                       //用户名未输完
					//printf("!match_enter\n");
					//telnet_info[num].user = strcat(telnet_info[num].user,payload);
					
					
					
					int len = strlen(telnet_info[p_num][num].user);
					//printf("len=%d\n",len);
					
					if(strstr(payload,KEY_BACK)){
						telnet_info[p_num][num].user[len-1]=0x00;
						return 0;
						}
					else{
                 					memcpy(telnet_info[p_num][num].user+len ,payload ,p_info_hd->payload_len);
						return 0;
					}
				}
				else{
					/*
					int len = strlen(telnet_info[num].user);
					telnet_info[num].user[len]=0x00;
					*/
					//printf("match_enter\n");
					//printf("match_enter user_state = %d\n",telnet_info[num].user_state);
					telnet_info[p_num][num].log_state = 2;                         //输完用户名
					
					
						telnet_info[p_num][num].user_state=1;
						return 0;
					//}
					
					
					
					}
			}	
			if (telnet_info[p_num][num].pass_state == 1){                             //密码输入状态
				if(!match_enter){  //密码未输完
					//telnet_info[num].pass = strcat(telnet_info[num].pass,payload);
					
					int len = strlen(telnet_info[p_num][num].pass);
                 	memcpy(telnet_info[p_num][num].pass+len ,payload ,p_info_hd->payload_len);
					return 0;
					}
				else{                                                                //输完密码
					telnet_info[p_num][num].pass_state = 2;

					return 0;
					}
			}
		}
		else{ 																			//登陆
		
		
			if(!match_enter)                                                     //命令未输完
			{	
				telnet_info[p_num][num].cmd_flag = 0; //重置标示
				
				if(payload[0] == 0xff){
					return 0;
				}
				//printf("99999\n");
				//如果长度大于1，为粘贴直接往后加
				int len = strlen(telnet_info[p_num][num].cmd);
				if(p_info_hd->payload_len<2)//p_info_hd->payload_len
				{

					if (memcmp(telnet_info[p_num][num].cmd,KEY_N,sizeof(KEY_N))==0)                             //如果命令为问号
						{	
							memset(telnet_info[p_num][num].cmd,0,sizeof(telnet_info[p_num][num].cmd));
						}
					else{
					
					
					
					if(strstr(payload,KEY_BACK))
						{
							memset(tmp,0,sizeof(tmp));
							memcpy(tmp,telnet_info[p_num][num].cmd+len-telnet_info[p_num][num].index,telnet_info[p_num][num].index);
							//printf("%s\n",tmp);
							memcpy(telnet_info[p_num][num].cmd+len-telnet_info[p_num][num].index-1 ,tmp ,strlen(tmp));
							
							telnet_info[p_num][num].cmd[len-1]='\0';
							return 0;
						}
						
					
					}
				}
				
					
						
					if(strstr(payload,KEY_LEFT))
						{
							telnet_info[p_num][num].index++;
							if(telnet_info[p_num][num].index>len)
								telnet_info[p_num][num].index=len;
							
							return 0;
						}
					if(strstr(payload,KEY_RIGHT))
						{
							telnet_info[p_num][num].index--;
							if(telnet_info[p_num][num].index<0)
								telnet_info[p_num][num].index=0;
							
							return 0;
							
						}
					
					//(>> || >)
					
					memset(tmp,0,sizeof(tmp));
					
					memcpy(tmp,telnet_info[p_num][num].cmd+len-telnet_info[p_num][num].index,telnet_info[p_num][num].index);
							
					memcpy(telnet_info[p_num][num].cmd+len-telnet_info[p_num][num].index,payload,p_info_hd->payload_len);
					
                 	memcpy(telnet_info[p_num][num].cmd+len-telnet_info[p_num][num].index+p_info_hd->payload_len ,tmp ,strlen(tmp));
					//printf("cmd==%s\n",telnet_info[num].cmd);

					return 0;
				
						
					
				
			}//命令已输完
			
				//命令输完，记录回包标示
				if(telnet_info[p_num][num].mode_flag==0)
				{
					//memcpy(telnet_info[p_num][num].cmd+strlen(telnet_info[p_num][num].cmd),payload,p_info_hd->payload_len);
					memcpy(telnet_info[p_num][num].cmd+strlen(telnet_info[p_num][num].cmd)," ",1);
				}

					telnet_info[p_num][num].cmd_flag = 1;
				
				//printf("00000\n");
				//memcpy(telnet_info[num].cmd,payload,strlen(payload));
				//上下操作
				/*
				if(strstr(telnet_info[num].cmd,KEY_UP)||strstr(telnet_info[num].cmd,KEY_DOWN))
				{
					memset(telnet_info[num].cmd,0,sizeof(telnet_info[num].cmd));
				}
				*/
				//get_audit_time(time_now);
				telnet_info[p_num][num].pos=0;
				telnet_info[p_num][num].index=0;
				
				
				return 0;
					
			}
		}
		
		return 0; 
}
	/*
	return 0;
}

Bool match_quit( char *s1, char *s2){                  //匹配结束命令
	if (!strncasecmp(s1,s2,strlen(s1)))
		return 1;
	else
		return 0;
}//linux 下的memicmp
*/



