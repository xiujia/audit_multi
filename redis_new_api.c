
/*

redisCommand执行后返回值类型为redisReply。通过redisReply结构体中的type变量可以确定命令执行的情况。

REDIS_REPLY_STATUS：

返回执行结果为状态的命令。比如set命令的返回值的类型是REDIS_REPLY_STATUS，然后只有当返回信息是"OK"时，才表示该命令执行成功。可以通过reply->str得到文字信息，通过reply->len得到信息长度。

REDIS_REPLY_ERROR：

返回错误。错误信息可以通过reply->str得到文字信息，通过reply->len得到信息长度。

REDIS_REPLY_INTEGER：

返回整型标识。可以通过reply->integer变量得到类型为long long的值。

REDIS_REPLY_NIL:

返回nil对象，说明不存在要访问的数据。

REDIS_REPLY_STRING:

返回字符串标识。可以通过reply->str得到具体值，通过reply->len得到信息长度。

REDIS_REPLY_ARRAY:

返回数据集标识。数据集中元素的数目可以通过reply->elements获得，每个元素是个redisReply对象，元素值可以通过reply->element[..index..].*形式获得，用在获取多个数据结果的操作。



*/


/*
如果某些函数（如redisConnect， redisCommand（调用不成功，函数返回值为NULL或者REDIS_ERR，此时context结构体中的err成员为非零值，可能为以下几种常量

REDIS_ERR_IO:当创建连接时（试着写socket或者读socket）发生的I/O错误。如果你在代码中包含了errno.h头文件，你便能得到准确的错误码。

REDIS_ERR_EOF:redis服务端已经关闭了此连接。

REDIS_ERR_PROTOCOL:服务端解析协议时发生了错误。

REDIS_ERR_OTHER:其他错误。目前仅仅表示无法解析目标主机名的错误。


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

