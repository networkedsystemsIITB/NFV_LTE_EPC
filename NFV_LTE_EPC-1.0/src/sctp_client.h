#ifndef SCTP_CLIENT_H
#define SCTP_CLIENT_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

class SctpClient {
private:
	/* Address parameters */
	int conn_fd;
	
	/* Server address parameters */
	int server_port;
	string server_ip_addr;
	struct sockaddr_in server_sock_addr;

	void init(string, int);
	void connect_with_server();
	
public:
	SctpClient();
	void conn(string, int);
	void snd(Packet);
	void rcv(Packet&);
	~SctpClient();
};

#endif /* SCTP_CLIENT_H */