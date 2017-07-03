#ifndef MME_SERVER_H
#define MME_SERVER_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/mme.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/sctp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/sctp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/security.h"
#include "../../NFV_LTE_EPC-1.0/src/sync.h"
#include "../../NFV_LTE_EPC-1.0/src/telecom.h"
#include "../../NFV_LTE_EPC-1.0/src/udp_client.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

extern Mme g_mme;
extern int g_workers_count;
extern vector<SctpClient> hss_clients;
extern vector<UdpClient> sgw_s11_clients;

void check_usage(int);
void init(char**);
void run();
int handle_ue(int, int);

#endif /* MME_SERVER_H */
