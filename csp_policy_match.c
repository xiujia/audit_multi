#include "csp_policy.h"
#include "csp_redis.h"
#include "csp_deal.h"
int CspMapMatch(unsigned key,unsigned char * map){
	if(bit_test(key,map)){
		return 1;
	}
	return 0;
}

int CspPolicyMatch(CSP_FILE_INFO *csp,CSP_POLICY * policy){
		unsigned int time;
		unsigned char urlId[CSP_ID_LEN]={0};
		unsigned char userId[CSP_ID_LEN]={0};
		unsigned int url_id,user_id;
		int i =0;
		int isTime ,isUrl,isUrlAccount,isAccount;
		if(policy->valid_flag== 0) return -1;
		if(policy->time_flag == 1){
			time = atoi(csp->cspHead.policytime);
			isTime = CspMapMatch(time,policy->time);
		}
		else{
			isTime = 1;
		}

		if(isTime == 0) return 0;
		
		if(policy->url_flag == 1){
			CspRedisOperation(REDIS_DB_URL_ID,OPERATION_GET,csp->cspData.requestUrl,urlId,csp->conn);
			CspRedisOperation(REDIS_DB_ACCOUNT_ID,OPERATION_GET,csp->cspData.userName,userId,csp->conn);
			url_id = atoi(urlId);
			user_id = atoi(userId);
			for(i = 0;i< policy->url_num;i++){
				if(url_id == policy->url[i].requesturl_id){
					isUrl =1;
					isUrlAccount = CspMapMatch(url_id,policy->url[i].account);
					if(isUrlAccount){
						break;
					}
				}
			}

			if(isUrl == 1 && isUrlAccount ==0){
				return 1;
			}
			else return 0;
			
		}
		else if(policy->account_flag == 1){
			CspRedisOperation(REDIS_DB_ACCOUNT_ID,OPERATION_GET,csp->cspData.userName,userId,csp->conn);
			user_id = atoi(userId);
			isAccount = CspMapMatch(url_id,policy->url[i].account);
			if(isAccount == 0){
				return 0;
			}
			
		}
		return 1;
}

