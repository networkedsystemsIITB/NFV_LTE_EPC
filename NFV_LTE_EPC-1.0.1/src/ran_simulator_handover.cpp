#include "ran_simulator.h"

time_t g_start_time;
int g_threads_count;
uint64_t g_req_dur;
uint64_t g_run_dur;
int g_tot_regs;
uint64_t g_tot_regstime;
pthread_mutex_t g_mux;
vector<thread> g_umon_thread;
vector<thread> g_dmon_thread;
vector<thread> g_threads;
thread g_rtt_thread;
TrafficMonitor g_traf_mon;

Ran ranS; //Source ran for handover operation
Ran ranT; //Target ran for handover operation

void utraffic_monitor() {
	UdpClient sgw_s1_client;
	
	sgw_s1_client.set_client(g_trafmon_ip_addr);
	while (1) {
		g_traf_mon.handle_uplink_udata(sgw_s1_client);
	}	
}

void dtraffic_monitor() {
	while (1) {
		g_traf_mon.handle_downlink_udata();		
	}
}

void ping(){
	string cmd;
	
	cmd = "ping -I 172.16.1.3 172.16.0.2 -c 60 | grep \"^rtt\" >> ping.txt";
	cout << cmd << endl;
	system(cmd.c_str());
}

//This function handles all incoming MME connection for Handover purposes
int handle_mme_conn(int conn_fd,int dummy) {

	bool res;
	Packet pkt;

	server.rcv(conn_fd, pkt);
	pkt.extract_s1ap_hdr();
	if (pkt.s1ap_hdr.mme_s1ap_ue_id > 0) {
		switch (pkt.s1ap_hdr.msg_type) {
		
		//packet numbers from 1-6 are already used for attach and detach operations
		case 7:
			TRACE(cout << "Target Ran: handle handover request:" << " case 7:" << endl;)
			ranT.handle_handover(pkt);

			break;
			
		case 8:
			TRACE(cout << "Source Ran:receive indirect tunnel from sgw:" << " case 8:" << endl;)
			ranS.indirect_tunnel_complete(pkt);
			break;
			
		case 9:
			TRACE(cout << "Source Ran to intiate a teardown for indirect teid:" << " case 9:" << endl;)
			ranS.request_tear_down(pkt);

			break;
			
			/* For error handling */
		default:
			TRACE(cout << "ran_simulator_handle_mme:" << " default case: handover" << endl;)
			break;
 		}
 	}
	return 1;
}


void handover_traffic_monitor() {

	server.run(g_ran_sctp_ip_addr, g_ran_port, 1, handle_mme_conn);
	
} 

void simulateHandover() {
	

		TRACE(cout<<"simulating.Handover between sRan and tRan\n";)

		CLOCK::time_point start_time;
		CLOCK::time_point stop_time;
		MICROSECONDS time_diff_us;
		
		int status;
		int ran_num;
		bool ok;
		ranS.handover_state = 0; //not in handover;

		ranS.init(1);// source enodeb intialization
		ranT.init(2);// target enodeb initialization

		ranS.conn_mme();

		ranS.initial_attach();
		ok = ranS.authenticate();
		
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " autn failure" << endl;)
		}
		
		ok = ranS.set_security();
		
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " security setup failure" << endl;)
		}
		
		ok = ranS.set_eps_session(g_traf_mon);
		
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " eps session setup failure" << endl;)
		}
		
		//attach complete
		start_time = CLOCK::now();
		cout<<"initiate Handover\n";
		ranS.initiate_handover();

		//sleep for some time to simulate disconnection and rejoining of UE to EnodeBs, then teardown
		usleep(500000);
		cout<<"simulating UE disconnection/reconnection from enodeBs.."<<endl;
		//here ranT signals that ue has connected to tRan and its ready to take over
		//this resultss in switching of down link to target enodeb and tearing down of indirect tunnel
		ranT.complete_handover();
		/* Stop time */
		stop_time = CLOCK::now();

		/* Response time */
		time_diff_us = std::chrono::duration_cast<MICROSECONDS>((stop_time - start_time));
		
		cout<<endl<<endl;
		cout <<"Measured uration for handover between source ran to target ran "<< time_diff_us.count()-500000<<" \n";
			//ok = ranT.detach();
			
		cout <<"*****Handover Completed***** in "<<time_diff_us.count()-500000<<"ms"<<endl;
		
}
 
 void print_results() {
 	g_run_dur = difftime(time(0), g_start_time);
 	cout << "Requested duration has ended. Finishing the program." << endl;
 }
