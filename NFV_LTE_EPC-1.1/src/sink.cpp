#include "sink.h"
#include <algorithm>

string g_pgw_sgi_ip_addr = "10.129.26.201";
string g_sink_ip_addr = "10.129.28.19";
int g_pgw_sgi_port = 8100;
int g_sink_port = 8500;

void TrafficMonitor::handle_uplink_udata() {
	struct sockaddr_in src_sock_addr;
	Packet pkt;

	server.rcv(src_sock_addr, pkt);
	tun.snd(pkt);


}

void TrafficMonitor::handle_downlink_udata(vector<UdpClient>& pgw_sgi_clients) {
	Packet pkt;

	tun.rcv(pkt);
	string ip = g_nw.get_dst_ip_addr(pkt);
	replace(ip.begin(), ip.end(), '.', '0' );
	int index = stoll(ip);
	index += rand();
	index = index % pgw_sgi_clients.size();
	index = (index + rand()) % pgw_sgi_clients.size();

	pgw_sgi_clients[index].snd(pkt);

}
