#ifndef HSS_H
#define HSS_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_server.h"
#include "sync.h"
#include "utils.h"

//const string ds_path = PDS;
const string ds_path = "10.129.28.188:8090";




using namespace kvstore;

extern string g_hss_ip_addr;
extern int g_hss_port;
class Authinfo {
public:
	uint64_t key_id;
	uint64_t rand_num;

	template<class Archive>
    void serialize(Archive &ar, const unsigned int version);

};
class Hss {
private:
	pthread_mutex_t mysql_client_mux;

	vector<KVStore<uint64_t,Authinfo>> ds_autn_infos;
	vector<KVStore<uint64_t,uint32_t>> ds_loc_infos;

	void get_autn_info(uint64_t, uint64_t&, uint64_t&, int);
	void set_loc_info(uint64_t, uint32_t, int);
	
public:
	SctpServer server;

	Hss();
	void handle_mysql_conn();
	void initialize_kvstore_clients(int);
	void handle_autninfo_req(int, Packet&, int);
	void handle_location_update(int, Packet&, int);
	~Hss();
};


#endif //HSS_H
