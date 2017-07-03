#ifndef SINK_H
#define SINK_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/tun.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

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
