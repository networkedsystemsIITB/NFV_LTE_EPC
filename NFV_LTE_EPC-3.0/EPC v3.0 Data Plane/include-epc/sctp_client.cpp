#include "sctp_client.h"

SctpClient::SctpClient() {
	conn_fd = socket(AF_INET, SOCK_STREAM, 0);
	g_utils.handle_type1_error(conn_fd, "Socket error: sctpclient_sctpclient");
}

void SctpClient::conn(string arg_server_ip_addr, int arg_server_port) {
	//cout<<"Estd Conn"<<endl;
	init(arg_server_ip_addr, arg_server_port);
	// g_nw.set_rcv_timeout(conn_fd, 1);
	connect_with_server();
}

void SctpClient::init(string arg_server_ip_addr, int arg_server_port) {
	int status;

	server_port = arg_server_port;
	server_ip_addr = arg_server_ip_addr;
	g_nw.set_inet_sock_addr(server_ip_addr, server_port, server_sock_addr);
}

void SctpClient::connect_with_server() {
	int status;

	while (1) {
		status = connect(conn_fd, (struct sockaddr *)&server_sock_addr, sizeof(server_sock_addr));
		if (errno == ECONNREFUSED || errno == ETIMEDOUT) {
			errno = 0;
			usleep(1000);
			continue;
		}
		else {
			break;
		}
	}
	g_utils.handle_type1_error(status, "Connect error: sctpclient_connectwithserver");
}

void SctpClient::snd(Packet pkt) {
	int status;
	while (1) {
		status = g_nw.write_sctp_pkt(conn_fd, pkt);
		if (errno == EPERM) {
			errno = 0;
			usleep(1000);
			continue;
		}
		else {
			break;
		}		
	}
	g_utils.handle_type2_error(status, "Write error: sctpclient_snd");
}

void SctpClient::rcv(Packet &pkt) {
	int status;
		status = g_nw.read_sctp_pkt(conn_fd, pkt);
		g_utils.handle_type2_error(status, "Read error: sctpclient_rcv");
}

SctpClient::~SctpClient() {
	//cout<<"Closing conn"<<endl;
	close(conn_fd);
}
