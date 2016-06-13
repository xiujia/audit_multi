#include "csp_policy.h"
#include "csp_redis.h"
#include "csp_deal.h"
#include "audit_ensemble.h"
int CspMapMatch(unsigned key,unsigned char * map) {
    if(bit_test(key,map)) {
        return 1;
    }
    return 0;
}

int  csp_audit_policy_match(CSP_FILE_INFO *csp, CSP_AUDIT_POLICY *policy) {
	unsigned int time;
	unsigned char urlId[CSP_ID_LEN]={0};
	unsigned char userId[CSP_ID_LEN]={0};
	unsigned int url_id,user_id;
	int i =0;
	int isTime ,isUrl,isip,is_appid;

	if(policy->valid_flag == 0) return -1;

	/* Æ¥Åätime */

	time = atoi(csp->cspHead.policytime);
	isTime = CspMapMatch(time, policy->time);


	if(isTime == 0) return 0;

	/* Æ¥Åäip */
	
	u_int32_t ip_map_addr, iip0, iip1, iip2, iip3;
	sscanf(csp->cspHead.userip, "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);
	ip_map_addr =iip2*256*256+ iip1*256 + iip0;
	isip = CspMapMatch(ip_map_addr, policy->ip);
	if(isip == 0) {
		return 0;
	}
	

	/* Æ¥Åäapp_id */
	is_appid = CspMapMatch(csp->type, policy->app_id);
	if(is_appid == 0) {
		return 0;
	}
	return 1;
}

int policy_match(CSP_FILE_INFO *csp, CACHE_POLICY_CONF *policy_conf){
	int i;
	int flags[CSP_POLICY_NUM]={0};
	int flag=0;
	#if GA_TEST
	for(i=0;i<CSP_POLICY_NUM;i++){
		flags[i] = csp_audit_policy_match(csp,&policy_conf->csp_audit_policy[i]);
		if(flags[i]==1){
			flag = 1;
			if(csp->security_level == 0||csp->security_level > policy_conf->csp_audit_policy[i].security_level){
				csp->security_level = policy_conf->csp_audit_policy[i].security_level;
			}
		}	
	}
	
	#else
	for(i=0;i<CSP_POLICY_NUM;i++){
		flag = csp_audit_policy_match(csp,&policy_conf->csp_audit_policy[i]);
		if(flag==1){
			break;
		}
	}

	#endif
	return flag;
}
#if 1
int  ensemble_audit_policy_match(AUDIT_ENSEMBLE_REL*rel, CSP_AUDIT_POLICY *policy) {
	unsigned int time;
	unsigned char urlId[CSP_ID_LEN]={0};
	unsigned char userId[CSP_ID_LEN]={0};
	unsigned int url_id,user_id;
	int i =0;
	int isTime=0 ,isUrl=0,isip=0,is_appid=0;
	u_int32_t ip_map_addr, iip0, iip1, iip2, iip3;

	if(policy->valid_flag == 0) return -1;

	/* Æ¥Åätime */
#if MULTI_THREADS
	time = rel->policytime;
#else
	time = atoi(rel->policytime);
#endif
	isTime = CspMapMatch(time, policy->time);
	if(isTime == 0) return 0;

	/* Æ¥Åäip */
#if MULTI_THREADS
	ip_map_addr = rel->userip & 0xffffff;
#else
	sscanf(rel->userip, "%d.%d.%d.%d", &iip3, &iip2, &iip1, &iip0);
	ip_map_addr =iip2*256*256+ iip1*256 + iip0;
#endif
	isip = CspMapMatch(ip_map_addr, policy->ip);
	if(isip == 0) {
		return 0;
	}
	

	/* Æ¥Åäapp_id */
	is_appid = CspMapMatch(rel->type, policy->app_id);
	if(is_appid == 0) {
		return 0;
	}
	return 1;
}



int policy_match_ensemble(AUDIT_ENSEMBLE_REL*rel, CACHE_POLICY_CONF *policy_conf){
	int i;
	int flag;

	#if GA_TEST
	int flags[CSP_POLICY_NUM]={0};
	for(i=0;i<CSP_POLICY_NUM;i++){
		flags[i] = ensemble_audit_policy_match(rel,&policy_conf->csp_audit_policy[i]);
		if(flags[i]==1){
			flag = 1;
			if(rel->security_level == 0||rel->security_level > policy_conf->csp_audit_policy[i].security_level){
				rel->security_level = policy_conf->csp_audit_policy[i].security_level;
			}
		}	
	}
	#else
	for(i=0;i<CSP_POLICY_NUM;i++){
		flag = ensemble_audit_policy_match(rel,&policy_conf->csp_audit_policy[i]);
		if(flag == 1){
			break;
		}
	}
	#endif
	return flag;
}


#endif

