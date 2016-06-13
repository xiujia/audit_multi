#include <stdio.h>

#include "audit_api.h"
#include "audit_policy.h"

Taudit_policy_mem * policy_shmem;
//Taudit_policy_mem * audit_policy_share_mem;
Taudit_policy_file audit_policy_file_tmp;

Taudit_policy_mem * create_audit_policy_shmem(int * rret){
	int shm_size,ret;
	Taudit_policy_mem * pm;
	  shm_size=sizeof(Taudit_policy_mem);
//	  ret = shmget(AUDIT_POLICY_SHMEM, shm_size, IPC_CREAT); 
//   	 shmctl(ret, IPC_RMID, 0 );
    	ret = shmget(AUDIT_POLICY_SHMEM, shm_size, 0666|IPC_CREAT); 
	*rret = ret;
	if(ret == -1){
		fprintf(stderr,"shmget struct Taudit_policy_mem create  error 1\n");
		return NULL;
	}
    	pm= ( Taudit_policy_mem *)shmat( ret, 0, 0 );
	return pm;
}
Taudit_policy_mem * get_audit_policy_shmem(){
	int shm_size,ret;
	Taudit_policy_mem * pm;
	  shm_size=sizeof(Taudit_policy_mem);
    	ret = shmget(AUDIT_POLICY_SHMEM, shm_size, 0666|IPC_EXCL); 
	if(ret == -1){
		fprintf(stderr,"shmget struct Taudit_policy_mem get  error 1\n");
		return NULL;
	}

    	pm= ( Taudit_policy_mem *)shmat( ret, 0, 0 );
	return pm;
}




inline void audit_policy_init(Taudit_policy_mem * p){
	u_int32_t  i, j;
	for(i =0 ; i < MAX_AUDIT_POLICY; i++){
		p->audit_policy[i].link_id = 0;
		p->audit_policy[i].policy_type = 0;
		p->audit_policy[i].policy_id = 0;
		for(j=0;j<8192*8;j++){//ip
			bit_clr(j,p->audit_policy[i].policy_str.sip);
	//		bit_clr(j,p->audit_policy[i].policy_str.dip);
		}
		for(j=0;j<10081;j++){//time
			bit_clr(j,p->audit_policy[i].policy_str.time);
		}
		for(j=0;j<32;j++){//10种需要审计
			bit_clr(j,p->audit_policy[i].policy_str.type);
		}
	}		
}





//add_policy interfaces

