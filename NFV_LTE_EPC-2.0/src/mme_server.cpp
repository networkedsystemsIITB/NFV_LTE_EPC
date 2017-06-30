#include "mme_server.h"

Mme g_mme;
int g_workers_count;
vector<SctpClient> hss_clients;
vector<UdpClient> sgw_s11_clients;
vector<UdpClient> mme_rep_clients1;
vector<UdpClient> mme_rep_clients2;

vector<thread> mme_rep_server_threads;
void check_usage(int argc) {
	if (argc < 2) {
		TRACE(cout << "Usage: ./<mme_server_exec> THREADS_COUNT" << endl;)
				g_utils.handle_type1_error(-1, "Invalid usage error: mmeserver_checkusage");
	}
}

void init(char *argv[]) {
	g_workers_count = atoi(argv[1]);
	hss_clients.resize(g_workers_count);
	sgw_s11_clients.resize(g_workers_count);
	g_mme.initialize_kvstore_clients(g_workers_count);
	//cout<<"came here2"<<endl;
	mme_rep_clients1.resize(g_workers_count);
	mme_rep_clients2.resize(g_workers_count);

	mme_rep_server_threads.resize(g_workers_count);

	signal(SIGPIPE, SIG_IGN);
}

void handle_replication(int worker_id) {
//	UdpClient pgw_s5_client;
	struct sockaddr_in src_sock_addr;
	 Packet pkt;//cout<<".game ."<<endl;
	 while (1) {
	 	//
		g_mme.rep_server.rcv(src_sock_addr, pkt);
		//pkt.extract_gtp_hdr();
		g_mme.receive_replication(pkt,worker_id);	
	 }
}

void run2() {

int i;
	//if(REP){	
	g_mme.rep_server.run(g_mme_ip_addr, g_mme_rep_port);
	for (i = 0; i < 50; i++) {
		mme_rep_server_threads[i] = thread(handle_replication,i);//cout<<"replllication"<<endl;
		
	}
		//cout<<"thread runner"<<endl;
}
void run3(){
	int i;
	for (i = 0; i < 50; i++) {
		if (mme_rep_server_threads[i].joinable()) {
			mme_rep_server_threads[i].join();
		}
	}
	

}
void run() {
	int i;
	run2();cout<<"MME started"<<endl;
	TRACE(cout << "MME server started" << endl;)

	for (i = 0; i < g_workers_count; i++) {
		hss_clients[i].conn(g_hss_ip_addr, g_hss_port);	
		sgw_s11_clients[i].conn(g_mme_ip_addr, g_sgw_s11_ip_addr, g_sgw_s11_port);
		mme_rep_clients1[i].conn(g_mme_ip_addr, g_mme_rep_ip1, g_mme_rep_port);
		mme_rep_clients2[i].conn(g_mme_ip_addr, g_mme_rep_ip2, g_mme_rep_port);


	}
	cout<<"hss connected"<<endl;

	g_mme.server.run(g_mme_ip_addr, g_mme_port, g_workers_count, handle_ue);
	run3();

}

int handle_ue(int conn_fd, int worker_id) {
	//cout<<"hiiii"<<endl;
	bool res;
	Packet pkt;

	g_mme.server.rcv(conn_fd, pkt);
	if (pkt.len <= 0) {
		TRACE(cout << "mmeserver_handleue:" << " Connection closed" << endl;)
				return 0;
	}
	pkt.extract_s1ap_hdr();
	if (pkt.s1ap_hdr.mme_s1ap_ue_id == 0) {
		switch (pkt.s1ap_hdr.msg_type) {
		/* Initial Attach request */
		case 1:
			TRACE(cout << "mmeserver_handleue:" << " case 1: initial attach" << endl;)
			g_mme.handle_initial_attach(conn_fd, pkt, hss_clients[worker_id],worker_id);
			break;

			/* For error handling */
		default:
			TRACE(cout << "mmeserver_handleue:" << " default case: new" << endl;)
			break;
		}		
	}
	else if (pkt.s1ap_hdr.mme_s1ap_ue_id > 0) {
		switch (pkt.s1ap_hdr.msg_type) {
		/* Authentication response */
		case 2:
			TRACE(cout << "mmeserver_handleue:" << " case 2: authentication response" << endl;)
			res = g_mme.handle_autn(conn_fd, pkt,worker_id);
			
				//cout<<"came heree"<<__LINE__<<":"<<res<<endl;

			if (res) {
				g_mme.handle_security_mode_cmd(conn_fd, pkt,worker_id);
			}
			break;

			/* Security Mode Complete */
		case 3:
			TRACE(cout << "mmeserver_handleue:" << " case 3: security mode complete" << endl;)

			res = g_mme.handle_security_mode_complete(conn_fd, pkt,worker_id);
			if (res) {
				g_mme.handle_create_session(conn_fd, pkt, sgw_s11_clients,sgw_s11_clients[worker_id],worker_id);
			}
			break;

			/* Attach Complete */
		case 4:
			TRACE(cout << "mmeserver_handleue:" << " case 4: attach complete" << endl;)

			g_mme.handle_attach_complete(pkt,worker_id);
			g_mme.handle_modify_bearer(conn_fd, pkt, sgw_s11_clients,sgw_s11_clients[worker_id],worker_id);
			break;

			/* Detach request */
		case 5:
			TRACE(cout << "mmeserver_handleue:" << " case 5: detach request" << endl;)

			g_mme.handle_detach(conn_fd, pkt, sgw_s11_clients,sgw_s11_clients[worker_id],worker_id);

			break;

			/* For error handling */	
		default:
			TRACE(cout << "mmeserver_handleue:" << " default case: attached" << endl;)
			break;
		}				
	}		
	return 1;
}

int main(int argc, char *argv[]) {
	cout<<"MME running in MODE:"<<UE_BINDING<<endl;

	check_usage(argc);
	init(argv);
	run();
	return 0;
}
