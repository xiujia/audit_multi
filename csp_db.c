#include <stdio.h>
#include "csp_db.h"

/************************************************************************************************************/
/*mysql 数据库连接函数。其中unix_socket_name和flags通常为NULL和0*/
/*090324*/
int connect_mysql(MYSQL *m_mysql)
{
	/*初始化mysql数据库连接*/
	if (!mysql_init(m_mysql))
	{
		printf("init mysql failed\n");
		if(mysql_errno(m_mysql))
		{
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}
		return -1;
	}
	
	/*设置选项，可以有多个，但要一个一个设置*/
  if(mysql_options(m_mysql, MYSQL_READ_DEFAULT_GROUP, "simple"))
  {
		printf("mysql set option failure!\n");
		return -1;
	}
        
	if(mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8"))
	{
		printf("mysql set option failure!\n");
		return -1;
	}

	/*连接 MYSQL 数据库*/  
	if(!mysql_real_connect(m_mysql, SERVER_HOST, SQL_USER_NAME, SQL_PASSWORD, DB_MONITOR_NAME, PORT_NUMBER, UNIX_SOCKET_NAME, FLAGS))
	{
		printf("connect to mysql failure!\n");
		if ( mysql_errno(m_mysql))
		{
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}
		return -1;
	}

	return 0;
}


/*mysql 数据库关闭函数*/
int close_mysql(MYSQL *m_mysql)
{
	mysql_close(m_mysql);
	return 0;
}


/*mysql 数据库更新函数*/
int update_mysql(MYSQL *m_mysql, const char *sql_update_command)
{
	if(mysql_query(m_mysql,sql_update_command))
  {
		printf("update_mysql failed!\n");
    if(mysql_errno(m_mysql))
		{
			fprintf(stderr,"update_mysql error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}	
		return -1;	
	}
	return 0;
}


/*mysql 数据库删除函数*/
int delete_mysql(MYSQL *m_mysql, const char *sql_delete_command)
{
	if(mysql_query(m_mysql,sql_delete_command))
  {
		printf("Delete failed!\n");
    if(mysql_errno(m_mysql))
		{
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}	
		return -1;	
	}
	return 0;
}


/*mysql 数据库插入函数*/
int insert_mysql(MYSQL *m_mysql, const char *sql_insert_command)
{
	if(mysql_query(m_mysql,sql_insert_command))
  {
		printf("Inserted failed!\n");
    if(mysql_errno(m_mysql))
		{
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}	
		return -1;	
	}
	return 0;	
}


#if 0
/*由于无法封装成API，所以这里只提供一个例子*/
/*mysql 数据库查询函数(例子为实现从select中获取数据并打印出来)*/
/*可根据实际情况对某sql语句中的某些字段进行函数参数化，从而进行大批量的sql语句的执行*/

int select_mysql(MYSQL *m_mysql)
{
	MYSQL_ROW sqlrow;
	MYSQL_RES *res_ptr;
	char sql_select_command[100] = "SELECT id,description FROM cfg_app_group WHERE name='456'";
	
	if(mysql_query(m_mysql,sql_select_command))
	{
		printf("Select failed!\n");
    if(mysql_errno(m_mysql))
		{
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}	
		return -1;	
	}
	res_ptr = mysql_use_result(m_mysql);
	if(res_ptr)
	{
		while((sqlrow = mysql_fetch_row(res_ptr)))
		{
			printf("id : %s  ,description : %s  \n", sqlrow[0], sqlrow[1]);
		}
		mysql_free_result(res_ptr);
	}
	return 0;
}
#endif
