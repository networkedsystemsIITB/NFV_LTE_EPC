#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

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