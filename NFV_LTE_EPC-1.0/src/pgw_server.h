#ifndef PGW_SERVER_H
#define PGW_SERVER_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/pgw.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/sync.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

extern int g_s5_server_threads_count;
extern int g_sgi_server_threads_count;
extern vector<thread> g_s5_server_threads;
extern vector<thread> g_sgi_server_threads;
extern Pgw g_pgw;

void check_usage(int);
void init(char**);
void run();
void handle_s5_traffic();
void handle_sgi_traffic();

#endif /* PGW_SERVER_H */