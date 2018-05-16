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
	uint32_t tid;		//Key used to identify UE
	uint64_t guti;		//Key used to identify UE
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
	1		:	Packet from MME, take action according to packet gtp header:
				1: Third Attach - Connect to PGW (goto 2)
				2: Fourth Attach - Process packet and send reply to MME
				3: Detach - Send to PGW, wait for reply (goto 4)
				
	2		:	Connection to PGW established, 
				send packet to pgw, wait for reply (goto 3)
 	3		:	Received packet from PGW, process and send to MME, 
 				wait for next request from mme (goto 1)
	4		:	Received detach packet from PGW, destroy state and send to MME,
				close connection with pgw and mme, erase mdata.
*/

//used to send/receive packets
char * dataptr;
unsigned char data[BUF_SIZE];

/*
These definitions are required for SGW
*/
string g_sgw_s11_ip_addr = sgw_ip;
string g_sgw_s1_ip_addr = sgw_ip;
string g_sgw_s5_ip_addr = sgw_ip;
int sgw_s11_port = sgw_s11_portnum;
int sgw_s1_port = sgw_s1_portnum;
int sgw_s5_port = sgw_s5_portnum;

//UE context class specific to SGW
class UeContext {
public:
	/* UE location info */
	uint64_t tai; /* Tracking Area Identifier */

	/* EPS session info */
	uint64_t apn_in_use; /* Access Point Name in Use */

	/* EPS Bearer info */
	uint8_t eps_bearer_id; /* Evolved Packet System Bearer Id */
	uint32_t s1_uteid_ul; /* S1 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s1_uteid_dl; /* S1 Userplane Tunnel Endpoint Identifier - Downlink */
	uint32_t s5_uteid_ul; /* S5 Userplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s5_uteid_dl; /* S5 Userplane Tunnel Endpoint Identifier - Downlink */
	uint32_t s11_cteid_mme; /* S11 Controlplane Tunnel Endpoint Identifier - MME */
	uint32_t s11_cteid_sgw; /* S11 Controlplane Tunnel Endpoint Identifier - SGW */
	uint32_t s5_cteid_ul; /* S5 Controlplane Tunnel Endpoint Identifier - Uplink */
	uint32_t s5_cteid_dl; /* S5 Controlplane Tunnel Endpoint Identifier - Downlink */

	/* PGW info */
	string pgw_s5_ip_addr;
	int pgw_s5_port;

	/* eNodeB info */
	string enodeb_ip_addr;
	int enodeb_port;

	UeContext();
	void init(uint64_t, uint64_t, uint8_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, string, int);
	~UeContext();
};


UeContext::UeContext() {
	tai = 0; 
	apn_in_use = 0; 
	eps_bearer_id = 0;
	s1_uteid_ul = 0; 
	s1_uteid_dl = 0; 
	s5_uteid_ul = 0; 
	s5_uteid_dl = 0; 
	s11_cteid_mme = 0;
	s11_cteid_sgw = 0;
	s5_cteid_ul = 0;
	s5_cteid_dl = 0;
	pgw_s5_ip_addr = "";
	pgw_s5_port = 0;
	enodeb_ip_addr = "";
	enodeb_port = 0;	
}

void UeContext::init(uint64_t arg_tai, uint64_t arg_apn_in_use, uint8_t arg_eps_bearer_id, uint32_t arg_s1_uteid_ul, uint32_t arg_s5_uteid_dl, uint32_t arg_s11_cteid_mme, uint32_t arg_s11_cteid_sgw, uint32_t arg_s5_cteid_dl, string arg_pgw_s5_ip_addr, int arg_pgw_s5_port) {
	tai = arg_tai; 
	apn_in_use = arg_apn_in_use;
	eps_bearer_id = arg_eps_bearer_id;
	s1_uteid_ul = arg_s1_uteid_ul;
	s5_uteid_dl = arg_s5_uteid_dl;
	s11_cteid_mme = arg_s11_cteid_mme;
	s11_cteid_sgw = arg_s11_cteid_sgw;
	s5_cteid_dl = arg_s5_cteid_dl;
	pgw_s5_ip_addr = arg_pgw_s5_ip_addr;
	pgw_s5_port = arg_pgw_s5_port;
}

UeContext::~UeContext() {

}

//Not needed for single core
	pthread_mutex_t s11id_mux; /* Handles s11_id */
	pthread_mutex_t s1id_mux; /* Handles s1_id */
	pthread_mutex_t s5id_mux; /* Handles s5_id */
	pthread_mutex_t uectx_mux; /* Handles ue_ctx */


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
	tem = tem.substr(7, -1); /* Extracting only the last 9 digits of UE MSISDN */
	s11_cteid_mme = stoull(tem);
	return s11_cteid_mme;
}

//Function Definitions
void handle_sgw_accept(int sgw_listen_fd);

void handle_pgw_connect(int cur_fd,Packet pkt,struct mdata fddata, string pgw_s5_ip_addr,int pgw_s5_port);

void send_request_pgw(int cur_fd, struct mdata fddata);

void send_attach_four(int cur_fd, Packet pkt);

void send_pgw_detach(int sgw_fd, struct mdata fddata, Packet pkt);

