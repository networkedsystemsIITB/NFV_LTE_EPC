#ifndef HSS_SERVER_H
#define HSS_SERVER_H

#include "diameter.h"
#include "gtp.h"
#include "hss.h"
#include "mysql.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_server.h"
#include "sync.h"
#include "utils.h"

extern Hss g_hss;
extern int g_workers_count;

void check_usage(int);
void init(char**);
void run();
int handle_mme(int, int);
void finish();

#endif //HSS_SERVER_H
