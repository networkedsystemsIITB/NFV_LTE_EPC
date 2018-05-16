#include "common.h"


#define MAXCONN 500
#define MAXEVENTS 500
/*
MTCP/Epoll specific data
*/
int done;
mctx_t mctx;
int epollfd;
struct mtcp_epoll_event epevent;

void SignalHandler(int signum)
{
	//Handle ctrl+C here
	mtcp_destroy_context(mctx);	
	mtcp_destroy();
	done = 1;
}

/*
mdata is file descriptor specific data.
In event driven programming, state specific to a file descriptor needs to be maintained by the application.
In mdata, the required information is stored
*/
struct mdata{
	int act;			//Action to be taken on event generated for this fd.
	int initial_fd;		//Corresponding file descriptor that initiated the connection
	uint8_t buf[500];	//Store packet data if connection is being established, send when connection established
	int buflen;			//length of packet data stored
	uint32_t msui;		//Key used to identify UE (mme_s1ap_ue_id)
};

/*
Map to store each file descriptor's corresponding information
*/
map<int, mdata> fdmap;

/*FDMAP 
	key : file descriptor whose state needs to be maintained
	value: mdata - state type ("act"), and corresponding required information
 	
 	Action type(act) and corresponding action to be taken
	Value	:	Current State and/or Action Required
	1		:	Packet from RAN, take action according to packet header:
				0: First Attach - Connect to HSS (goto 2)
				2: Second Attach - Process packet and send reply to RAN
				3: Third Attach - Connect to SGW (goto 4)
				4: Fourth Attach - Send to SGW, wait for reply (goto 6)
				5: Detach - Send to SGW, wait for reply (goto 7)			
	2		:	Connection to HSS established, 
				send packet to hss, wait for reply (goto 3)
 	3		:	Received packet from HSS, process and send to RAN, 
 				close hss connection, wait for next request from ran (goto 1)
 	4		:	Connection to SGW established, 
				send packet to sgw, wait for reply (goto 5)	
	5		:	Received attach 3 packet from SGW, process and send to RAN,
				wait for next request from ran (goto 1)
	6		: 	Received attach 4 packet from SGW, process and send to RAN,
				wait for next request from ran (goto 1)
	7		:	Received detach packet from SGW, destroy state and send to RAN,
				close connection with sgw and ran, erase mdata.
*/



//used to send/receive packets
char * dataptr;
unsigned char data[BUF_SIZE];



/*
These definitions are required for MME
*/

uint64_t ue_count = 0;

//locks, used for multicore, not single core
pthread_mutex_t s1mmeid_mux; /* Handles s1mme_id and ue_count */
pthread_mutex_t uectx_mux; /* Handles ue_ctx */

string g_trafmon_ip_addr = ran_ip;// not used in control plane
string g_mme_ip_addr = mme_ip;
string g_hss_ip_addr = hss_ip;
string g_sgw_s11_ip_addr = sgw_ip;
string g_sgw_s1_ip_addr = sgw_ip;
string g_sgw_s5_ip_addr = sgw_ip;
string g_pgw_s5_ip_addr = pgw_ip;

int g_trafmon_port = 4000;
int g_mme_port = 5000;
int g_hss_port = 6000;
int g_sgw_s11_port = 7000;
int g_sgw_s1_port = 7100;
int g_sgw_s5_port = 7200;
int g_pgw_s5_port = 8000;

uint64_t g_timer = 100;

//UE context class
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

void UeContext::init(uint64_t arg_imsi, uint32_t arg_enodeb_s1ap_ue_id, uint32_t arg_mme_s1ap_ue_id, uint64_t arg_tai, uint16_t arg_nw_capability) {
	imsi = arg_imsi;
	enodeb_s1ap_ue_id = arg_enodeb_s1ap_ue_id;
	mme_s1ap_ue_id = arg_mme_s1ap_ue_id;
	tai = arg_tai;
	nw_capability = arg_nw_capability;
}

UeContext::~UeContext() {
}

MmeIds::MmeIds() {
	mcc = 1;
	mnc = 1;
	plmn_id = g_telecom.get_plmn_id(mcc, mnc);
	mmegi = 1;
	mmec = 1;
	mmei = g_telecom.get_mmei(mmegi, mmec);
	gummei = g_telecom.get_gummei(plmn_id, mmei);
}

MmeIds::~MmeIds() {
	
}

UeContext::UeContext() {
	emm_state = 0;
	ecm_state = 0;
	imsi = 0;
	string ip_addr = "";
	enodeb_s1ap_ue_id = 0;
	mme_s1ap_ue_id = 0;
	tai = 0;
	tau_timer = 0;
	ksi_asme = 0;
	k_asme = 0; 
	k_nas_enc = 0; 
	k_nas_int = 0; 
	nas_enc_algo = 0; 
	nas_int_algo = 0; 
	count = 1;
	bearer = 0;
	dir = 1;
	default_apn = 0; 
	apn_in_use = 0; 
	eps_bearer_id = 0; 
	e_rab_id = 0;
	s1_uteid_ul = 0; 
	s1_uteid_dl = 0; 
	s5_uteid_ul = 0; 
	s5_uteid_dl = 0; 
	xres = 0;
	nw_type = 0;
	nw_capability = 0;	
	pgw_s5_ip_addr = "";	
	pgw_s5_port = 0;
	s11_cteid_mme = 0;
	s11_cteid_sgw = 0;	
}

/*
Initialize, lock and unlock pthread locks
Not used in single core
*/
void mux_init(pthread_mutex_t &mux) {
	pthread_mutex_init(&mux, NULL);
}

void mlock(pthread_mutex_t &mux) {
	int status;

	status = pthread_mutex_lock(&mux);
	if(status)
	cout<<status<<"Lock error: sync_mlock"<<endl;
}

void munlock(pthread_mutex_t &mux) {
	int status;

	status = pthread_mutex_unlock(&mux);
	if(status)
	cout<<status<<"Unlock error: sync_munlock"<<endl;
}

//support functions
int make_socket_nb(int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      cout<<"Error: NBS fcntl"<<'\n';
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      cout<<"Error: NBS fcntl flags"<<'\n';
      return -1;
    }

  return 0;
}

uint32_t gettid(uint64_t guti) {
	uint32_t s11_cteid_mme;
	string tem;

	tem = to_string(guti);
	tem = tem.substr(7, -1);
	s11_cteid_mme = stoull(tem);
	return s11_cteid_mme;
}

//function definitions
void handle_ran_accept(int ran_listen_fd);

void handle_hss_connect(int cur_fd, Packet pkt, int mme_s1ap_ue_id);

void send_request_hss(int cur_fd, struct mdata fddata);

void send_ran_attach_one(int cur_fd, Packet pkt);

void send_ran_attach_two(int cur_fd, Packet pkt);

void handle_sgw_connect(int cur_fd, Packet pkt, int msui);

void send_request_sgw_athree(int cur_fd, struct mdata fddata);

void send_sgw_afour(int cur_fd, struct mdata fddata, Packet pkt);

void send_sgw_detach(int cur_fd, struct mdata fddata, Packet pkt);