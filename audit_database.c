#include "audit_database.h"
#include "audit_api.h"


/*
int CspGetUsername(int type ,IN char * payload, OUT char * depart, OUT char * username){

	switch(type){
		case AUDIT_CACHE_CSP:
			
			return 0;
		case AUDIT_CACHE_CSP_PORTAL:

			return 0;
		default:
			return 0;
			
	}




}
*/


pthread_mutex_t  csp_id_mutex;
unsigned long GetId(char * filename){
	FILE * fp;
	char id[40] = {0};
	char *pos;
	unsigned long a = 0;
	memset(id,0,sizeof(id));
	fp = fopen(filename,"r");
	if(fp){
		if(fgets(id,30,fp)!=NULL){
			pos = strstr(id,"\n");
			if(pos){
				*pos = '\0';
			}
			a = strtoul(id,0,10);
		}
		fclose(fp);
	}
	return a;
}

void SetId(char * filename,unsigned long id){
	FILE * fp;
	fp = fopen(filename,"w");
	if(fp){
		fprintf(fp,"%lu",id);
		fflush(fp);
		fclose(fp);
	}
}

static void CspGetIp(OUT char * ip, IN u_int32_t int_ip){
	sprintf(ip,"%d.%d.%d.%d",(int) UCHARPTR(&int_ip)[0],(int) UCHARPTR(&int_ip)[1],(int)UCHARPTR(&int_ip)[2],(int)UCHARPTR(&int_ip)[3]);
}

#define  CspGetMac(strmac, netmac)   \
	sprintf(strmac, "%02x-%02x-%02x-%02x-%02x-%02x",netmac[0],netmac[1],netmac[2],netmac[3],netmac[4],netmac[5]);


#define CspGetPath(a,b,c)  \
	sprintf((a),"%s%lu.tmp",(b),(c));
	
