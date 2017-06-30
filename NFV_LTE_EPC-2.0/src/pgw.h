	#ifndef PGW_H
#define PGW_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sync.h"
#include "udp_client.h"
#include "udp_server.h"
#include "utils.h"



extern string g_sgw_s5_ip_addr;
extern string g_pgw_s5_ip_addr;
extern string g_pgw_sgi_ip_addr;
extern string g_sink_ip_addr;
extern int g_sgw_s5_port;
extern int g_pgw_s5_port;
extern int g_pgw_sgi_port;
extern int g_sink_port;
//const string dspgw_path = "10.129.26.173:8090";
//const string dspgw_path = "10.129.28.44:7001";
const string dspgw_path = PDS;
extern int g_pgw_rep_port;
extern vector<UdpClient> pgw_rep1_clients;
extern vector<UdpClient> pgw_rep2_clients;

extern string g_pgw_rep1_ip;
extern string g_pgw_rep2_ip;


class UeContext {
public:
	/* UE id */
	string ip_addr;	

	/* UE location info */
	uint64_t tai; /* Tracking Area Identifier */

	/* EPS session info */
	uint64_t apn_in_use; /* Access Point Name in Use */

	/* EPS bearer info */
	uint8_t eps_bearer_id;
	uint32_t s5_uteid_ul; /* S5 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s5_uteid_dl; /* S5 Userplane Tunnel Endpoint Identifier - Downlink */
	uint32_t s5_cteid_ul; /* S5 Controlplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s5_cteid_dl; /* S5 Controlplane Tunnel Endpoint Identifier - Downlink */

	UeContext();
	void init(string, uint64_t, uint64_t, uint8_t, uint32_t, uint32_t, uint32_t, uint32_t);
	template<class Archive>
		void serialize(Archive &ar, const unsigned int version);
	~UeContext();
};
class Pgw_state{
public:
UeContext Pgw_state_uect;
uint64_t imsi;

//for serializability
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version);
	Pgw_state(UeContext,uint64_t);
	Pgw_state();

};
class Pgw {
private:
	unordered_map<uint32_t, uint64_t> s5_id;  //S5 UE identification table: s5_cteid_ul -> imsi
	unordered_map<string, uint64_t> sgi_id; // SGI UE identification table: ue_ip_addr -> imsi
	unordered_map<uint64_t, UeContext> ue_ctx; // UE context table: imsi -> UeContext

	 //IP addresses table - Write once, Read always table - No need to put mlock
	unordered_map<uint64_t, string> ip_addrs;

	vector<KVStore<uint32_t,Pgw_state>> ds_s5_id;
	vector<KVStore<string,Pgw_state>> ds_sgi_id;
//	vector<KVStore<uint64_t,string>> ds_ip_addrs;
	vector<KVRequest> ds_all;

	/* Lock parameters */
	pthread_mutex_t s5id_mux; /* Handles s5_id */
	pthread_mutex_t sgiid_mux; /* Handles sgi_id */
	pthread_mutex_t uectx_mux; /* Handles ue_ctx */
	
	void clrstl();
	void set_ip_addrs();
	void update_itfid(int, uint32_t, string, uint64_t);
	uint64_t get_imsi(int, uint32_t, string);
	bool get_downlink_info(uint64_t, uint32_t&);	
	void rem_itfid(int, uint32_t, string);
	void rem_uectx(uint64_t);

public:
	UdpServer s5_server;
	UdpServer sgi_server;
	UdpServer rep_server;

	Pgw();
	void handle_create_session(struct sockaddr_in, Packet,int);
	void handle_uplink_udata(Packet, UdpClient&,int);
	void handle_downlink_udata(Packet, UdpClient&,int);
	void handle_detach(struct sockaddr_in, Packet,int);

	uint64_t retrive_sgi_id(string ip_addr);
	string retrive_ip_addrs(uint64_t guti);
	uint64_t retrive_s5_id(uint32_t teid);

	void initialize_kvstore_clients(int );
	void push_context(uint64_t ,UeContext ,int );

	void pull_data_context(string ,uint32_t& ,uint64_t& , int );

	void pull_context(Packet,uint64_t&,UeContext&,int);
	void erase_context(uint32_t ,string ,int );

	void send_replication(int,Pgw_state ,uint32_t ,string ,uint64_t,int );
	void receive_replication(Packet,int);

	~Pgw();
};

#endif /* PGW_H */
