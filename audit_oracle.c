#include<ctype.h>
#include<stdio.h>
#include<time.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<unistd.h>
#include"audit_api.h"
#include"audit_database_sql.h"
#include "audit_time_api.h"

char oracle_file[THREADS_ORACLE_NUM][200];

int AuditWriteOffset(char *fname,off_t offset,unsigned char *data,int len)
{	
	int fd;
	int written;
	fd=open(fname, O_RDWR | O_CREAT, 0666);
	if(fd <=  2) return -1;
	lseek(fd,offset,SEEK_SET);
	written = AuditWrite(fd,data,len);
	close(fd);
	return  written;
}
u_int32_t audit_oracle(struct audit_pack_info_head *p_info_hd, struct audit_pack_info *p_info, unsigned char * data){
	int len = 0;
	int file_count = 0;
	char oracle_file_path[] = "/dev/shm/oracle/";
	int out_offset = 0,in_offset = 0;
	char oracle_filename[1024]={0};
	char smac[20];//xxxxxxxxxxxx
	char dmac[20];
	//p_info->flag == 1/0  开始为0 碰到另一个方向 置1  碰到另一个方向置0 交替往复
    len = p_info_hd->payload_len;
    if (len <= 0) return 0;
/*
	//去除连续重复包
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
*/
	sprintf(smac,"%02x%02x%02x%02x%02x%02x",p_info_hd->cli_mac[0],p_info_hd->cli_mac[1],p_info_hd->cli_mac[2],p_info_hd->cli_mac[3],p_info_hd->cli_mac[4],p_info_hd->cli_mac[5]);
	sprintf(dmac,"%02x%02x%02x%02x%02x%02x",p_info_hd->ser_mac[0],p_info_hd->ser_mac[1],p_info_hd->ser_mac[2],p_info_hd->ser_mac[3],p_info_hd->ser_mac[4],p_info_hd->ser_mac[5]);
	

//	printf("isin = %d\n",p_info_hd->isin);
	//out 0
	if(!p_info_hd->isin){
		
		if(p_info->isnew){
			p_info->flag = 1;
			p_info->isnew = 0;
			p_info->file_num = 0;
		}
		if(p_info->flag == 1){ //request file
			//get new nano time
			//判断data is_oracle_odbc 1是 2 不是 p_info->type_flag 标记
			p_info->file_num++;
			p_info->type_flag = is_oracle_odbc(data,p_info_hd,p_info);

			//通过 type_flag  判断策略 是否留存
			
			p_info->wflag = audit_oracle_type_match(p_info->type_flag,p_info->policy_id,p_info_hd->policy);
			if(p_info->wflag != 1) {
				p_info->flag = p_info_hd->isin;
				return 0;
			}
			p_info->nanotime = get_usec_time();
			p_info->out_seq = p_info_hd->seq;
			p_info->in_offset = 0;
			p_info->out_offset = 0;
			memset(oracle_file[p_info_hd->thrid-1],0,200);
			sprintf(oracle_file[p_info_hd->thrid-1],"%s%d/%llu_%lu_%d_%lu_%s_%hu_%lu_%s_%hu_%d_%lu_request",oracle_file_path,p_info_hd->thrid,p_info->nanotime,p_info_hd->hash,p_info->type_flag,p_info_hd->cli_ip,smac,p_info_hd->cli_port,p_info_hd->ser_ip,dmac,p_info_hd->ser_port,p_info_hd->app_id,p_info->file_num);
		//	printf("request:%s\n",oracle_filename);
		}
			if(p_info->wflag!=1){
				return 0;
			}
		
		
		memset(oracle_file[p_info_hd->thrid-1],0,200);
		sprintf(oracle_file[p_info_hd->thrid-1],"%s%d/%llu_%lu_%d_%lu_%s_%hu_%lu_%s_%hu_%d_%lu_request",oracle_file_path,p_info_hd->thrid,p_info->nanotime,p_info_hd->hash,p_info->type_flag,p_info_hd->cli_ip,smac,p_info_hd->cli_port,p_info_hd->ser_ip,dmac,p_info_hd->ser_port,p_info_hd->app_id,p_info->file_num);
		p_info->out_offset = p_info_hd->seq - p_info->out_seq;
		AuditWriteOffset(oracle_file[p_info_hd->thrid-1],p_info->out_offset,data,len);
		p_info->flag = p_info_hd->isin;
		
	}
	else{//in  1
		if(p_info->isnew){
			return 0;
		}
		
		if(p_info->flag == 0){ //response file 
			if(p_info->wflag !=1){
				p_info->flag = p_info_hd->isin;
				return 0;
			}
			p_info->in_seq = p_info_hd->seq; //变向
			
			memset(oracle_file[p_info_hd->thrid-1],0,200);
			sprintf(oracle_file[p_info_hd->thrid-1],"%s%d/%llu_%lu_%d_%lu_%s_%hu_%lu_%s_%hu_%d_%lu_response",oracle_file_path,p_info_hd->thrid,p_info->nanotime,p_info_hd->hash,p_info->type_flag,p_info_hd->cli_ip,smac,p_info_hd->cli_port,p_info_hd->ser_ip,dmac,p_info_hd->ser_port,p_info_hd->app_id,p_info->file_num);
			//printf("response:%s\n",oracle_filename);
		}
			if(p_info->wflag!=1){
				return 0;
			}
			
		memset(oracle_file[p_info_hd->thrid-1],0,200);
		sprintf(oracle_file[p_info_hd->thrid-1],"%s%d/%llu_%lu_%d_%lu_%s_%hu_%lu_%s_%hu_%d_%lu_response",oracle_file_path,p_info_hd->thrid,p_info->nanotime,p_info_hd->hash,p_info->type_flag,p_info_hd->cli_ip,smac,p_info_hd->cli_port,p_info_hd->ser_ip,dmac,p_info_hd->ser_port,p_info_hd->app_id,p_info->file_num);
		p_info->in_offset =p_info_hd->seq - p_info->in_seq;
		
 		AuditWriteOffset(oracle_file[p_info_hd->thrid-1],p_info->in_offset,data,len);
 		p_info->flag = p_info_hd->isin;
	}
	
}

