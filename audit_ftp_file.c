#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "audit_api.h"
#include "audit_release.h"
#include "audit_database_sql.h"
#include<sys/types.h>

#define FILE_MAX_LEN     10000000
static char filename[THREADS_NUM][1000];
static char tml_file[THREADS_NUM][100];
static char dst_filename[THREADS_NUM][1000];
static char flnm_res[THREADS_NUM][100];

static char str_sql[THREADS_NUM][FILE_MAX_LEN];
static char tmp_res[THREADS_NUM][FILE_MAX_LEN];

FILE *fp_insert[THREADS_NUM];
FILE *fp_res[THREADS_NUM];

//extern pthread_rwlock_t tablesTime_lock;

extern pthread_rwlock_t tablesTime_lock;

extern Taudit_ftp_info ftp_info[THREADS_NUM][AUDIT_INDEX_NUM];
extern char tablesTime[AUDIT_TIME_LEN];


extern Taudit_file_info file_info[AUDIT_INDEX_NUM];

u_int32_t  audit_ftp_file(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char* payload)
{
	if(0==p_info_hd->payload_len)
	{
		return 0;
	}
	
	char 	src_ip[32];
	char	dst_ip[32];
	char 	src_mac[64];
	char	des_mac[64];

	int p_num = p_info_hd->thrid;//线程号
	int port_index;


	if(p_info_hd->cli_port==20)
	{
		port_index = p_info_hd->ser_port;
		
		pthread_rwlock_rdlock(&file_info[port_index].ftp_file_lock);
		if(!file_info[port_index].f_info.file_set)
		{
			pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
			return 0;
			
		}
		pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
	}
	else
	{
		port_index = p_info_hd->cli_port;
		pthread_rwlock_rdlock(&file_info[port_index].ftp_file_lock);
		if(!file_info[port_index].f_info.file_set)
		{
			pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
			return 0;
		}
		pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
	}
	
	pthread_rwlock_rdlock(&file_info[port_index].ftp_file_lock);
	
	if(file_info[port_index].f_info.res_len + p_info_hd->payload_len < (FILE_MAX_LEN-1000))
	{
		pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
		if(NULL==fp_res[p_num])
		{
					
			memset(flnm_res[p_num],0,100);
			sprintf(flnm_res[p_num],"/dev/shm/ftp_tmp/Res_ftp_%lu_%d",p_info_hd->hash,p_num);
			fp_res[p_num] = fopen(flnm_res[p_num],"a+");
			if(NULL == fp_res[p_num])
			{
				printf("The file doesn't exist!\n");	
				return -1;
			}
					
		}
	}
	else
	{
		pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
	}

	fwrite(payload,p_info_hd->payload_len,1,fp_res[p_num]);
	
	
	pthread_rwlock_wrlock(&file_info[port_index].ftp_file_lock);
	file_info[port_index].f_info.res_len += p_info_hd->payload_len;
	
	pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
	
	if(p_info_hd->psh)
	{
		
		memset(filename[p_num],0,1000);
		memset(tml_file[p_num],0,100);
		memset(dst_filename[p_num],0,1000);
		
		sprintf(tml_file[p_num],"Sql_ftp_%lu_%d",p_info_hd->audit_time,p_num);
		sprintf(filename[p_num],"%s%s",SQL_TMP,tml_file[p_num]);
		sprintf(dst_filename[p_num],"%s%s",SQL_PATH,tml_file[p_num]);

		
		fp_insert[p_num] = fopen(filename[p_num],"a+");
		if(NULL == fp_insert[p_num])
								
		{
			printf("The file doesn't exist!\n");
			return -1;
		}
		fseek(fp_res[p_num],0,SEEK_SET);
		fread(tmp_res[p_num],file_info[port_index].f_info.res_len,1,fp_res[p_num]);
		tmp_res[p_num][file_info[port_index].f_info.res_len]='\0';
		fclose(fp_res[p_num]);
		fp_res[p_num] = NULL;
		unlink(flnm_res[p_num]);
		
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
							
					
		char * tables_time ;
		pthread_rwlock_rdlock(&tablesTime_lock);
		tables_time = tablesTime;
		pthread_rwlock_unlock(&tablesTime_lock);
		
		
		pthread_rwlock_rdlock(&file_info[port_index].ftp_file_lock);
		sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n",p_info_hd->audit_time,tables_time,5,src_ip,dst_ip,src_mac,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,file_info[port_index].f_info.user,file_info[port_index].f_info.file_cmd,file_info[port_index].f_info.res_len);
		//sprintf(str_sql[p_num],"rowkey=%lu|colfam1:table=%s|colfam1:app_id=%u|colfam1:src_ip=%s|colfam1:dst_ip=%s|colfam1:src_mac=%s|colfam1:dst_mac=%s|colfam1:src_port=%u|colfam1:dst_port=%u|colfam1:user_name=%s|colfam1:operation_command=%s|colfam1:response_content=%d|colfam\n",p_info_hd->audit_time,tables_time,5,file_info[port_index].f_info.srcip,file_info[port_index].f_info.dstip,file_info[port_index].f_info.srcmac,file_info[port_index].f_info.dstmac,p_info_hd->cli_port,p_info_hd->ser_port,file_info[port_index].f_info.user,file_info[port_index].f_info.file_cmd,file_info[port_index].f_info.res_len);
		pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
		
						
		str_sql[p_num][strlen(str_sql[p_num])]='\0';
		fwrite(str_sql[p_num],strlen(str_sql[p_num]),1,fp_insert[p_num]);
		fwrite(tmp_res[p_num],file_info[port_index].f_info.res_len,1,fp_insert[p_num]);
		fwrite("\n",1,1,fp_insert[p_num]);
		fflush(fp_insert[p_num]);
						
		fclose(fp_insert[p_num]);//关闭文件
		fp_insert[p_num] = NULL;
						
						
							//mv
		int ret;
		ret = rename(filename[p_num], dst_filename[p_num]);
						
		unlink(filename[p_num]);
		pthread_rwlock_wrlock(&file_info[port_index].ftp_file_lock);
		file_info[port_index].f_info.res_len =0;
		file_info[port_index].f_info.file_set=0;
		pthread_rwlock_unlock(&file_info[port_index].ftp_file_lock);
		return 0;
	}
	return 0;
	
}