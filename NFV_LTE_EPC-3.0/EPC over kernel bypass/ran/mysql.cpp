#include "mysql.h"

ConnInfo::ConnInfo() {
	server = "localhost";
	user = "root";
	passwd = "mynamer";
	db = "hss";
}

ConnInfo::~ConnInfo() {

}

MySql::MySql() {
	conn_fd = mysql_init(NULL);
}

void MySql::conn() {
	if (!mysql_real_connect(conn_fd, conn_info.server.c_str(), conn_info.user.c_str(), conn_info.passwd.c_str(), conn_info.db.c_str(), 0, NULL, 0)) {
		handle_db_error();
	}
}

void MySql::handle_query(string query, MYSQL_RES **result) {
	if (mysql_query(conn_fd, query.c_str())) {
		handle_db_error();
	}
	*result = mysql_store_result(conn_fd);
}

void MySql::handle_db_error() {
	TRACE(cout << mysql_error(conn_fd) << endl;)
	g_utils.handle_type1_error(-1, "mysql error: mysql_handledberror");
}

MySql::~MySql() {
	mysql_close(conn_fd);
}