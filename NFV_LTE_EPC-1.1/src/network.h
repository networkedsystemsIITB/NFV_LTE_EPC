#ifndef NETWORK_H
#define NETWORK_H

/* (C) inet_aton, inet_ntop */
#include <arpa/inet.h>

/* (C) open */
#include <fcntl.h>

/* (C) IFF_TUN, IFF_NO_PI, TUNSETIFF */
#include <linux/if_tun.h>

/* (C) ifreq, IFNAMSIZ */
#include <net/if.h>

/* (C) ioctl */
#include <sys/ioctl.h>

/* (C) select */
#include <sys/select.h>

/* (C) socket */
#include <sys/socket.h>

/* (C) read, write, close */
#include <unistd.h>

#include "diameter.h"
#include "gtp.h"
#include "packet.h"
#include "s1ap.h"
#include "utils.h"

extern socklen_t g_sock_addr_len;
extern int g_reuse;
extern int g_freeport;
extern struct timeval g_timeout_lev1;
extern struct timeval g_timeout_lev2;
extern struct timeval g_timeout_lev3;

class Network {
private:
	int read_stream(int, uint8_t*, int);
	int write_stream(int, uint8_t*, int);
	
public:
	void set_inet_sock_addr(string, int, struct sockaddr_in&);
	void bind_sock(int, struct sockaddr_in);
	void get_sock_addr(int, struct sockaddr_in&);
	void set_sock_reuse(int);
	void set_rcv_timeout(int, int);
	string get_src_ip_addr(Packet);
	string get_dst_ip_addr(Packet);
	void add_itf(int, string);
	void rem_itf(int);
	int read_sctp_pkt(int, Packet&);
	int write_sctp_pkt(int, Packet);
};

extern Network g_nw;

#endif /* NETWORK_H */