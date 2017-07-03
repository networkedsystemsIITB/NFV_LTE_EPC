#ifndef RAN_SIMULATOR_H
#define RAN_SIMULATOR_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/ran.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/sctp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/sctp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/security.h"
#include "../../NFV_LTE_EPC-1.0/src/sync.h"
#include "../../NFV_LTE_EPC-1.0/src/telecom.h"
#include "../../NFV_LTE_EPC-1.0/src/tun.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

#define NUM_MONITORS 50

extern time_t g_start_time;
extern int g_threads_count;
extern uint64_t g_req_duration;
extern uint64_t g_run_duration;
extern int g_tot_regs;
extern uint64_t g_tot_regstime;
extern pthread_mutex_t g_mux;
extern vector<thread> g_umon_thread;
extern vector<thread> g_dmon_thread;
extern vector<thread> g_threads;
extern thread g_rtt_thread;
extern TrafficMonitor g_traf_mon;

void utraffic_monitor();
void dtraffic_monitor();
void simulate(int);
void ping();
void check_usage(int);
void init(char**);
void run();
void print_results();
//handover changes
SctpServer server;

//Ran ranS;
//Ran ranT;
string g_ran_sctp_ip_addr = "10.129.26.169";
int g_ran_port = 4905;

//hochanges
int handle_mme_conn(int,int);
void simulateHandover();
#endif /* RAN_SIMULATOR_H */
