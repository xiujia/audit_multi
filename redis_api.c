#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <hiredis/hiredis.h>
//#include "hiredis.h"
#include "csp_redis.h"




 int CspRedisOperation(int databaseNO,int type,char * key, char *value,redisContext * conn){
	
	redisReply *reply =NULL;
		
	char Command[COMMANDLEN]={0};
	char selectCmd[COMMANDLEN] = {0};
	char valuestr[VALUELEN];
	int returnval=0;

	if(strlen(key)> COMMANDLEN){
		return 0;
	}
/*
	if(strlen(key) != 0)
		printf("key = %s\n",key);
	if(strlen(value) != 0)
		printf("value  = %s\n",value);
*/
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
				value[reply->len]='\0';
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

/*

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
