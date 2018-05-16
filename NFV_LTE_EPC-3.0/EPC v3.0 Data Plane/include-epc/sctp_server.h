#ifndef SCTP_SERVER_H
#define SCTP_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "utils.h"

class SctpServer {
private:
	/* Address parameters */
	int listen_fd;
	int port;
	string ip_addr;
	struct sockaddr_in sock_addr;

	/* Thread pool parameters */
	int workers_count;
	vector<thread> workers;	
	int (*serve_client)(int, int);

	/* Pipe parameter - for communication between main thread and worker threads */
	vector<int*> pipe_fds;

	void init(string, int, int, int (*)(int, int));
	void init_pipe_fds();
	void create_workers();
	void worker_func(int);
	void accept_clients();
	
public:
	SctpServer();
	void run(string, int, int, int (*)(int, int));
	void snd(int, Packet);
	void rcv(int, Packet&);
	~SctpServer();
};

#endif /* SCTP_SERVER_H */