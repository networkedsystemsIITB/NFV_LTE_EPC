#ifndef SGW_SERVER_H
#define SGW_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sgw.h"
#include "sync.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"

extern int g_s11_server_threads_count;
extern int g_s1_server_threads_count;
extern int g_s5_server_threads_count;
extern vector<thread> g_s11_server_threads;
extern vector<thread> g_s1_server_threads;
extern vector<thread> g_s5_server_threads;
extern Sgw g_sgw;

void check_usage(int);
void init(char**);
void run();
void handle_s11_traffic();
void handle_s1_traffic();
void handle_s5_traffic();

#endif /* SGW_SERVER_H */