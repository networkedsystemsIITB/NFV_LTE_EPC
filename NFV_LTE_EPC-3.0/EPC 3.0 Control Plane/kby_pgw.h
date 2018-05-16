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
	uint32_t idfr;		//Key used to identify UE
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
	1		:	Packet from SGW, take action according to packet gtp header:
				1: Third Attach - Process packet and send to SGW
				4: Detach - Received detach request from SGW, destroy state and send packet to SGW,
							close connection with sgw and erase mdata.
*/

//used to send/receive packets
char * dataptr;
unsigned char data[BUF_SIZE];

/*
These definitions are required for PGW
*/
string g_sgw_s5_ip_addr = sgw_ip;
string g_pgw_s5_ip_addr = pgw_ip;
string g_pgw_sgi_ip_addr = pgw_ip;
string g_sink_ip_addr = sink_ip;
int sgw_s5_port = sgw_s5_portnum;
int pgw_s5_port = pgw_s5_portnum;
int pgw_sgi_port = pgw_sgi_portnum;
int sink_port = sink_portnum;

//UE context class specific to PGW
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

UeContext::UeContext() {
	ip_addr = "";
	tai = 0; 
	apn_in_use = 0; 
	s5_uteid_ul = 0; 
	s5_uteid_dl = 0; 
	s5_cteid_ul = 0;
	s5_cteid_dl = 0;
}

void UeContext::init(string arg_ip_addr, uint64_t arg_tai, uint64_t arg_apn_in_use, uint8_t arg_eps_bearer_id, uint32_t arg_s5_uteid_ul, uint32_t arg_s5_uteid_dl, uint32_t arg_s5_cteid_ul, uint32_t arg_s5_cteid_dl) {
	ip_addr = arg_ip_addr;
	tai = arg_tai; 
	apn_in_use = arg_apn_in_use; 
	eps_bearer_id = arg_eps_bearer_id; 
	s5_uteid_ul = arg_s5_uteid_ul; 
	s5_uteid_dl = arg_s5_uteid_dl; 
	s5_cteid_ul = arg_s5_cteid_ul;
	s5_cteid_dl = arg_s5_cteid_dl;
}

UeContext::~UeContext() {

}

//Not needed for single core
pthread_mutex_t s5id_mux; /* Handles s5_id */
pthread_mutex_t sgiid_mux; /* Handles sgi_id */
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
void handle_pgw_accept(int pgw_listen_fd);

void pgw_send_a3(int cur_fd, Packet pkt);