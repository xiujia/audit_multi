
/*

redisCommandִ�к󷵻�ֵ����ΪredisReply��ͨ��redisReply�ṹ���е�type��������ȷ������ִ�е������

REDIS_REPLY_STATUS��

����ִ�н��Ϊ״̬���������set����ķ���ֵ��������REDIS_REPLY_STATUS��Ȼ��ֻ�е�������Ϣ��"OK"ʱ���ű�ʾ������ִ�гɹ�������ͨ��reply->str�õ�������Ϣ��ͨ��reply->len�õ���Ϣ���ȡ�

REDIS_REPLY_ERROR��

���ش��󡣴�����Ϣ����ͨ��reply->str�õ�������Ϣ��ͨ��reply->len�õ���Ϣ���ȡ�

REDIS_REPLY_INTEGER��

�������ͱ�ʶ������ͨ��reply->integer�����õ�����Ϊlong long��ֵ��

REDIS_REPLY_NIL:

����nil����˵��������Ҫ���ʵ����ݡ�

REDIS_REPLY_STRING:

�����ַ�����ʶ������ͨ��reply->str�õ�����ֵ��ͨ��reply->len�õ���Ϣ���ȡ�

REDIS_REPLY_ARRAY:

�������ݼ���ʶ�����ݼ���Ԫ�ص���Ŀ����ͨ��reply->elements��ã�ÿ��Ԫ���Ǹ�redisReply����Ԫ��ֵ����ͨ��reply->element[..index..].*��ʽ��ã����ڻ�ȡ������ݽ���Ĳ�����



*/


/*
���ĳЩ��������redisConnect�� redisCommand�����ò��ɹ�����������ֵΪNULL����REDIS_ERR����ʱcontext�ṹ���е�err��ԱΪ����ֵ������Ϊ���¼��ֳ���

REDIS_ERR_IO:����������ʱ������дsocket���߶�socket��������I/O����������ڴ����а�����errno.hͷ�ļ�������ܵõ�׼ȷ�Ĵ����롣

REDIS_ERR_EOF:redis������Ѿ��ر��˴����ӡ�

REDIS_ERR_PROTOCOL:����˽���Э��ʱ�����˴���

REDIS_ERR_OTHER:��������Ŀǰ������ʾ�޷�����Ŀ���������Ĵ���


*/


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <hiredis/hiredis.h>
#include "redis_new_api.h"


#define COMMANDLEN 1024





inline void redis_call_error_catch(redisContext * conn){
		fprintf(stderr,"error %d:%s\n",conn->err,conn->errstr);
}



int redis_get_value_str(int databaseNO,char *key,char *value,redisContext * conn){
	redisReply *reply =NULL;
	char Command[COMMANDLEN];
	char selectCmd[COMMANDLEN];
	int rel=0;

	//key can not be 0 length
	if(strlen(key) == 0){
		return -1;
	}

	// select database table
	reply =(redisReply*)redisCommand(conn,"select %d",databaseNO);

	if(conn->err){
		REDIS_CALL_ERROR_CACHE(conn);
		if(reply){
			freeReplyObject(reply);
			return 0;
		}
	}
	
	freeReplyObject(reply);//free reply

	//get value
	reply = (redisReply*)redisCommand(conn,"get %s",key);
	
	if(conn->err){
		REDIS_CALL_ERROR_CACHE(conn);
		if(reply){
			freeReplyObject(reply);
			return 0;
		}
	}


	switch(reply->type){
		case REDIS_REPLY_STRING://get string ok
			strncpy(value,reply->str,reply->len);
			value[reply->len]='\0';
			rel = 1;
			break;
		case REDIS_REPLY_NIL://data not exist
			rel = 2;
			break;
		default:
			rel = 3;
			break;
	}
	freeReplyObject(reply);//free reply
	reply = NULL;

	return rel;
}

int redis_set_value_str(int databaseNO,char *key,char *value,redisContext * conn){
	redisReply *reply =NULL;
	char Command[COMMANDLEN]={0};
	char selectCmd[COMMANDLEN] = {0};
	int rel = 0;
		//key can not be 0 length
	if(strlen(key)==0 || strlen(key)==0){
		return -1;
	}


		reply =(redisReply*)redisCommand(conn,"select %d",databaseNO);
		if(conn->err){
			REDIS_CALL_ERROR_CACHE(conn);
			if(reply){
				freeReplyObject(reply);
				return 0;
			}
		}

		
		freeReplyObject(reply);
		reply = NULL;
		
		reply = (redisReply*)redisCommand(conn,"set %s %s",key,value);

		if(conn->err){
			REDIS_CALL_ERROR_CACHE(conn);
			if(reply){
				freeReplyObject(reply);
				return 0;
			}
		}


		switch(reply->type){
			case REDIS_REPLY_STATUS:
				if(strcasecmp(reply->str,"OK") == 0){
					rel =  1;//set ok
				}
				else{
					rel = 2; // set not ok
				}
				break;
			case REDIS_REPLY_NIL:
				rel = 3;//
				break;
			default:
				rel = 4;
				break;
		}
		freeReplyObject(reply);
		reply = NULL;
		return rel;
		
}



