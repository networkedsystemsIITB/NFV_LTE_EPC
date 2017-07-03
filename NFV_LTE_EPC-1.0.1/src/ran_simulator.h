#ifndef RAN_SIMULATOR_H
#define RAN_SIMULATOR_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "ran.h"
#include "s1ap.h"
#include "sctp_client.h"
#include "sctp_server.h"
#include "security.h"
#include "sync.h"
#include "telecom.h"
#include "tun.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"

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
