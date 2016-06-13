


#ifndef _AUDIT_DATABASE_SQL_H
#define _AUDIT_DATABASE_SQL_H




/*
	数据库插入不采用实时插入，改为写入文件批量导入。
	每种审计的sql文件名的前缀分别为：
	csp：Sql_csp_
	terminal:Sql_terminal_
	studio:Sql_studio_

	前缀后面跟创建时间
	
	完整名字举例：Sql_csp_20150101
*/


#define SQL_PATH	"/data/audit/sql/"
#define SQL_TMP		"/data/audit/sql_tmp/"
#define SQL_PATH_BACKUP   "/data/audit/sql_backup/"

#define CSP_SQL_FILE				"Sql_csp_"
#define CSP_SQL_ALARM_FILE			"Sql_csp_alarm_"

#define TERMINAL_SQL_FILE			"Sql_terminal_"
#define TERMINAL_SQL_ALARM_FILE		"Sql_terminal_alarm_"

#define STUDIO_SQL_FILE				"Sql_studio_"
#define STUDIO_SQL_ALARM_FILE		"Sql_studio_alarm_"


#endif