#if 1
int CspPackRefrom(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * payload){
	char * data;
	char cspFilePathTmp[AUDIT_PATH_LEN] = {0},cspFilePath[AUDIT_PATH_LEN] = {0};
	char times[AUDIT_TIME_LEN] = {0},userip[AUDIT_IPADDR_LEN] = {0},desip[AUDIT_IPADDR_LEN]={0},src_mac[AUDIT_MAC_LEN] = {0},des_mac[AUDIT_MAC_LEN]={0};
	char conStr[AUDIT_PACK_LEN]={0};
	char requestUrl[AUDIT_URL_LEN];
	int cspFd;
	int type;
	int flag;
	int dir = 0;
	unsigned int policy_time;
	unsigned char * pos = NULL;
	int str_len = 0;
	int len = p_info_hd->payload_len;
#if CSP_DEBUG
	CspDebug( p_info_hd,p_info,payload);
#endif

//	printf("seq:%u\n",p_info_hd->seq);
         if(p_info_hd->seq > 4294967295) return 0;
	dir = p_info_hd->hash%10;

	if(p_info->audit_type == AUDIT_CACHE_CSP_HIS){
		type = AUDIT_CSP_TYPE_HIS;
	}
	else if(p_info->audit_type == AUDIT_CACHE_CSP_PORTAL){
		type = AUDIT_CSP_TYPE_PORTAL;
	}
	
	if(p_info_hd->isin){
		if( p_info->flag ==1){
			if(memcmp(payload,"HTTP/1.",7)==0)
			{
			
			char sip[100],dip[100];
			
			sprintf(dip,"%d.%d.%d.%d",(int)UCHARPTR(&p_info_hd->ser_ip)[0],(int)UCHARPTR(&p_info_hd->ser_ip)[1],(int)UCHARPTR(&p_info_hd->ser_ip)[2],(int)UCHARPTR(&p_info_hd->ser_ip)[3]);
	//	if(p_info->audit_type == AUDIT_CACHE_CSP_PORTAL)
	//	printf("HTTP  %d hash:%lu serpt:%u dip:%s seq:%u\n",dir,p_info_hd->hash,p_info_hd->ser_port,dip,p_info_hd->seq);
							
				p_info->in_seq = p_info_hd->seq;
				p_info->flag =2;
				memset(cspFilePathTmp,0,sizeof(cspFilePathTmp));
//				CspGetPath(cspFilePathTmp,AUDIT_TMP_CSP_PATH,p_info->id);
				sprintf(cspFilePathTmp,"%s%d/P_%lu",AUDIT_TMP_CSP_PATH,dir,p_info->id);
				cspFd = open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
				if(cspFd < 2) return 0;
				p_info->in_offset = AuditWrite(cspFd,payload,len);
				if(p_info->in_offset < 0){
					p_info->in_offset=0;
					close(cspFd);
					return 0;
				}
				close(cspFd);

			
				return 0;
			}
			return 0;
		}
		else if( p_info->flag == 2){
			if(memcmp(payload,"HTTP/1.",7)==0){
				return 0;
			}
			
		//	printf("intseq:%u,seq:%u,len :%u\n",p_info->in_seq,p_info_hd->seq,p_info_hd->seq-p_info->in_seq);
			memset(cspFilePathTmp,0,sizeof(cspFilePathTmp));
			sprintf(cspFilePathTmp,"%s%d/P_%lu",AUDIT_TMP_CSP_PATH,dir,p_info->id);
			cspFd = open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
			if(cspFd < 2) return 0;
	//		lseek(cspFd,p_info->in_offset, SEEK_SET);
			lseek(cspFd,p_info_hd->seq - p_info->in_seq, SEEK_SET);
			AuditWrite(cspFd,payload,len);
			close(cspFd);

			return 0;
		}	
		if(p_info_hd->rst == 1 || p_info_hd->fin == 1){
				if(p_info->id !=0){
					AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id, HTTP_REQUEST,dir);
					AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id,HTTP_RESPONSE,dir);
					p_info->in_offset = 0;
					p_info->id = 0;
				}
				p_info->flag = 0;
				//memset(p_info,0,sizeof(*p_info));
				return 0;
		}
		return 0;

	}
	else{
		
		if(p_info_hd->fin ==1||p_info_hd->rst == 1  ){
			if(p_info->id !=0){
				AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id, HTTP_REQUEST,dir);
				AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id,HTTP_RESPONSE,dir);
				p_info->in_offset = 0;
				p_info->id = 0;
			}
			p_info->flag = 0;
	//		memset(p_info,0,sizeof(*p_info));
			return 0;
		}
		
		if(memcmp(payload,"POST ",5) == 0){		
			
			if(p_info->flag == 2 ) {
				if(p_info->id !=0){
					AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id, HTTP_REQUEST,dir);
					AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id,HTTP_RESPONSE,dir);
					p_info->in_offset = 0;
					p_info->id = 0;
				}
				p_info->flag = 0;
			}
			memset(requestUrl,0,sizeof(requestUrl));
			pos = strstr(payload," HTTP/1.");
			if(!pos){
				return 0;
			}
			str_len  =  pos - payload ;
			str_len-=4;
			if(str_len < 0) return 0;
			if(str_len >= AUDIT_URL_LEN){
				str_len = AUDIT_URL_LEN-1;
			}
			memcpy(requestUrl,payload+5,str_len);
			
			pos = strstr(requestUrl,"?");
			if(pos){
				*pos = '\0';
			}


			if(!strstr(requestUrl,"dhc.logon.csp") && !strstr(requestUrl,"UtilHome.csp")){
				return 0;
			}
			if(pos){
				*pos = '?';
			}		//url match
			if(type == AUDIT_CSP_TYPE_HIS){
				pthread_rwlock_rdlock(&(p_info_hd->policy->audit_lock));
				flag = audit_cache_policy_match(type,p_info_hd->cli_ip,p_info_hd->timenow,requestUrl,p_info_hd->policy,p_info_hd->redisConn);
				pthread_rwlock_unlock(&(p_info_hd->policy->audit_lock));
					if(flag == 0){
						return 0;
					}
			}
	//		printf("POST %s\n",requestUrl);
	//		if(strncmp(payload+5,AUDIT_CSP_REQUEST_LOGON,strlen(AUDIT_CSP_REQUEST_LOGON)) != 0 &&strncmp(payload+5,AUDIT_CSP_REQUEST_LOGON_PORTAL,strlen(AUDIT_CSP_REQUEST_LOGON_PORTAL)) != 0  ) return 0;
			if(p_info->out_seq != p_info_hd->seq){
				pthread_mutex_lock(&csp_id_mutex);
				p_info->id=++csp_id;
		//		csp_id_hash[csp_id%AUDIT_INDEX_NUM] = p_info_hd->hash;
				SetId(AUDIT_CSP_ID_PATH,csp_id);
				pthread_mutex_unlock(&csp_id_mutex);
				p_info->out_seq = p_info_hd->seq;
			}
			
			#if REL_HBASE
				sprintf(times,"%lu",p_info_hd->audit_time);
			#else
				get_audit_time(times);
			#endif
				
	//		CspGetPath(cspFilePathTmp,AUDIT_TMP_CSP_PATH,p_info->id);
			sprintf(cspFilePathTmp,"%s%d/Q_%lu",AUDIT_TMP_CSP_PATH,dir,p_info->id);
			
			CspGetIp(userip,p_info_hd->cli_ip);

			CspGetIp(desip,p_info_hd->ser_ip);
			
			CspGetMac(src_mac,p_info_hd->cli_mac);

			CspGetMac(des_mac,p_info_hd->ser_mac);

		//	policy_time = get_min();
			
			pthread_rwlock_rdlock(&tablesTime_lock);
			sprintf(conStr,"%llu|%d|%s|%s|%s|%s|%s|%u|%u|%u|%d|%s|&\r\n",p_info->id,p_info_hd->user_id,times,userip,src_mac,desip,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,p_info_hd->timenow,type,tablesTime);
			pthread_rwlock_unlock(&tablesTime_lock);
			cspFd=open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
			if(cspFd <= 2) return 0;
			p_info->out_offset = AuditWrite(cspFd,conStr,strlen(conStr));
			if(p_info->out_offset < 0){
				p_info->out_offset = 0;
				close(cspFd);
				return 0;
			}
			
			close(cspFd);
			p_info->flag=1;
		}
		else if(memcmp(payload,"GET ",4)==0){
			if(p_info->flag == 2 ) {
				if(p_info->id !=0){
					AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id, HTTP_REQUEST,dir);
					AuditFileRename(AUDIT_TMP_CSP_PATH,AUDIT_CSP_CONTENT_PATH,p_info->id,HTTP_RESPONSE,dir);
					p_info->in_offset = 0;
					p_info->id = 0;
				}
				p_info->flag = 0;
			}
			memset(requestUrl,0,sizeof(requestUrl));
			pos = strstr(payload," HTTP/1.");
			if(!pos){
				return 0;
			}
			str_len  =  pos - payload ;
			str_len-=4;
			if(str_len < 0) return 0;
			if(str_len >= AUDIT_URL_LEN){
				str_len = AUDIT_URL_LEN-1;
			}
			memcpy(requestUrl,payload+4,str_len);


			pos = strstr(requestUrl,"?");
			if(pos){
				*pos = '\0';
			}

			if(strstr(requestUrl,"dhcvisanopwait")){
				return 0;
			}
			if(strstr(requestUrl,"dhcvispatwait")){
				return 0;
			}
	//		dhcrisappbill  dhcnuripexeclist

			if(strstr(requestUrl,"dhcrisappbill")){
				return 0;
			}

			if(strstr(requestUrl,"dhcnuripexeclist")){
				return 0;
			}
			if(strstr(requestUrl,"dhc.epr.messagetab.csp")){
				return 0;
			}
			if(strstr(requestUrl,"dhc.bdp.ext.sys.csp")){
				return 0;
			}

			

			
			if(!strstr(requestUrl,".csp") &&!strstr(requestUrl,".cls"))
				return 0;

			if(pos){
				*pos = '?';
			}
			if(type == AUDIT_CSP_TYPE_HIS){
		//		printf("url:%s\n",requestUrl);
				pthread_rwlock_rdlock(&(p_info_hd->policy->audit_lock));
				flag = audit_cache_policy_match(type,p_info_hd->cli_ip,p_info_hd->timenow,requestUrl,p_info_hd->policy,p_info_hd->redisConn);
				pthread_rwlock_unlock(&(p_info_hd->policy->audit_lock));
			//	printf("url:%s,flag = %d\n",requestUrl,flag);
				if(flag == 0){
					return 0;
				}
			}


			char sip[100],dip[100];
			
			sprintf(dip,"%d.%d.%d.%d",(int)UCHARPTR(&p_info_hd->ser_ip)[0],(int)UCHARPTR(&p_info_hd->ser_ip)[1],(int)UCHARPTR(&p_info_hd->ser_ip)[2],(int)UCHARPTR(&p_info_hd->ser_ip)[3]);

			
			//if(p_info->audit_type == AUDIT_CACHE_CSP_PORTAL)
		//	printf("GET %s %d hash:%lu serpt:%u dip:%s seq:%u",requestUrl,dir,p_info_hd->hash,p_info_hd->ser_port,dip,p_info_hd->seq);
