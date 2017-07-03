#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

class UdpClient {
private:
	/* Address parameters */
	int conn_fd;
	int port;
	string ip_addr;
	struct sockaddr_in sock_addr;

	/* Server parameters */
	int server_port;
	string server_ip_addr;
	struct sockaddr_in server_sock_addr;

	void init(string, string, int);
	void set_port();
	
public:
	UdpClient();
	void conn(string, string, int);
	void set_client(string);
	void set_server(string, int);
	void snd(Packet);
	void rcv(Packet&);
	~UdpClient();		
};

#endif /* UDP_CLIENT_H */