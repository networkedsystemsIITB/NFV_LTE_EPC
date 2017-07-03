#ifndef SINK_SERVER_H
#define SINK_SERVER_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/sink.h"
#include "../../NFV_LTE_EPC-1.0/src/tun.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

extern int g_threads_count;
extern vector<thread> g_threads;
extern thread g_mon_thread;
extern TrafficMonitor g_traf_mon;

void traffic_monitor();
void sink(int);
void check_usage(int);
void init(char**);
void run();

#endif /* SINK_SERVER_H */