/*
			if(strcmp(&requestUrl[str_len-4],".gif")==0||strcmp(&requestUrl[str_len-4],".css")==0||strcmp(&requestUrl[str_len-4],".js")==0||strcmp(&requestUrl[str_len-3],".png")==0||strcmp(&requestUrl[str_len-3],".jpg")==0 ){
				return 0
			}
			
			if(strncmp(payload+4,AUDIT_CSP_REQUEST_CSP,strlen(AUDIT_CSP_REQUEST_CSP)) != 0 && strncmp(payload+4,AUDIT_CSP_GLOBLE_VIEW_CSP,strlen(AUDIT_CSP_GLOBLE_VIEW_CSP))!=0 && strncmp(payload+4,AUDIT_CSP_SQL_VIEW_CSP,strlen(AUDIT_CSP_SQL_VIEW_CSP))!=0) {
				return 0;
			}
*/			
	//		printf("0 outseq:%u,seq:%u\n",p_info->out_seq,p_info_hd->seq);

			if(p_info->out_seq != p_info_hd->seq){
				pthread_mutex_lock(&csp_id_mutex);
				p_info->id=++csp_id;
			//	csp_id_hash[csp_id%AUDIT_INDEX_NUM] = p_info_hd->hash;
				SetId(AUDIT_CSP_ID_PATH,csp_id);
				pthread_mutex_unlock(&csp_id_mutex);
				p_info->out_seq = p_info_hd->seq;
			}
		//	if(p_info->audit_type == AUDIT_CACHE_CSP_PORTAL)
	//		printf(" id:%lu\n",p_info->id);	
			
			
			#if REL_HBASE
				sprintf(times,"%lu",p_info_hd->audit_time);
			#else
				get_audit_time(times);
			#endif