inline u_int32_t audit_policy_ip_map(u_int32_t policy_ip,u_int16_t  n){
	u_int8_t  ip_addr[20];
	u_int32_t ip_map_addr, iip, iip0, iip1, iip2, iip3;
	
	iip=htonl(policy_ip);	
	sprintf(ip_addr, "%s", inet_ntoa(*(struct in_addr *)&iip));
	sscanf(ip_addr, "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);

	if(3 == n) {
		ip_map_addr = iip2*256*256 + iip1*256 + iip0;
	} else if(2 == n){
		ip_map_addr = iip1*256 + iip0;
	}
	//printf("ip_start:%s--->%d.%d.%d.%d-->map:%d\n",ip_addr,iip3,iip2,iip1,iip0,ip_map_addr);
	
	return(ip_map_addr);	
}
inline  void audit_policy_ip_deal(char *content,  int ip_type){
	
	//int8_t ip_start[20],ip_end[20];
		u_int32_t  i, ii, ip, startip, endip, mask, level1_ip_bitnum;
		u_int32_t  ip_bitmap=0,ret=0;
		int8_t *p = NULL;
		
	
		//printf("content:%s,ip_type:%d\n",content,ip_type);
		//memset(ip_start,0,sizeof(ip_start));
		//memset(ip_end,0,sizeof(ip_end));
	
		if((p = strchr(content, '-')) != NULL){
	
			*p = '\0';
			startip = ntohl(inet_addr(content));
			endip = ntohl(inet_addr(p+1));
			//printf("startip=%u,endip=%u\n",startip,endip);
	
			//sscanf(content,"%[^-]-%s",ip_start,ip_end);
			//startip = ntohl(inet_addr(ip_start));
			//endip = ntohl(inet_addr(ip_end)); 
			//printf("startip=%u,endip=%u\n",startip,endip);		
			for(i=startip;i<=endip;i++)
			{
				if(0 == ip_type) {
			//		policy_file_tmp.ip_count++;
					//ii=htonl(i);
				//printf("i=%u-->%s\n",i,inet_ntoa(*(struct in_addr *)&ii));
					ip_bitmap = audit_policy_ip_map(i, 2);
					bit_set(ip_bitmap,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.sip);//policy_1[policy_file_tmp.policy_prior].policy_str.src_ip);
					//printf("--%d\n",bit_test(ip_bitmap,policy_share_cache->policy_1[policy_file_tmp.policy_id].policy_str.src_ip));		
				}
				else {
					ip_bitmap = audit_policy_ip_map(i, 2);
					bit_set(ip_bitmap,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.dip);					
				}
			}
		}
		else if((p = strchr(content, '/')) != NULL){
			*p = '\0';
			ip = ntohl(inet_addr(content));
			mask = (u_int32_t)(0xffffffff << (sizeof(u_int32_t)*8 - atoi(p + 1)));
			startip = ip & mask;
			endip = ip | (~mask);
			//printf("startip=%u,endip=%u\n",startip,endip);
			for(i=startip;i<=endip;i++)
			{
				if(0 == ip_type) {
					//policy_file_tmp.ip_count++;
					//ii=htonl(i);
				//printf("i=%u-->%s\n",i,inet_ntoa(*(struct in_addr *)&ii));
					ip_bitmap = audit_policy_ip_map(i, 2);
					bit_set(ip_bitmap, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.sip);			
				}
				else {
					ip_bitmap = audit_policy_ip_map(i, 2);
					bit_set(ip_bitmap, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.sip);					
				}
			}
		}
		else{
			if(0 == ip_type) {//in user
				if(1 == strlen(content)) {//all user!
					//ret = local_user_range();
					//if(0 == ret) {//no local user!
						for(ip_bitmap=0; ip_bitmap<8192*8; ip_bitmap++) {
								bit_set(ip_bitmap, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.sip);
								//printf("--%d\n",bit_test(level1_ip_bitnum,policy_share_cache->policy_1[policy_file_tmp.policy_id].policy_str.src_ip));	
						}
					//}
				}
				else {//single user
					//policy_file_tmp.ip_count++;
					ip = ntohl(inet_addr(content));
					printf("ip_start:%s--->%u\n",content,ip);
					ip_bitmap = audit_policy_ip_map(ip, 2);
					printf("%d,%d,%d\n",ip_bitmap,ip_bitmap/8,ip_bitmap%8);
					bit_set(ip_bitmap, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.sip);	
					//printf("prior=%d|policy_id:%d=policy_id:%d",policy_file_tmp.policy_prior,policy_file_tmp.policy_id,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_id);
					//printf("bittest--%d\n",bit_test(ip_bitmap,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.src_ip)); 
				}
			}
			else {//ex user
				if(1 == strlen(content)) {//all user!
					for(ip_bitmap=0; ip_bitmap<8192*8; ip_bitmap++) {
							bit_set(ip_bitmap, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.dip);
					}
				}
				else {//single user
					//policy_file_tmp.ip_count++;
					ip = ntohl(inet_addr(content));
					//printf("ip_start:%s--->%u\n",content,ip);
					ip_bitmap = audit_policy_ip_map(ip, 2);
					bit_set(ip_bitmap, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.dip);	
				}
			}
		}
}

inline void audit_policy_type_deal(char * content){
	u_int16_t app_id, i;
	
	app_id = atoi(content);
	//printf("app_id=%d\n",app_id);

	if(0 != app_id) {	
			bit_set(app_id,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.type);
	} else {//all

			for(i=0;i<4*8;i++) {
				bit_set(i,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.type);
				//printf("appid=%d---appid_map=%d\n",i,bit_test(app_id,policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.app));
			}
	} 

}

inline void audit_policy_time_deal(char * content){
	
		int8_t timestart[10], timeend[10];
		int32_t time_span, time_start, time_end;
		
		sscanf(content,"%[^-]-%s",timestart,timeend);
		time_start = atoi(timestart);
		time_end   = atoi(timeend);
		//printf("time_start=%d,time_end=%d\n",time_start,time_end);	
		
		for(time_span=time_start; time_span<=time_end; time_span++) 
		{
			bit_set(time_span, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.time);
			//bit_set(time_span, policy_shmem->audit_policy[audit_policy_file_tmp.policy_id].policy_str.time);
			//printf("time_span=%d--time_map=%d\n",time_span, bit_test(time_span, policy_share_cache->policy_1[policy_file_tmp.policy_id].policy_str.time));
		}
}
inline void audit_policy_file_content_deal(char *type, char *content){
	if(strncmp(AUDIT_P_ID,type,strlen(AUDIT_P_ID)) == 0){
		audit_policy_file_tmp.policy_id = atoi(content) -1  ;
		printf("id = %u\n",audit_policy_file_tmp.policy_id);
		policy_shmem->policy_count = audit_policy_file_tmp.policy_id+1;
	}
	else if(strncmp(AUDIT_P_LINK,type,strlen(AUDIT_P_LINK)) == 0){
		audit_policy_file_tmp.type = atoi(content)-1;
		printf("link_id = %s\n",content);
	}
	else if(strncmp(AUDIT_P_INTIP,type,strlen(AUDIT_P_INTIP))==0){
		audit_policy_ip_deal(content,0);
		printf("intip = %s\n",content);
	}
/*	else if(strncmp(AUDIT_P_EXTIP,type,strlen(AUDIT_P_EXTIP))==0){
		audit_policy_ip_deal(content,1);
		printf("extip = %s\n",content);
	}
	*/
	else if(strncmp(AUDIT_P_TYPE,type,strlen(AUDIT_P_TYPE))==0){
		audit_policy_type_deal(content);
		printf("audittype = %s\n",content);
	}	
	else if(strncmp(AUDIT_P_TIME,type,strlen(AUDIT_P_TIME))==0){
		audit_policy_time_deal(content);
		printf("time = %s\n",content);
	}
}

Bool audit_policy_add(){
	FILE * audit_policy_fp;
	audit_policy_fp = fopen(AUDIT_P_PATH,"r");
	char policy_buff[AUDIT_POLICY_FILE_READ_BLOCK],type[20],content[AUDIT_POLICY_FILE_READ_BLOCK];
	Taudit_policy_mem *audit_p_mem;
	 u_int16_t audit_policy_num = 0,audit_policy_count = 0;
	 u_int16_t file_line = 0,audit_policy_line = 0;
//	Taudit_policy_mem * policy_mem;
	policy_shmem = get_audit_policy_shmem();
	if(policy_shmem == NULL) {
		printf("get audit policy shmem failed.\n");
		return 0;

	}
	audit_policy_init(policy_shmem);
	
	if(!audit_policy_fp){
		fprintf(stderr,"open audit policy config file failed .\n");
		return 0;
	}
//	(audit_policy_count = 0; audit_policy_count < MAX_AUDIT_POLICY ; audit_policy_count++)
	audit_policy_count = 0;
	while(1){
			if(audit_policy_count == MAX_AUDIT_POLICY) break;
			memset(policy_buff,0,sizeof(policy_buff));
			memset(type,0,sizeof(type));
			memset(content,0,sizeof(content));
			if(fgets(policy_buff,AUDIT_POLICY_FILE_READ_BLOCK,audit_policy_fp)!=NULL)//读取一行
			{
				file_line++;
				if(strncmp(AUDIT_P_START,policy_buff,strlen(AUDIT_P_START)) == 0){
					audit_policy_count++;
					//每个策略的行数
					for(audit_policy_line = 0; audit_policy_line < AUDIT_POLICY_LINE_NUM; audit_policy_line++){
						memset(policy_buff,0,sizeof(policy_buff));
						memset(type,0,sizeof(type));
						memset(content,0,sizeof(content));
						if(fgets(policy_buff,AUDIT_POLICY_FILE_READ_BLOCK,audit_policy_fp)!=NULL){
							file_line++;
							if(strncmp(AUDIT_P_END,policy_buff,strlen(AUDIT_P_END)) == 0){//结束跳出当前策略循环
								break;
							}
							else{
								sscanf(policy_buff,"%[^=]=%s",type,content);
								//process content
								 audit_policy_file_content_deal(type, content);
							}
						}//end if fgets policy_buff
					}//end for audit_policy_line
				}
				else{//no AUDIT_P_START
					break;
				}
			}
			else
			{
				break;
			}

	}
//	policy_shmem->policy_count = audit_policy_count;
	if(policy_shmem->policy_count  > 0)
		policy_shmem->policy_stat = 1;//1代表有策略并且执行过
	else
		policy_shmem->policy_stat = 0;//0执行过 但无策略2代表一直有策略
	printf("policy_shmem->policy_stat = %d",policy_shmem->policy_stat);
	fclose(audit_policy_fp);
	shmdt((const void *)policy_shmem);
	return 1;
}

Bool audit_policy_del(){//清楚所有位图即可
		int i;
//		Taudit_policy_mem * policy_mem;
		policy_shmem= get_audit_policy_shmem();
		if(policy_shmem == NULL) {
			printf("get_audit_policy_shmem failed.\n");
			return 0;
		} 
		policy_shmem->policy_count = 0;
			for(i=1; i<MAX_AUDIT_POLICY; i++ ) {//init policy 
				
		/*		if(i > policy_user_share_cache->policy1_count+4) {
					if(0 == policy_user_share_cache->policy_1[i].policy_id) {
						break;
					}
				}
		*/
				policy_shmem->audit_policy[i].policy_id =0;
		
				policy_shmem->audit_policy[i].link_id = 0;
				policy_shmem->audit_policy[i].policy_type = 0;
				
				memset(policy_shmem->audit_policy[i].policy_str.sip,0,8192);
				memset(policy_shmem->audit_policy[i].policy_str.dip,0,8192);
				memset(policy_shmem->audit_policy[i].policy_str.type,0,4);
				memset(policy_shmem->audit_policy[i].policy_str.time,0,1261);
			}
			shmdt((const void *)policy_shmem);
			return 1;
}



