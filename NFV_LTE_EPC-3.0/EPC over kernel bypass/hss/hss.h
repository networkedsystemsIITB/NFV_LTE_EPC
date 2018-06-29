#ifndef HSS_H
#define HSS_H

#include "diameter.h"
#include "gtp.h"
#include "mysql.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_server.h"
#include "sync.h"
#include "utils.h"
#include <map>

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