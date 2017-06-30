#include "pgw_server.h"

int g_s5_server_threads_count;
int g_sgi_server_threads_count;
vector<thread> g_s5_server_threads;
vector<thread> g_sgi_server_threads;
vector<UdpClient> sgw_s5_clients;

vector<UdpClient> pgw_rep1_clients;
vector<UdpClient> pgw_rep2_clients;

vector<thread> g_rep_server_threads;

Pgw g_pgw;

void check_usage(int argc) {
	if (argc < 3) {
		TRACE(cout << "Usage: ./<pgw_server_exec> S5_SERVER_THREADS_COUNT SGI_SERVER_THREADS_COUNT" << endl;)
						g_utils.handle_type1_error(-1, "Invalid usage error: pgwserver_checkusage");
	}
}

void init(char *argv[]) {
	g_s5_server_threads_count = atoi(argv[1]);
	g_sgi_server_threads_count = atoi(argv[2]);
	g_s5_server_threads.resize(g_s5_server_threads_count);
	g_sgi_server_threads.resize(g_sgi_server_threads_count);
	g_pgw.initialize_kvstore_clients(g_s5_server_threads_count);

	g_rep_server_threads.resize(g_s5_server_threads_count);
	pgw_rep1_clients.resize(g_s5_server_threads_count);
	pgw_rep2_clients.resize(g_s5_server_threads_count);


	signal(SIGPIPE, SIG_IGN);
}

void run() {
	int i;

	/* downlink clients */
	sgw_s5_clients.resize(g_s5_server_threads_count);
	for (i = 0; i < g_sgi_server_threads_count; i++) {
		sgw_s5_clients[i].conn(PGW1,SGWLB,7200);
		pgw_rep1_clients[i].conn(PGW1, g_pgw_rep1_ip, g_pgw_rep_port);
		pgw_rep2_clients[i].conn(PGW1, g_pgw_rep2_ip, g_pgw_rep_port);

	}
	/* PGW S5 server */
	TRACE(cout << "PGW S5 server started" << endl;)
	g_pgw.s5_server.run(g_pgw_s5_ip_addr, g_pgw_s5_port);
	for (i = 0; i < g_s5_server_threads_count; i++) {
		g_s5_server_threads[i] = thread(handle_s5_traffic,i);
	}

	/* PGW SGI server */
	TRACE(cout << "PGW SGI server started" << endl;)
	g_pgw.sgi_server.run(g_pgw_sgi_ip_addr, g_pgw_sgi_port);
	for (i = 0; i < g_sgi_server_threads_count; i++) {
		g_sgi_server_threads[i] = thread(handle_sgi_traffic,i);
	}


	/* Replication server */
	TRACE(cout << "SGW Replication server started" << endl;)
	g_pgw.rep_server.run(g_pgw_s5_ip_addr, g_pgw_rep_port);
	for (i = 0; i < g_s5_server_threads_count; i++) {
		g_rep_server_threads[i] = thread(handle_replication,i);
	}

	/* Joining all threads */
	for (i = 0; i < g_s5_server_threads_count; i++) {
		if (g_s5_server_threads[i].joinable()) {
			g_s5_server_threads[i].join();
		}
	}	
	for (i = 0; i < g_sgi_server_threads_count; i++) {
		if (g_sgi_server_threads[i].joinable()) {
			g_sgi_server_threads[i].join();
		}
	}				
for (i = 0; i < g_s5_server_threads_count; i++) {
		if (g_rep_server_threads[i].joinable()) {
			g_rep_server_threads[i].join();
		}
	}

}
void handle_replication(int worker_id) {
	struct sockaddr_in src_sock_addr;
	 Packet pkt;
	 while (1) {	
		g_pgw.rep_server.rcv(src_sock_addr, pkt);
		g_pgw.receive_replication(pkt,worker_id);	
	 }
}
void handle_s5_traffic(int worker_id) {
	UdpClient sink_client;
	struct sockaddr_in src_sock_addr;
	Packet pkt;

	sink_client.set_client(g_pgw_sgi_ip_addr);
	while (1) {
		g_pgw.s5_server.rcv(src_sock_addr, pkt);
		pkt.extract_gtp_hdr();
		switch(pkt.gtp_hdr.msg_type) {
		/* Create session */
		case 1:
			TRACE(cout << "pgwserver_handles5traffic:" << " case 1: create session" << endl;	)
			g_pgw.handle_create_session(src_sock_addr, pkt,worker_id);
			break;

			/* Uplink userplane data */
		case 2:
			TRACE(cout << "pgwserver_handles5traffic:" << " case 2: uplink udata" << endl;	)
			g_pgw.handle_uplink_udata(pkt, sink_client,worker_id);
			break;

			/* Detach */
		case 4:
			TRACE(cout << "pgwserver_handles5traffic:" << " case 4: detach" << endl;	)
			g_pgw.handle_detach(src_sock_addr, pkt,worker_id);
			break;

			/* For error handling */
		default:
			TRACE(cout << "pgwserver_handles5traffic:" << " default case:" << endl;	)
		}		
	}
}
int getIndex(Packet pkt){

	//if(NOCACHE) return 0;

	int size = g_s5_server_threads_count;
	string ip = g_nw.get_dst_ip_addr(pkt);
		//cout<<"down link data..."<<ip<<endl;

	ip = ip.substr(ip.find_last_of('.')+1, ip.size());
	int index = stoi(ip);
	index = index % size;
	return index;
}
void handle_sgi_traffic(int worker_id) {
	UdpClient sgw_s5_client;
	struct sockaddr_in src_sock_addr;
	Packet pkt;

	sgw_s5_client.set_client(g_pgw_s5_ip_addr);
	while (1) {
		g_pgw.sgi_server.rcv(src_sock_addr, pkt);

		/* Downlink userplane data */
		TRACE(cout << "pgwserver_handlesgitraffic: downlink udata" << endl;	)
		g_pgw.handle_downlink_udata(pkt, sgw_s5_clients[getIndex(pkt)],worker_id);
	}	
}

int main(int argc, char *argv[]) {
	cout<<"PGW running in MODE:"<<UE_BINDING<<endl;

	check_usage(argc);
	init(argv);
	run();
	return 0;
}