//			CspGetPath(cspFilePathTmp,AUDIT_TMP_CSP_PATH,p_info->id);
			sprintf(cspFilePathTmp,"%s%d/Q_%lu",AUDIT_TMP_CSP_PATH,dir,p_info->id);

			CspGetIp(userip,p_info_hd->cli_ip);

			CspGetIp(desip,p_info_hd->ser_ip);
			
			CspGetMac(src_mac,p_info_hd->cli_mac);

			CspGetMac(des_mac,p_info_hd->ser_mac);

	//		policy_time = get_min();
			pthread_rwlock_rdlock(&tablesTime_lock);
			sprintf(conStr,"%llu|%d|%s|%s|%s|%s|%s|%u|%u|%u|%d|%s|&\r\n",p_info->id,p_info_hd->user_id,times,userip,src_mac,desip,des_mac,p_info_hd->cli_port,p_info_hd->ser_port,p_info_hd->timenow,type,tablesTime);
			pthread_rwlock_unlock(&tablesTime_lock);		
			cspFd=open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
			if(cspFd <= 2) return 0;
			p_info->out_offset = AuditWrite(cspFd,conStr,strlen(conStr));
			if(p_info->out_offset < 0){
				p_info->out_offset = 0;
				close(cspFd);
				return 0;
			}
			close(cspFd);
			p_info->flag=1;
		}
		if( p_info->flag==0)
			return 0;
		if( p_info->flag==1){
			
	///		printf("1 outseq:%u,seq:%u\n",p_info->out_seq,p_info_hd->seq);
			memset(cspFilePathTmp,0,sizeof(cspFilePathTmp));
			//CspGetPath(cspFilePathTmp,AUDIT_TMP_CSP_PATH,p_info->id);
			sprintf(cspFilePathTmp,"%s%d/Q_%lu",AUDIT_TMP_CSP_PATH,dir,p_info->id);
			cspFd = open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
			if(cspFd <= 2) return 0;
			lseek(cspFd,p_info->out_offset, SEEK_SET);
			lseek(cspFd,p_info_hd->seq - p_info->out_seq, SEEK_CUR);
			
			AuditWrite(cspFd,payload,len);
			// p_info->writtenbyes += len;
			AuditWrite(cspFd,"\r\n",2);
		//	 p_info->writtenbyes += 2;
			 
			close(cspFd);
			return 0;
		}
	}
}
#endif

