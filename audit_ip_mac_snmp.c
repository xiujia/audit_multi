#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "op_db.h"
#include "redis_new_api.h"

#define CMD_SNMPWALK	"snmpwalk -Os"
#define OPT_PASS 		"-c"
#define OPT_VERSION 	"-v"
#define OPT_MODULE		"-m"
#define IP_MAC_SHELL_SCRIPT_FILE		"/usr/inp/bin/audit_ip_mac.sh"
#define IP_MAC_LIST_PATH				"/media/data/tmp/"
#define IP_MAC_LIST_TYPE				".tmp"
//#define SNMP_CFG_FILE	"/usr/inp/cfg/snmp_policy.cfg"



#define DEFAULT_PASS	"public"
#define DEFAULT_VERSION	"1"
#define DEFAULT_MODULE	"ALL"
#define DEFAULT_OID		"ipNetToMediaPhysAddress"

//snmpwalk -c public -v 1 -m ALL ipaddr ipNetToMediaPhysAddress

int snmp_off_on_flag;

static int mystrcat(char * dest, int startpos,char *arg1,char *arg2){
	int len = 0;
	
	if(arg1&&arg2){
		sprintf(dest+startpos,"%s %s ",arg1,arg2);
	}
	else if(arg1){
		sprintf(dest+startpos,"%s ",arg1);
	}

	len = strlen(dest);
	return len;
	
}

int getSnmpCmdString(char *snmpwalk_cmd,char * password,char *version,char * module,char *oid,char *ipaddr){
//	char snmpwalk_cmd[1024]={0};
	int  len=0;
//	int i=0;
	if(!ipaddr){
		fprintf(stderr,"No ip configuration,please config an ip address for target.\n");
		return -1;
	}

	len = mystrcat(snmpwalk_cmd,len,CMD_SNMPWALK,NULL);
	

	if(password){
		len = mystrcat(snmpwalk_cmd,len,OPT_PASS,password);
	}
	else{
		len = mystrcat(snmpwalk_cmd,len,OPT_PASS,DEFAULT_PASS);	
	}

	if(version){
		len = mystrcat(snmpwalk_cmd,len,OPT_VERSION,version);
	}
	else{
		len = mystrcat(snmpwalk_cmd,len,OPT_VERSION,DEFAULT_VERSION);	
	}

	if(module){
		len = mystrcat(snmpwalk_cmd,len,OPT_MODULE,module);
	}
	else{
		len = mystrcat(snmpwalk_cmd,len,OPT_MODULE,DEFAULT_MODULE);
	}


	if(oid){
		len = mystrcat(snmpwalk_cmd,len,ipaddr,oid);
	}
	else{
		len = mystrcat(snmpwalk_cmd,len,ipaddr,DEFAULT_OID);
	}

//	len = mystrcat(snmpwalk_cmd,IP_MAC_FILE,NULL);

	
	return len ;
	
}




/*
DROP TABLE IF EXISTS `sys_cfg_snmp`;
CREATE TABLE `sys_cfg_snmp` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(200) NOT NULL COMMENT '设备名称',
  `ipaddr` varchar(200) NOT NULL COMMENT '设备地址',
  `password` varchar(200) DEFAULT '' COMMENT 'SNMP密码',
  `version` varchar(200) NOT NULL COMMENT 'SNMP版本v1、v2、v2c、v3',
  `module` varchar(200) DEFAULT 'ALL' COMMENT '模块',
  `oid` varchar(200) DEFAULT 'null' COMMENT 'OID',
  `flag` int(4) NOT NULL DEFAULT '0' COMMENT '1开启、0关闭',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of sys_cfg_snmp
-- ----------------------------

-- ----------------------------
-- Table structure for `sys_cfg_snmpflag`
-- ----------------------------
DROP TABLE IF EXISTS `sys_cfg_snmpflag`;
CREATE TABLE `sys_cfg_snmpflag` (
  `flag` int(4) NOT NULL DEFAULT '0' COMMENT '1开启、0关闭'
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

*/

//for mysql_use_result  mysql_fetch_row
//select flag,id,ipaddr,password,version from sys_cfg_snmp;

