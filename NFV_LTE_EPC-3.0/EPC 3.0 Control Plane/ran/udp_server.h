#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "utils.h"

class UdpServer {
private:
	/* Address parameters */
	int port;
	string ip_addr;
	struct sockaddr_in sock_addr;

	void init(string, int);
	
public:
	int conn_fd;

	UdpServer();
	void run(string, int);
	void snd(struct sockaddr_in, Packet);
	void rcv(struct sockaddr_in&, Packet&);
	~UdpServer();
};

#endif /* UDP_SERVER_H */