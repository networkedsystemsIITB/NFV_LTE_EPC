#ifndef MME_H
#define MME_H

#include "diameter.h"
#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sctp_client.h"
#include "sctp_server.h"
#include "security.h"
#include "sync.h"
#include "telecom.h"
#include "udp_client.h"
#include "utils.h"

extern string g_trafmon_ip_addr;
extern string g_mme_ip_addr;
extern string g_hss_ip_addr;
extern string g_sgw_s11_ip_addr;
extern string g_sgw_s1_ip_addr;
extern string g_sgw_s5_ip_addr;
extern string g_pgw_s5_ip_addr;
extern int g_trafmon_port;
extern int g_mme_port;
extern int g_hss_port;
extern int g_sgw_s11_port;
extern int g_sgw_s1_port;
extern int g_sgw_s5_port;
extern int g_pgw_s5_port;
extern uint64_t g_timer;

class UeContext {
public:
	/* EMM state 
	 * 0 - Deregistered
	 * 1 - Registered */
	int emm_state; /* EPS Mobililty Management state */

	/* ECM state 
	 * 0 - Disconnected
	 * 1 - Connected 
	 * 2 - Idle */	 
	int ecm_state; /* EPS Connection Management state */

	/* UE id */
	uint64_t imsi; /* International Mobile Subscriber Identity */
	string ip_addr;
	uint32_t enodeb_s1ap_ue_id; /* eNodeB S1AP UE ID */
	uint32_t mme_s1ap_ue_id; /* MME S1AP UE ID */

	/* UE location info */
	uint64_t tai; /* Tracking Area Identifier */
	vector<uint64_t> tai_list; /* Tracking Area Identifier list */
	uint64_t tau_timer; /* Tracking area update timer */

	/* UE security context */
	uint64_t ksi_asme; /* Key Selection Identifier for Access Security Management Entity */	
	uint64_t k_asme; /* Key for Access Security Management Entity */	
	uint64_t k_enodeb; /* Key for Access Stratum */	
	uint64_t k_nas_enc; /* Key for NAS Encryption / Decryption */
	uint64_t k_nas_int; /* Key for NAS Integrity check */
	uint64_t nas_enc_algo; /* Idenitifier of NAS Encryption / Decryption */
	uint64_t nas_int_algo; /* Idenitifier of NAS Integrity check */
	uint64_t count;
	uint64_t bearer;
	uint64_t dir;

	/* EPS info, EPS bearer info */
	uint64_t default_apn; /* Default Access Point Name */
	uint64_t apn_in_use; /* Access Point Name in Use */
	uint8_t eps_bearer_id; /* Evolved Packet System Bearer ID */
	uint8_t e_rab_id; /* Evolved Radio Access Bearer ID */	
	uint32_t s1_uteid_ul; /* S1 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s1_uteid_dl; /* S1 Userplane Tunnel Endpoint Identifier - Downlink */
	uint32_t s5_uteid_ul; /* S5 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s5_uteid_dl; /* S5 Userplane Tunnel Endpoint Identifier - Downlink */

	/* Authentication info */ 
	uint64_t xres;

	/* UE Operator network info */
	uint16_t nw_type;
	uint16_t nw_capability;

	/* PGW info */
	string pgw_s5_ip_addr;
	int pgw_s5_port;

	/* Control plane info */
	uint32_t s11_cteid_mme; /* S11 Controlplane Tunnel Endpoint Identifier - MME */
	uint32_t s11_cteid_sgw; /* S11 Controlplane Tunnel Endpoint Identifier - SGW */

	UeContext();
	void init(uint64_t, uint32_t, uint32_t, uint64_t, uint16_t);
	~UeContext();
};

class MmeIds {
public:
	uint16_t mcc; /* Mobile Country Code */
	uint16_t mnc; /* Mobile Network Code */
	uint16_t plmn_id; /* Public Land Mobile Network ID */
	uint16_t mmegi; /* MME Group Identifier */
	uint8_t mmec; /* MME Code */
	uint32_t mmei; /* MME Identifier */
	uint64_t gummei; /* Globally Unique MME Identifier */

	MmeIds();
	~MmeIds();
};

class Mme {
private:
	MmeIds mme_ids;
	uint64_t ue_count;
	unordered_map<uint32_t, uint64_t> s1mme_id; /* S1_MME UE identification table: mme_s1ap_ue_id -> guti */
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */

	/* Lock parameters */
	pthread_mutex_t s1mmeid_mux; /* Handles s1mme_id and ue_count */
	pthread_mutex_t uectx_mux; /* Handles ue_ctx */
	
	void clrstl();
	uint32_t get_s11cteidmme(uint64_t);
	void set_crypt_context(uint64_t);
	void set_integrity_context(uint64_t);
	void set_pgw_info(uint64_t);
	uint64_t get_guti(Packet);
	void rem_itfid(uint32_t);
	void rem_uectx(uint64_t);
	
public:
	SctpServer server;

	Mme();
	void handle_initial_attach(int, Packet, SctpClient&);
	bool handle_autn(int, Packet);
	void handle_security_mode_cmd(int, Packet);
	bool handle_security_mode_complete(int, Packet);
	void handle_location_update(Packet, SctpClient&);
	void handle_create_session(int, Packet, UdpClient&);
	void handle_attach_complete(Packet);
	void handle_modify_bearer(Packet, UdpClient&);
	void handle_detach(int, Packet, UdpClient&);
	~Mme();
};

#endif /* MME_H */
