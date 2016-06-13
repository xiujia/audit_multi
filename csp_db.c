#include <stdio.h>
#include "csp_db.h"

/************************************************************************************************************/
/*mysql ���ݿ����Ӻ���������unix_socket_name��flagsͨ��ΪNULL��0*/
/*090324*/
int connect_mysql(MYSQL *m_mysql)
{
	/*��ʼ��mysql���ݿ�����*/
	if (!mysql_init(m_mysql))
	{
		printf("init mysql failed\n");
		if(mysql_errno(m_mysql))
		{
			fprintf(stderr,"error %d : %s\n", mysql_errno(m_mysql),mysql_error(m_mysql));
		}
		return -1;
	}
	
	/*����ѡ������ж������Ҫһ��һ������*/
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

	/*���� MYSQL ���ݿ�*/  
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


/*mysql ���ݿ�رպ���*/
int close_mysql(MYSQL *m_mysql)
{
	mysql_close(m_mysql);
	return 0;
}


/*mysql ���ݿ���º���*/
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


/*mysql ���ݿ�ɾ������*/
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


/*mysql ���ݿ���뺯��*/
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
/*�����޷���װ��API����������ֻ�ṩһ������*/
/*mysql ���ݿ��ѯ����(����Ϊʵ�ִ�select�л�ȡ���ݲ���ӡ����)*/
/*�ɸ���ʵ�������ĳsql����е�ĳЩ�ֶν��к������������Ӷ����д�������sql����ִ��*/

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
