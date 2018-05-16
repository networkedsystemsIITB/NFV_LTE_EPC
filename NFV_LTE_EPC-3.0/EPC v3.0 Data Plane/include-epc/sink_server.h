#ifndef SINK_SERVER_H
#define SINK_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sink.h"
#include "tun.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"

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