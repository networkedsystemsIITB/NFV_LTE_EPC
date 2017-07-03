#ifndef SINK_H
#define SINK_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "tun.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"

extern string g_pgw_sgi_ip_addr;
extern string g_sink_ip_addr;
extern int g_pgw_sgi_port;
extern int g_sink_port;

class TrafficMonitor {
public:
	Tun tun;
	UdpServer server;

	void handle_uplink_udata();
	void handle_downlink_udata();
};

#endif /* SINK_H */
