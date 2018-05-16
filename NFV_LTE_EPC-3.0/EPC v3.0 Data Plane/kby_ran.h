#define _LARGEFILE64_SOURCE

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
#include <chrono>
#include <ctime>
#include "defport.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>

#include "cpu.h"
#include "debug.h"
#include "mtcp_api.h"
#include "mtcp_epoll.h"


#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <time.h>
#include <unordered_map>


#define MAX_EVENTS 65536
#define no_of_connections 5
#define packet_size 1448
#define transfer_time 300

using namespace std;

//variable declaration

//end of variable declaration

class RanContext {
public:
	/* EMM state
	 * 0 - Deregistered
	 * 1 - Registered
	 */
	int emm_state; /* EPS Mobililty Management state */

	/* ECM state
	 * 0 - Disconnected
	 * 1 - Connected
	 * 2 - Idle
	 */
	int ecm_state; /* EPS Connection Management state */

	/* UE id */
	uint64_t imsi; /* International Mobile Subscriber Identity */
	uint64_t guti; /* Globally Unique Temporary Identifier */
	string ip_addr;
	uint32_t enodeb_s1ap_ue_id; /* eNodeB S1AP UE ID */
	uint32_t mme_s1ap_ue_id; /* MME S1AP UE ID */

	/* UE location info */
	uint64_t tai; /* Tracking Area Identifier */
	vector<uint64_t> tai_list; /* Tracking Area Identifier list */
	uint64_t tau_timer;

	/* UE security context */
	uint64_t key; /* Primary key used in generating secondary keys */
	uint64_t k_asme; /* Key for Access Security Management Entity */
	uint64_t ksi_asme; /* Key Selection Identifier for Access Security Management Entity */
	uint64_t k_nas_enc; /* Key for NAS Encryption / Decryption */
	uint64_t k_nas_int; /* Key for NAS Integrity check */
	uint64_t nas_enc_algo; /* Idenitifier of NAS Encryption / Decryption */
	uint64_t nas_int_algo; /* Idenitifier of NAS Integrity check */
	uint64_t count;
	uint64_t bearer;
	uint64_t dir;

	/* EPS info, EPS bearer info */
	uint64_t apn_in_use; /* Access Point Name in Use */
	uint8_t eps_bearer_id; /* Evolved Packet System Bearer ID */
	uint8_t e_rab_id; /* Evolved Radio Access Bearer ID */
	uint32_t s1_uteid_ul; /* S1 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s1_uteid_dl; /* S1 Userplane Tunnel Endpoint Identifier - Downlink */

	/* Network Operator info */
	uint16_t mcc; /* Mobile Country Code */
	uint16_t mnc; /* Mobile Network Code */
	uint16_t plmn_id; /* Public Land Mobile Network ID */
	uint64_t msisdn; /* Mobile Station International Subscriber Directory Number - Mobile number */
	uint16_t nw_capability;

	/* Parameters added for hand-over  */
	uint16_t eNodeB_id; /* EnodeB identifier */ // we could send this at all msg
	uint16_t handover_target_eNodeB_id; /* EnodeB identifier */

	bool inHandover; /* handover phase*/

	//when inHandover = true use the below uplink
	uint32_t indirect_s1_uteid_ul;
	// handover changes end/

	RanContext();
	void init(uint32_t);
	~RanContext();
};

class EpcAddrs {
public:
	int mme_port;
	int sgw_s1_port;
	string mme_ip_addr;
	string sgw_s1_ip_addr;

	EpcAddrs();
	~EpcAddrs();
};

class Ran {
private:
	//EpcAddrs epc_addrs;
	//SctpClient mme_client;


public:
	/* Parameters added for hand-over  */

	bool inHandover; /* handover phase*/
 	//when inHandover = true use the below uplink
	uint32_t indirect_s1_uteid_ul;
	// handover changes end/

	/* Parameters added for hand-over  */
	uint16_t eNodeB_id; /* EnodeB identifier */
	uint16_t handover_target_eNodeB_id; /* EnodeB identifier */
	int handover_state;

	RanContext ran_ctx;
	Packet pkt;
	void init(int);

};



RanContext::RanContext() {
	emm_state = 0; 
	ecm_state = 0; 
	imsi = 0; 
	guti = 0; 
	ip_addr = "";
	enodeb_s1ap_ue_id = 0; 
	mme_s1ap_ue_id = 0; 
	tai = 1; 
	tau_timer = 0;
	key = 0; 
	k_asme = 0; 
	ksi_asme = 7; 
	k_nas_enc = 0; 
	k_nas_int = 0; 
	nas_enc_algo = 0; 
	nas_int_algo = 0; 
	count = 1;
	bearer = 0;
	dir = 0;
	apn_in_use = 0; 
	eps_bearer_id = 0; 
	e_rab_id = 0; 
	s1_uteid_ul = 0; 
	s1_uteid_dl = 0; 
	mcc = 1; 
	mnc = 1; 
	plmn_id = g_telecom.get_plmn_id(mcc, mnc);
	msisdn = 0; 
	nw_capability = 1;
}

void RanContext::init(uint32_t arg) {
	enodeb_s1ap_ue_id = arg;
	key = arg;
	msisdn = 9000000000 + arg;
	imsi = g_telecom.get_imsi(plmn_id, msisdn);
}

EpcAddrs::EpcAddrs() {
	mme_port = 5000;
	sgw_s1_port = 0;
	mme_ip_addr = "169.254.9.8";	
	sgw_s1_ip_addr = "";
}

EpcAddrs::~EpcAddrs() {

}

RanContext::~RanContext() {

}