#if 0
int CspPackRefrom(struct audit_pack_info_head * p_info_hd,struct audit_pack_info * p_info,unsigned char * payload){
	char * data;
	char cspFilePathTmp[AUDIT_PATH_LEN] = {0},cspFilePath[AUDIT_PATH_LEN] = {0};
	char times[AUDIT_TIME_LEN] = {0},userip[AUDIT_IPADDR_LEN] = {0},desip[AUDIT_IPADDR_LEN]={0},src_mac[AUDIT_MAC_LEN] = {0},des_mac[AUDIT_MAC_LEN]={0};
	char conStr[AUDIT_PACK_LEN]={0};
	int cspFd;
	int len = p_info_hd->payload_len;

	if(p_info_hd->isin){
		if(p_info_hd->rst == 1|| p_info_hd->fin == 1){
			printf("mv file Q_%lu\n",p_info->id);
		}
		if(memcmp(payload,"HTTP/1.",7) == 0){
			p_info->in_seq = p_info_hd->seq;
			p_info->id=++csp_id;
			csp_id_hash[csp_id%AUDIT_INDEX_NUM] = p_info_hd->hash;
			
			get_audit_time(times);

	//		CspGetPath(cspFilePathTmp,AUDIT_TMP_CSP_PATH,p_info->id);
			sprintf(cspFilePathTmp,"%sQ_%lu",AUDIT_TMP_CSP_PATH,p_info->id);
			
			CspGetIp(userip,p_info_hd->cli_ip);

			CspGetIp(desip,p_info_hd->ser_ip);
			
			CspGetMac(src_mac,p_info_hd->cli_mac);

			CspGetMac(des_mac,p_info_hd->ser_mac);
			sprintf(conStr,"%llu|%d|%s|%s|%s|%s|%s|%u|%u|&\r\n",p_info->id,p_info_hd->user_id,times,userip,src_mac,desip,des_mac,p_info_hd->cli_port,p_info_hd->ser_port);
			
			cspFd=open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
			if(cspFd <= 2) return 0;
			p_info->out_offset = AuditWrite(cspFd,conStr,strlen(conStr));
			if(p_info->out_offset < 0){
				p_info->out_offset = 0;
				close(cspFd);
				return 0;
			}
			
			close(cspFd);
			p_info->flag=1;
		}
		if( p_info->flag==0)
			return 0;
		if( p_info->flag==1){
			memset(cspFilePathTmp,0,sizeof(cspFilePathTmp));
			//CspGetPath(cspFilePathTmp,AUDIT_TMP_CSP_PATH,p_info->id);
			sprintf(cspFilePathTmp,"%sQ_%lu",AUDIT_TMP_CSP_PATH,p_info->id);
			cspFd = open(cspFilePathTmp, O_RDWR | O_CREAT, 0666);
			if(cspFd <= 2) return 0;
			lseek(cspFd,p_info->out_offset, SEEK_SET);
			lseek(cspFd,p_info_hd->seq - p_info->in_seq, SEEK_CUR);
			
			AuditWrite(cspFd,payload,len);
			// p_info->writtenbyes += len;
			AuditWrite(cspFd,"\r\n",2);
		//	 p_info->writtenbyes += 2;
			 
			close(cspFd);
			
			return 0;
		}

	}

	
}
#endif