int AuditWrite(int Fd,char *psData,int Len){
	int writtenSuccsesBytes = 0;
	int writtenBytes = 0;
 	while(1){
		writtenSuccsesBytes = write(Fd,psData + writtenBytes,Len - writtenBytes );
		if (writtenSuccsesBytes == -1) {
			return -1;
			perror("write:");
		}
		writtenBytes += writtenSuccsesBytes;
		if (writtenBytes < Len) {
			continue;
		} else if (writtenBytes == Len) {
			break;
		}
	}
	return writtenBytes;
}

int get_row_field(MYSQL_ROW row,unsigned int fields_num,unsigned long * fields_length){
	int i=0;
	char id[100]={0};
	char flag[100]={0};
	char ipaddr[100]={0};
	char password[100]={0};
	char version[100]={0};
	char snmpwalk_cmd[1024]={0};
	char temp_str[1024];
	int cmd_len=0;
	int snmp_shell_fd;
//0-flag,1-id,2-ipaddr,3-password,4-version

	
	for(i = 0;i<fields_num;i++){
	//printf("row[%d]:%s\n",i,row[i]);
		switch(i){
			case 0:
				memcpy(flag,row[i],fields_length[i]);
				if(atoi(flag) == 0) {
					return 0 ;
				}
				break;
			case 1:
				memcpy(id,row[i],fields_length[i]);
				break;
			case 2:
				memcpy(ipaddr,row[i],fields_length[i]);
				break;
			case 3:
				memcpy(password,row[i],fields_length[i]);
				break;
			case 4:
				memcpy(version,row[i],fields_length[i]);
				break;
			default:
				break;
		}
		
	}
	memset(temp_str,0,sizeof(temp_str));
	cmd_len = getSnmpCmdString(snmpwalk_cmd,password,version,NULL,NULL,ipaddr);
	sprintf(temp_str,"%s >> %s%s.tmp\n",snmpwalk_cmd,IP_MAC_LIST_PATH,flag);
	cmd_len = strlen(temp_str);
	snmp_shell_fd = open(IP_MAC_SHELL_SCRIPT_FILE, O_RDWR |O_CREAT|O_APPEND, 0666);
	if(snmp_shell_fd < 0){
		return -1;
	}
	if(snmp_shell_fd < 2){
		close(snmp_shell_fd);
		snmp_shell_fd = 0;
		return -1;
	}
	printf("str:%s",temp_str);
	AuditWrite(snmp_shell_fd,temp_str,cmd_len);

	close(snmp_shell_fd);
	snmp_shell_fd = 0;

	return 1;
}


//select flag from sys_cfg_snmpflag;
int get_off_on(MYSQL_ROW row,unsigned int fields_num,unsigned long * fields_length){
	int i;
	char flag[100]={0};
	for(i = 0;i<fields_num;i++){
		memcpy(flag,row[i],fields_length[i]);
	}
	snmp_off_on_flag = atoi(flag);
	return 0;
} 



int main(int argc,char **argv){
	MYSQL my_db;
	snmp_off_on_flag = 0;
	char cmd[]="/usr/inp/bin/get_snmp_file & > /dev/null";
	const char sql_cmd1[]="select flag from sys_cfg_snmpflag;";
	const char sql_cmd[]="select flag,id,ipaddr,password,version from sys_cfg_snmp;";


	redis_clear_db(REDIS_IP_MAC_TABLE);


	if(connect_mysql_opt(&my_db, SERVER_HOST, SQL_USER_NAME, SQL_PASSWORD, DB_MONITOR, PORT_NUMBER, UNIX_SOCKET_NAME, FLAGS)==-1){
		return -1;
	}

	if(select_mysql(&my_db,sql_cmd1,get_off_on)==-1){
		return -1;
	}
	printf("%d\n",snmp_off_on_flag);
	if(snmp_off_on_flag == 0){
		
		return -1;
	} 
	
	if(select_mysql(&my_db,sql_cmd,get_row_field)==-1){
		return -1;
	}

	close_mysql(&my_db);


	system(cmd);
	return 0;
}
