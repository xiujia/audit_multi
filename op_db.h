#ifndef   OPDB_H
#define 	 OPDB_H

#include <mysql/mysql.h>

/*对于相对于固定的数据库连接参数，我们在这里定义宏，赋值到函数里面，可以将connect_mysql的参数简化到最少*/
#define SERVER_HOST "localhost"
#define SQL_USER_NAME	"forceview"
#define SQL_PASSWORD "forceview"
#define DB_NAME  			"forceview"
#define DB_MONITOR	"dbmonitor"
#define PORT_NUMBER 3306
#define UNIX_SOCKET_NAME NULL
#define FLAGS 0

int connect_mysql(MYSQL *m_mysql);

int connect_mysql_opt(MYSQL *m_mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long client_flag);

int close_mysql(MYSQL *m_mysql);

int update_mysql(MYSQL *m_mysql, const char *sql_update_command);

int delete_mysql(MYSQL *m_mysql, const char *sql_delete_command);

int insert_mysql(MYSQL *m_mysql, const char *sql_insert_command);

int select_mysql(MYSQL * m_mysql,const char * sql_command,int (*callbackFun)(MYSQL_ROW ,unsigned int ,unsigned long * ));

#endif
