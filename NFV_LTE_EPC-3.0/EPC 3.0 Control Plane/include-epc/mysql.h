#ifndef MYSQL_H
#define MYSQL_H

/* (C) MYSQL, MYSQL_RES, mysql_init */
#include <mysql/mysql.h>

#include "utils.h"

class ConnInfo {
public:
	string server;
	string user;
	string passwd;
	string db;

	ConnInfo();
	~ConnInfo();
};

class MySql {
private:
	MYSQL *conn_fd;
	ConnInfo conn_info;

	void handle_db_error();
	
public:
	MySql();
	void conn();
	void handle_query(string, MYSQL_RES**);
	~MySql();
};

#endif //MYSQL_H