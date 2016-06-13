#include "sink_server.h"

int g_threads_count;
vector<thread> g_threads;
thread g_mon_thread;
TrafficMonitor g_traf_mon;

void traffic_monitor() {
	fd_set rcv_set;
	int max_fd;
	int status;

	max_fd = max(g_traf_mon.server.conn_fd, g_traf_mon.tun.conn_fd);
	while (1) {
		FD_ZERO(&rcv_set);
		FD_SET(g_traf_mon.server.conn_fd, &rcv_set); 
		FD_SET(g_traf_mon.tun.conn_fd, &rcv_set); 
		
		status = select(max_fd + 1, &rcv_set, NULL, NULL, NULL);
		g_utils.handle_type1_error(status, "select error: sinkserver_trafficmonitor");		
		
		if (FD_ISSET(g_traf_mon.server.conn_fd, &rcv_set)) {
			g_traf_mon.handle_uplink_udata();
		}
		else if (FD_ISSET(g_traf_mon.tun.conn_fd, &rcv_set)) {
			g_traf_mon.handle_downlink_udata();
		}
	}
}

void sink(int sink_num) {
	string cmd;
	int port;

	port = (sink_num + 55000);
	cmd = "iperf3 -s -B 172.16.0.2 -p " + to_string(port);
	system(cmd.c_str());
}

void check_usage(int argc) {
	if (argc < 2) {
		TRACE(cout << "Usage: ./<sink_server_exec> THREADS_COUNT" << endl;)
		g_utils.handle_type1_error(-1, "Invalid usage error: sinkserver_checkusage");
	}
}

void init(char *argv[]) {
	g_threads_count = atoi(argv[1]);
	g_threads.resize(g_threads_count);
}

void run() {
	int i;

	g_nw.add_itf(0, "172.16.0.2/8");

	/* Tun */
	g_traf_mon.tun.set_itf("tun1", "172.16.0.1/16");
	g_traf_mon.tun.conn("tun1");

	/* Traffic monitor server */
	TRACE(cout << "Traffic monitor server started" << endl;)
	g_traf_mon.server.run(g_sink_ip_addr, g_sink_port);

	g_mon_thread = thread(traffic_monitor);
	g_mon_thread.detach();
	for (i = 0; i < g_threads_count; i++) {
		g_threads[i] = thread(sink, i);
	}	
	for (i = 0; i < g_threads_count; i++) {
		if (g_threads[i].joinable()) {
			g_threads[i].join();
		}
	}	
}

int main(int argc, char *argv[]) {
	check_usage(argc);
	init(argv);
	run();
	return 0;
}
