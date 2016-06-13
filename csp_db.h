#ifndef   _CSP_DB_H
#define 	 _CSP_DB_H

#include <mysql/mysql.h>

/*对于相对于固定的数据库连接参数，我们在这里定义宏，赋值到函数里面，可以将connect_mysql的参数简化到最少*/
#define SERVER_HOST "localhost"
#define SQL_USER_NAME	"forceview"
#define SQL_PASSWORD "forceview"
#define DB_MONITOR_NAME "dbmonitor"
#define PORT_NUMBER 3306
#define UNIX_SOCKET_NAME NULL
#define FLAGS 0

int connect_mysql(MYSQL *m_mysql);
					
int close_mysql(MYSQL *m_mysql);

int update_mysql(MYSQL *m_mysql, const char *sql_update_command);

int delete_mysql(MYSQL *m_mysql, const char *sql_delete_command);

int insert_mysql(MYSQL *m_mysql, const char *sql_insert_command);

#endif