void simulate(int arg) {
	CLOCK::time_point mstart_time;
	CLOCK::time_point mstop_time;
	MICROSECONDS mtime_diff_us;		
	Ran ran;
	int status;
	int ran_num;
	bool ok;
	bool time_exceeded;

	ran_num = arg;
	time_exceeded = false;
	ran.init(ran_num);
	ran.conn_mme();

	while (1) {
		// Run duration check
		g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
		if (time_exceeded) {
			break;
		}
		// Start time
		mstart_time = CLOCK::now();	

		// Initial attach
		ran.initial_attach();

		// Authentication
		ok = ran.authenticate();
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " autn failure" << endl;)
			return;
		}

		// Set security
		ok = ran.set_security();
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " security setup failure" << endl;)
			return;
		}

		// Set eps session
		ok = ran.set_eps_session(g_traf_mon);
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " eps session setup failure" << endl;)
			return;
		}

		/*
		// To find RTT
		if (ran_num == 0) {
			g_rtt_thread = thread(ping);
			g_rtt_thread.detach();		
		}
		*/ 

		/* Data transfer */
		ran.transfer_data(g_req_dur);
		
		// Detach
		ok = ran.detach();
		if (!ok) {
			TRACE(cout << "ransimulator_simulate:" << " detach failure" << endl;)
			return;
		}

		// Stop time
		mstop_time = CLOCK::now();
		
		// Response time
		mtime_diff_us = std::chrono::duration_cast<MICROSECONDS>(mstop_time - mstart_time);

		/* Updating performance metrics */
		g_sync.mlock(g_mux);
		g_tot_regs++;
		g_tot_regstime += mtime_diff_us.count();		
		g_sync.munlock(g_mux);			
	}
}

void check_usage(int argc) {
	if (argc < 3) {
		TRACE(cout << "Usage: ./<ran_simulator_exec> THREADS_COUNT DURATION" << endl;)
		g_utils.handle_type1_error(-1, "Invalid usage error: ransimulator_checkusage");
	}
}

void init(char *argv[]) {
	g_start_time = time(0);
	//g_threads_count = atoi(argv[1]);
	g_req_dur = atoi(argv[2]);
	g_tot_regs = 0;
	g_tot_regstime = 0;
	g_sync.mux_init(g_mux);	
	g_umon_thread.resize(NUM_MONITORS);
	g_dmon_thread.resize(NUM_MONITORS);
	g_threads.resize(2);
	signal(SIGPIPE, SIG_IGN);
}

void run_extra() {
	int i;

	/* Tun */
	g_traf_mon.tun.set_itf("tun1", "172.16.0.1/16");
	g_traf_mon.tun.conn("tun1");

	/* Traffic monitor server */
	TRACE(cout << "Traffic monitor server started" << endl;)
	g_traf_mon.server.run(g_trafmon_ip_addr, g_trafmon_port);	

	// Uplink traffic monitor
	for (i = 0; i < NUM_MONITORS; i++) {
		g_umon_thread[i] = thread(utraffic_monitor);
		g_umon_thread[i].detach();		
	}

	// Downlink traffic monitor
	for (i = 0; i < NUM_MONITORS; i++) {
		g_dmon_thread[i] = thread(dtraffic_monitor);
		g_dmon_thread[i].detach();			
	}
	
	// Simulator threads
	for (i = 0; i < g_threads_count; i++) {
		g_threads[i] = thread(simulate, i);
	}	
	for (i = 0; i < g_threads_count; i++) {
		if (g_threads[i].joinable()) {
			g_threads[i].join();
		}
	}	
}

void run() {
		
	g_threads[0] = thread(handover_traffic_monitor);	
	g_threads[1] = thread(simulateHandover);
	
	if (g_threads[0].joinable())g_threads[0].join();	
	if (g_threads[1].joinable())g_threads[1].join();
				
}

void print_results_extra() {
	g_run_dur = difftime(time(0), g_start_time);
	
	cout << endl << endl;
	cout << "Requested duration has ended. Finishing the program." << endl;
	cout << "Total number of registrations is " << g_tot_regs << endl;
	cout << "Total time for registrations is " << g_tot_regstime * 1e-6 << " seconds" << endl;
	cout << "Total run duration is " << g_run_dur << " seconds" << endl;
	cout << "Latency is " << ((double)g_tot_regstime/g_tot_regs) * 1e-6 << " seconds" << endl;
	cout << "Throughput is " << ((double)g_tot_regs/g_run_dur) << endl;	
}

int main(int argc, char *argv[]) {
	check_usage(argc);
	init(argv);
	run();
	print_results();
	return 0;
}