int get_mac_str(char * ip,char * mac,redisContext * conn){
	int rel= 0;
	rel = redis_get_value_str(REDIS_IP_MAC_TABLE,ip,mac,conn);
	return rel;  //rel == 1  get success
}


int redis_clear_db(int db){
	redisContext * conn;
	redisReply * reply =NULL;
	conn = redisConnect(REDISSERVERHOST,REDISSERVERPORT);
	if(conn->err){
		REDIS_CALL_ERROR_CACHE(conn);
		redisFree(conn);
		conn=NULL;
		return 0;
	}

	reply =(redisReply*)redisCommand(conn,"select %d",db);
	if(conn->err){
		REDIS_CALL_ERROR_CACHE(conn);
		redisFree(conn);
		if(reply){
			freeReplyObject(reply);
			return 0;
		}
		
	}

	freeReplyObject(reply);
	
	reply =(redisReply*)redisCommand(conn,"flushdb");
	if(conn->err){
		REDIS_CALL_ERROR_CACHE(conn);
		redisFree(conn);
		if(reply){
			freeReplyObject(reply);
			return 0;
		}
	}

	freeReplyObject(reply);

	redisFree(conn);

	conn = NULL;

	return 1;
	
}

 /*

 int CspRedisOperation(int databaseNO,int type,char * key, char *value,redisContext * conn){
	
	redisReply *reply =NULL;
		
	char Command[COMMANDLEN]={0};
	char selectCmd[COMMANDLEN] = {0};
	char valuestr[VALUELEN];
	int returnval=0;

	if(strlen(key) != 0)
		printf("key = %s\n",key);
	if(strlen(value) != 0)
		printf("value  = %s\n",value);

	switch(type){
		case OPERATION_SET:
			if(strlen(key) == 0) return -1;
			if(strlen(value)==0) return -1;
			sprintf(selectCmd,"select %d",databaseNO);
			reply =(redisReply*)redisCommand(conn,"select %d",databaseNO);
			freeReplyObject(reply);
			reply = NULL;
			sprintf(Command,"set %s %s",key,value);
			#if CSP_RELEASE_DEBUG
			printf("command:%s\n",Command);
			#endif
			reply = (redisReply*)redisCommand(conn,"set %s  %s",key,value);
	//		freeReplyObject(reply);
	//		reply=  (redisReply*)redisCommand(conn,Command);
	//		freeReplyObject(reply);
			if(NULL == reply){
				printf("reply NULL .\n");
				return 0;
			}
			if(reply->type == REDIS_REPLY_STATUS){
				if(strcasecmp(reply->str,"OK") == 0){
					returnval =  1;//set ok
				}else{
					returnval =  3;//set not ok
				}
			}
			else if(reply->type == REDIS_REPLY_NIL){
				returnval = 2;  //not set
			}
			break;
		case OPERATION_GET:
			if(strlen(key) == 0) return -1;
			sprintf(selectCmd,"select %d",databaseNO);
			reply =(redisReply*)redisCommand(conn,"select %d",databaseNO);
		//	fprintf(stderr,"befroe free\n");
			freeReplyObject(reply);
			reply=NULL;
			sprintf(Command,"get %s",key);
			#if CSP_RELEASE_DEBUG
			printf("command:%s\n",Command);
			#endif
	//		fprintf(stderr,"befroe command\n");
			reply = (redisReply*)redisCommand(conn,"get %s",key);
	//		fprintf(stderr,"after command\n");
			if(NULL == reply){
				printf("reply NULL .\n");
				return 0;
			}

			if(reply->type == REDIS_REPLY_STRING){
			//	printf("%s,len = %d\n",reply->str,reply->len);
				memcpy(value,reply->str,reply->len);

				returnval =  4;//get string ok
			}
			else if(reply->type == REDIS_REPLY_NIL){
				
				returnval =  5;//get string nil
			}
			break;
		default:
			break;
	}
	

	freeReplyObject(reply);
	reply = NULL;
	return returnval;
}


int CspRedisClear(int db,redisContext * conn){
	char cmd[COMMANDLEN]={0};
	redisReply *reply;
	if(db == -1){

		sprintf(cmd,"flushall");
		reply =(redisReply*)redisCommand(conn,cmd);
		freeReplyObject(reply);

	}
	else{
			sprintf(cmd,"select %d",db);
			reply =(redisReply*)redisCommand(conn,cmd);
			freeReplyObject(reply);

			memset(cmd,0,sizeof(cmd));
			sprintf(cmd,"flushdb");
			reply =(redisReply*)redisCommand(conn,cmd);
			freeReplyObject(reply);
	}
	return 0;
}



int main(int argc ,char * argv[]){
	char * op = argv[1];
	int opp = atoi(op);
	char *key=NULL,*value=NULL;
	if (opp == 1){
		value = argv[3];
	}
	else{
		value = (char *)malloc(VALUELEN);
		memset(value,0,VALUELEN);
	}
	key = argv[2];
	
	
	CspRedisOperation(opp,key,value);

	if(opp !=1){
		printf("%s \n",value);
		free(value);
	}
	return 0;

}

*/

