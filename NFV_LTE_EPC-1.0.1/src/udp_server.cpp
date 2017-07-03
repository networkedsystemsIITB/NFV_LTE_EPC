#include "udp_server.h"

UdpServer::UdpServer() {
	conn_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	g_utils.handle_type1_error(conn_fd, "Socket error: udpserver_udpserver");		
}

void UdpServer::run(string arg_ip_addr, int arg_port) {
	init(arg_ip_addr, arg_port);
	g_nw.set_sock_reuse(conn_fd);
	g_nw.bind_sock(conn_fd, sock_addr);
}

void UdpServer::init(string arg_ip_addr, int arg_port) {
	int status;

	port = arg_port;
	ip_addr = arg_ip_addr;
	g_nw.set_inet_sock_addr(ip_addr, port, sock_addr);
}

void UdpServer::snd(struct sockaddr_in dst_sock_addr, Packet pkt){
	int status;

	while (1) {
		status = sendto(conn_fd, pkt.data, pkt.len, 0, (sockaddr*)&dst_sock_addr, g_sock_addr_len);
		if (errno == EPERM) {
			TRACE(cout << "Too many writes to a socket - error: udpserver_snd" << endl;)
			errno = 0;
			usleep(1000);
			continue;
		}
		else {
			break;
		}
	}
	g_utils.handle_type1_error(status, "Sendto error: udpserver_snd");
}

void UdpServer::rcv(struct sockaddr_in &src_sock_addr, Packet &pkt) {
	int status;

	pkt.clear_pkt();
	status = recvfrom(conn_fd, pkt.data, BUF_SIZE, 0, (sockaddr*)&src_sock_addr, &g_sock_addr_len);
	g_utils.handle_type1_error(status, "Recvfrom error: udpserver_rcv");
	pkt.data_ptr = 0;
	pkt.len = status;
}

UdpServer::~UdpServer() {
	close(conn_fd);
}
