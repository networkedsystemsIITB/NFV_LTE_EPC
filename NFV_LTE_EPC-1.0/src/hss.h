#ifndef HSS_H
#define HSS_H

#include "../../NFV_LTE_EPC-1.0/src/diameter.h"
#include "../../NFV_LTE_EPC-1.0/src/gtp.h"
#include "../../NFV_LTE_EPC-1.0/src/mysql.h"
#include "../../NFV_LTE_EPC-1.0/src/network.h"
#include "../../NFV_LTE_EPC-1.0/src/packet.h"
#include "../../NFV_LTE_EPC-1.0/src/s1ap.h"
#include "../../NFV_LTE_EPC-1.0/src/sctp_server.h"
#include "../../NFV_LTE_EPC-1.0/src/sync.h"
#include "../../NFV_LTE_EPC-1.0/src/utils.h"

extern string g_hss_ip_addr;
extern int g_hss_port;

class Hss {
private:
	pthread_mutex_t mysql_client_mux;
	
	void get_autn_info(uint64_t, uint64_t&, uint64_t&);
	void set_loc_info(uint64_t, uint32_t);
	
public:
	SctpServer server;
	MySql mysql_client;

	Hss();
	void handle_mysql_conn();
	void handle_autninfo_req(int, Packet&);
	void handle_location_update(int, Packet&);
	~Hss();
};

#endif //HSS_H