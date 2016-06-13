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
	~UeContext();
};

class Pgw {
private:
	unordered_map<uint32_t, uint64_t> s5_id; /* S5 UE identification table: s5_cteid_ul -> imsi */
	unordered_map<string, uint64_t> sgi_id; /* SGI UE identification table: ue_ip_addr -> imsi */
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: imsi -> UeContext */

	/* IP addresses table - Write once, Read always table - No need to put mlock */ 
	unordered_map<uint64_t, string> ip_addrs; 

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

	Pgw();
	void handle_create_session(struct sockaddr_in, Packet);
	void handle_uplink_udata(Packet, UdpClient&);
	void handle_downlink_udata(Packet, UdpClient&);
	void handle_detach(struct sockaddr_in, Packet);
	~Pgw();
};

#endif /* PGW_H */