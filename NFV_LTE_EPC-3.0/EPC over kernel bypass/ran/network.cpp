#include "network.h"

socklen_t g_sock_addr_len = sizeof(sockaddr_in);
int g_reuse = 1;
int g_freeport = 0;
struct timeval g_timeout_lev1 = {5, 0};
struct timeval g_timeout_lev2 = {30, 0};
struct timeval g_timeout_lev3 = {60, 0};
Network g_nw;

void Network::set_inet_sock_addr(string ip_addr, int port, struct sockaddr_in &sock_addr) {
	int status;
	
	bzero((void*)&sock_addr, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	status = inet_aton(ip_addr.c_str(), &sock_addr.sin_addr);	
	if (status == 0) {
		g_utils.handle_type1_error(-1, "inet_aton error: network_setinetsockaddr");
	}
}

void Network::bind_sock(int sock_fd, struct sockaddr_in sock_addr) {
	int status;
	
	status = bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	g_utils.handle_type1_error(status, "Bind error: network_bindsock");
}

void Network::get_sock_addr(int sock_fd, struct sockaddr_in &sock_addr) {
	int status;

	status = getsockname(sock_fd, (struct sockaddr*)&sock_addr, &g_sock_addr_len);
	g_utils.handle_type1_error(status, "Getsockname error: network_getsockaddr");
}

void Network::set_sock_reuse(int sock_fd) {
	int status;

	status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &g_reuse, sizeof(int));
	g_utils.handle_type1_error(status, "Setsockopt reuse error: network_setsockreuse");
}

void Network::set_rcv_timeout(int sock_fd, int level) {
	int status;

	switch (level) {
		/* Timeout period = 5s */
		case 1:
			status = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&g_timeout_lev1, sizeof(struct timeval));
			break;

		/* Timeout period = 30s */
		case 2:
			status = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&g_timeout_lev2, sizeof(struct timeval));
			break;

		/* Timeout period = 60s */
		case 3:
			status = setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&g_timeout_lev3, sizeof(struct timeval));
			break;

		default:
			g_utils.handle_type1_error(status, "Incorrect timeout level: network_setrcvtimeout");
	}
	g_utils.handle_type1_error(status, "Setsockopt rcv timeout error: network_setrcvtimeout");
}

string Network::get_src_ip_addr(Packet pkt) {
	struct ip *ip_hdr;
	string str_ip_addr;
	char *cstr_ip_addr;

	ip_hdr = pkt.allocate_ip_hdr_mem(IP_HDR_LEN);
	cstr_ip_addr = g_utils.allocate_str_mem(INET_ADDRSTRLEN);
	memmove(ip_hdr, pkt.data, IP_HDR_LEN * sizeof(uint8_t));
	if (inet_ntop(AF_INET, &(ip_hdr->ip_src), cstr_ip_addr, INET_ADDRSTRLEN) == NULL) {
		g_utils.handle_type1_error(-1, "inet_ntop error: network_getsrcipaddr");
	}
	str_ip_addr.assign(cstr_ip_addr);
	free(ip_hdr);
	free(cstr_ip_addr);
	return str_ip_addr;
}

string Network::get_dst_ip_addr(Packet pkt) {
	struct ip *ip_hdr;
	string str_ip_addr;
	char *cstr_ip_addr;

	ip_hdr = pkt.allocate_ip_hdr_mem(IP_HDR_LEN);
	cstr_ip_addr = g_utils.allocate_str_mem(INET_ADDRSTRLEN);
	memmove(ip_hdr, pkt.data, IP_HDR_LEN * sizeof(uint8_t));
	if (inet_ntop(AF_INET, &(ip_hdr->ip_dst), cstr_ip_addr, INET_ADDRSTRLEN) == NULL) {
		g_utils.handle_type1_error(-1, "inet_ntop error: network_getdstipaddr");
	}
	str_ip_addr.assign(cstr_ip_addr);
	free(ip_hdr);
	free(cstr_ip_addr);
	return str_ip_addr;
}

void Network::add_itf(int itf_no, string ip_addr_sp) {
	string cmd;

	cmd = "sudo ifconfig eth0:" + to_string(itf_no) + " " + ip_addr_sp;
	system(cmd.c_str());
}

void Network::rem_itf(int itf_no) {
	string cmd;

	cmd = "sudo ifconfig eth0:" + to_string(itf_no) + " down";
	system(cmd.c_str());
}

int Network::read_stream(int conn_fd, uint8_t *buf, int len) {
	int ptr;
	int retval;
	int read_bytes;
	int remaining_bytes;

	ptr = 0;
	remaining_bytes = len;
	if (conn_fd < 0 || len <= 0) {
		return -1;
	}
	while (1) {
		read_bytes = read(conn_fd, buf + ptr, remaining_bytes);
		if (read_bytes <= 0) {
			retval = read_bytes;
			break;
		}
		ptr += read_bytes;
		remaining_bytes -= read_bytes;
		if (remaining_bytes == 0) {
			retval = len;
			break;
		}
	}
	return retval;
}

int Network::write_stream(int conn_fd, uint8_t *buf, int len) {
	int ptr;
	int retval;
	int written_bytes;
	int remaining_bytes;

	ptr = 0;
	remaining_bytes = len;
	if (conn_fd < 0 || len <= 0) {
		return -1;
	}	
	while (1) {
		written_bytes = write(conn_fd, buf + ptr, remaining_bytes);
		if (written_bytes <= 0) {
			retval = written_bytes;
			break;
		}
		ptr += written_bytes;
		remaining_bytes -= written_bytes;
		if (remaining_bytes == 0) {
			retval = len;
			break;
		}
	}
	return retval;
}

int Network::read_sctp_pkt(int conn_fd, Packet &pkt) {
	int retval;
	int pkt_len;

	pkt.clear_pkt();
	retval = read_stream(conn_fd, pkt.data, sizeof(int));
	if (retval > 0) {
		memmove(&pkt_len, pkt.data, sizeof(int) * sizeof(uint8_t));
		pkt.clear_pkt();
		retval = read_stream(conn_fd, pkt.data, pkt_len);
		pkt.data_ptr = 0;
		pkt.len = retval;
	}
	return retval;
}

int Network::write_sctp_pkt(int conn_fd, Packet pkt) {
	int retval;

	pkt.prepend_len();
	retval = write_stream(conn_fd, pkt.data, pkt.len);
	return retval;
}