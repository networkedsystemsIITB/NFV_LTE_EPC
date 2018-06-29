#include "mkb_sgw.h"

#define MAX_THREADS 1

struct arg arguments[MAX_THREADS]; // Arguments sent to Pthreads
pthread_t servers[MAX_THREADS]; // Threads 

void SignalHandler(int signum)
{
	//Handle ctrl+C here
	done = 1;
	mtcp_destroy();
	
}

//State 
unordered_map<uint32_t, uint64_t> s11_id; /* S11 UE identification table: s11_cteid_sgw -> imsi */
unordered_map<uint32_t, uint64_t> s1_id; /* S1 UE identification table: s1_uteid_ul -> imsi */
unordered_map<uint32_t, uint64_t> s5_id; /* S5 UE identification table: s5_uteid_dl -> imsi */
unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: imsi -> UeContext */

//Not needed for single core
pthread_mutex_t s11id_mux; /* Handles s11_id */
pthread_mutex_t s1id_mux; /* Handles s1_id */
pthread_mutex_t s5id_mux; /* Handles s5_id */
pthread_mutex_t uectx_mux; /* Handles ue_ctx */

//kanase mtcp edits...
static in_addr_t daddr;
static in_port_t dport;
static in_addr_t saddr;
#define IP_RANGE 1
//kanase mtcp done


void *run(void* args)
{
		/*
		MTCP Setup
	*/
//	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */
//	ue_ctx.clear();
	struct arg argument = *((struct arg*)args); // Get argument 
	int core = argument.coreno; 
	mctx_t mctx ; // Get mtcp context

	//step 2. mtcp_core_affinitize
	mtcp_core_affinitize(core);
		
	//step 3. mtcp_create_context. Here order of affinitization and context creation matters.
	// mtcp_epoll_create
	mctx = mtcp_create_context(core);
	if (!mctx) {
		TRACE("Failed to create mtcp context!\n";)
		return NULL;
	}
	
	/* register signal handler to mtcp */
	mtcp_register_signal(SIGINT, SignalHandler);
	// Not reqd in MC ?

	//Kanase mtcp edits
	daddr = inet_addr(pgw_ip);
	dport = htons(pgw_s5_portnum);
	saddr = INADDR_ANY;	
	mtcp_init_rss(mctx, saddr, IP_RANGE, daddr, dport);
	//edits done

	//program specific
	int retval;
	int i,returnval,cur_fd, act_type;
	map<int, mdata> fdmap;
	struct mdata fddata;
	Packet pkt;
	int pkt_len;
	char * dataptr;
	unsigned char data[BUF_SIZE];

	/*
		SGW Specific data
	*/

	uint32_t s1_uteid_ul;
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint32_t s11_cteid_mme;
	uint32_t s11_cteid_sgw;
	uint32_t s5_cteid_ul;
	uint32_t s5_cteid_dl;
	uint64_t imsi;
	uint8_t eps_bearer_id;
	uint64_t apn_in_use;
	uint64_t tai;
	string pgw_s5_ip_addr;
	string ue_ip_addr;
	int pgw_s5_port;
	uint32_t s1_uteid_dl;
	string enodeb_ip_addr;
	int enodeb_port;
	bool res;

	/*
		Server side initialization
	*/
	int listen_fd, mme_fd, pgw_fd, sgw_fd;
	struct sockaddr_in sgw_server_addr, pgw_server_addr;

	listen_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		TRACE(cout<<"Error: SGW listen socket call"<<endl;)
		exit(-1);
	}
	
	retval = mtcp_setsock_nonblock(mctx, listen_fd);
	if(retval < 0)
	{
		TRACE(cout<<"Error: mtcp make nonblock"<<endl;)
		exit(-1);
	}

	bzero((char *) &sgw_server_addr, sizeof(sgw_server_addr));
	sgw_server_addr.sin_family = AF_INET;
	sgw_server_addr.sin_addr.s_addr = inet_addr(sgw_ip);
	sgw_server_addr.sin_port = htons(sgw_s11_port);

	retval = mtcp_bind(mctx, listen_fd, (struct sockaddr *) &sgw_server_addr, sizeof(sgw_server_addr));
	if(retval < 0)
	{
		TRACE(cout<<"Error: mtcp listenfd  bind call"<<endl;)
		exit(-1);
	}

	retval = mtcp_listen(mctx, listen_fd, MAXCONN);
	if(retval < 0)
	{
		TRACE(cout<<"Error: mtcp listen"<<endl;)
		exit(-1);
	}

	/*
		Epoll Setup
	*/
	int epollfd;
	struct mtcp_epoll_event epevent;
	int numevents;
	struct mtcp_epoll_event revent;
	struct mtcp_epoll_event *return_events;
	return_events = (struct mtcp_epoll_event *) malloc (sizeof (struct mtcp_epoll_event) * MAXEVENTS);
	if (!return_events) 
	{
		TRACE(cout<<"Error: malloc failed for revents"<<endl;)
		exit(-1);
	}

	epollfd = mtcp_epoll_create(mctx, MAXEVENTS);
	if(epollfd == -1)
	{
		TRACE(cout<<"Error: mtcp mme epoll_create"<<endl;)
		exit(-1);
	}

	epevent.data.sockid = listen_fd;
	epevent.events = MTCP_EPOLLIN;
	retval = mtcp_epoll_ctl(mctx, epollfd, EPOLL_CTL_ADD, listen_fd, &epevent);
	if(retval == -1)
	{
		TRACE(cout<<"Error: mtcp epoll_ctl_add ran"<<endl;)
		exit(-1);
	}

	int con_accepts = 0;
	int con_processed = 0;
	
	while(!done)
	{
		numevents = mtcp_epoll_wait(mctx, epollfd, return_events, MAXEVENTS, -1);
		
		//epoll errors
		if(numevents < 0)
		{
			cout<<"Error: mtcp wait :"<<errno<<endl;
			if(errno != EINTR)
					cout<<"EINTR error"<<endl;
			exit(-1);
		}

		if(numevents == 0)
		{
			TRACE(cout<<"Info: Return with no events"<<endl;)
		}

		for(int i = 0; i < numevents; ++i)
		{
			//errors in file descriptors
			if( (return_events[i].events & MTCP_EPOLLERR) ||
				(return_events[i].events & MTCP_EPOLLHUP))
			{

				cout<<"\n\nError: epoll event returned : "<<return_events[i].data.sockid<<" errno :"<<errno<<endl;
				if(return_events[i].data.sockid == listen_fd)
				{
					cout<<"Error: In Ran Listen fd"<<endl;
				}
				close(return_events[i].data.sockid);//mtcp
				continue;
			}

			//get an event
			revent = return_events[i];
			
			/*
				Check type of event
			*/

			if(revent.data.sockid == listen_fd) 
			{
				//If event in listening fd, its new connection
				//RAN ACCEPTS
				while(1)
				{
					mme_fd = mtcp_accept(mctx, listen_fd, NULL, NULL);
					if(mme_fd < 0)
					{
						if((errno == EAGAIN) ||	(errno == EWOULDBLOCK))
						{
							break;
						}
						else
						{
							cout<<"mtcp error : error on accept "<<endl;
							exit(-1);
						}
					}
					
					epevent.events = MTCP_EPOLLIN;
					epevent.data.sockid = mme_fd;
					mtcp_setsock_nonblock(mctx, mme_fd);
					retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, mme_fd, &epevent);
					if(retval < 0)
					{
						cout<<"MME accept epoll add error "<<errno<<" retval "<<retval<<" core "<<core<<endl;
						exit(-1);
					}
					fddata.act = 1;
					fddata.initial_fd = -1;
					memset(fddata.buf,'\0',500);
					fddata.buflen = 0;
					fdmap.insert(make_pair(mme_fd, fddata));
					con_accepts++;
				}//while accepts
				TRACE(cout<<" Core "<<core<<" accepted "<<con_accepts<<" till now "<<endl;)
				//go to act_type case 1
			}//if listen fd
			else
			{
				//Check action type
				cur_fd = revent.data.sockid;
				fddata = fdmap[cur_fd];
				act_type = fddata.act;

				switch(act_type)
				{
					case 1:
						if(revent.events & MTCP_EPOLLIN)
						{
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error mme epoll read delete from epoll"<<endl;
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);							
							if(retval == 0)
							{
								cout<<"Connection closed by MME at core "<<core<<endl;
								if(fddata.initial_fd != -1)
								{
									mtcp_close(mctx, fddata.initial_fd);
								}	
								mtcp_close(mctx, cur_fd);
								fdmap.erase(cur_fd);
								continue;
							}
							else
							if(retval < 0)
							{
								cout<<"Error: Ran read data case 1 "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}

							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;

							pkt.extract_gtp_hdr();

							if(pkt.gtp_hdr.msg_type == 1)
							{//MME attach 3

								pkt.extract_item(s11_cteid_mme);
								pkt.extract_item(imsi);
								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(pgw_s5_ip_addr);
								pkt.extract_item(pgw_s5_port);
								pkt.extract_item(apn_in_use);
								pkt.extract_item(tai);
								
								s1_uteid_ul = s11_cteid_mme;
								s5_uteid_dl = s11_cteid_mme;
								s11_cteid_sgw = s11_cteid_mme;
								s5_cteid_dl = s11_cteid_mme;

								mlock(s11id_mux);
								s11_id[s11_cteid_sgw] = imsi;
								munlock(s11id_mux);
								mlock(s1id_mux);
								s1_id[s1_uteid_ul] = imsi;
								munlock(s1id_mux);
								mlock(s5id_mux);
								s5_id[s5_uteid_dl] = imsi;
								munlock(s5id_mux);

								TRACE(cout<<"Attach 3 in"<<imsi<< " "<<s11_cteid_mme<<endl;)
								
								mlock(uectx_mux);
								ue_ctx[imsi].init(tai, apn_in_use, eps_bearer_id, s1_uteid_ul, s5_uteid_dl, s11_cteid_mme, s11_cteid_sgw, s5_cteid_dl, pgw_s5_ip_addr, pgw_s5_port);
								ue_ctx[imsi].tai = tai;
								munlock(uectx_mux);

								pkt.clear_pkt();
								pkt.append_item(s5_cteid_dl);
								pkt.append_item(imsi);
								pkt.append_item(eps_bearer_id);
								pkt.append_item(s5_uteid_dl);
								pkt.append_item(apn_in_use);
								pkt.append_item(tai);
								pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);
								pkt.prepend_len();



								//connect to pgw
								pgw_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
								if(pgw_fd < 0)
								{
									cout<<"Error: pgw new socket fd error"<<endl;
									exit(-1);
								}
								retval = mtcp_setsock_nonblock(mctx, pgw_fd);
								if(retval < 0)
								{
									cout<<"Error: create pgw nonblock"<<endl;
									exit(-1);
								}	

								bzero((char *) &pgw_server_addr, sizeof(pgw_server_addr));
								pgw_server_addr.sin_family = AF_INET;
								pgw_server_addr.sin_addr.s_addr = inet_addr(pgw_s5_ip_addr.c_str());
								pgw_server_addr.sin_port = htons(pgw_s5_port);

								retval = mtcp_connect(mctx, pgw_fd, (struct sockaddr*) &pgw_server_addr, sizeof(struct sockaddr_in));
								if((retval < 0 ) && (errno == EINPROGRESS))
								{
									epevent.events = MTCP_EPOLLOUT;
									epevent.data.sockid = pgw_fd;
									returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, pgw_fd, &epevent);
									if(returnval < 0)
									{
										cout<<"Error: epoll pgw add"<<endl;
										exit(-1);
									}

									fdmap.erase(cur_fd);
									fddata.act = 2;
									fddata.initial_fd = cur_fd;
									fddata.tid = s11_cteid_mme;
									fddata.guti = imsi;
									memcpy(fddata.buf, pkt.data, pkt.len);
									fddata.buflen = pkt.len;
									fdmap.insert(make_pair(pgw_fd, fddata));	
								}
								else
								if(retval < 0)
								{
									cout<<"ERror: pgw connect error"<<endl;
									exit(-1);
								}
								else
								{
									cout<<"Pgw connected immidietly, handle now\n";
									exit(-1);
								}
							}//3rd attach
							else
							if(pkt.gtp_hdr.msg_type == 2)
							{//attach 4 from mme
								mlock(s11id_mux);
								if (s11_id.find(pkt.gtp_hdr.teid) != s11_id.end()) {
								imsi = s11_id[pkt.gtp_hdr.teid];
								}
								else
								{
									cout<<"IMSI not found error"<<endl;
									exit(-1);
								}
								munlock(s11id_mux);

								if (imsi == 0) {
								TRACE(cout << "sgw_handlemodifybearer:" << " :zero imsi " << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
								exit(-1);
								}

								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(s1_uteid_dl);
								pkt.extract_item(enodeb_ip_addr);
								pkt.extract_item(enodeb_port);	
								
								mlock(uectx_mux);
								ue_ctx[imsi].s1_uteid_dl = s1_uteid_dl;
								ue_ctx[imsi].enodeb_ip_addr = enodeb_ip_addr;
								ue_ctx[imsi].enodeb_port = enodeb_port;
								s11_cteid_mme = ue_ctx[imsi].s11_cteid_mme;
								munlock(uectx_mux);

								TRACE(cout<<"In attach 4 "<< imsi<< " "<<s11_cteid_mme << "recv "<< s1_uteid_dl<<endl;)
								res = true;
								pkt.clear_pkt();
								pkt.append_item(res);
								pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_mme);
								pkt.prepend_len();

								retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
								if(retval < 0)
								{
									cout<<"Error MME write back to RAN A2"<<endl;
									exit(-1);
								}

								//fdmap.erase(cur_fd);
								//mtcp_close(mctx, cur_fd);
								//mtcp_close(mctx, fddata.initial_fd);
								//con_processed++;
								//cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;
								
								//fddata.act = 1;
								//fddata.buflen = 0;
								//memset(fddata.buf, '\0', 500);
								//fddata.tid = 0;
								//fddata.guti = 0;
								//fdmap.insert(make_pair(mme_fd, fddata));

								epevent.events = MTCP_EPOLLIN;
								epevent.data.sockid = cur_fd;
								retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, cur_fd, &epevent);
								if(retval < 0)
								{
									TRACE(cout<<"Error adding mme fd for attach 4 to epoll"<<endl;)
									exit(-1);
								}

							}//4th attach
							else
							if(pkt.gtp_hdr.msg_type == 3)
							{
								mlock(s11id_mux);
								if (s11_id.find(pkt.gtp_hdr.teid) != s11_id.end()) {
									imsi = s11_id[pkt.gtp_hdr.teid];
								}
								else
								{
									cout<<"IMSI not detach error"<<endl;
									exit(-1);
								}
								munlock(s11id_mux);

								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(tai);
								
								mlock(uectx_mux);
								if (ue_ctx.find(imsi) == ue_ctx.end()) {
									TRACE(cout << "sgw_handledetach:" << " no uectx: " << imsi << endl;)
									exit(-1);
								}
								munlock(uectx_mux);

								if(gettid(imsi) != pkt.gtp_hdr.teid)
								{
									cout<<"GUTI not equal Detach acc"<<imsi<<" "<<pkt.gtp_hdr.teid<<endl;
									exit(-1);
								}	

								mlock(uectx_mux);
								pgw_s5_ip_addr = ue_ctx[imsi].pgw_s5_ip_addr;
								pgw_s5_port = ue_ctx[imsi].pgw_s5_port;
								s5_cteid_ul = ue_ctx[imsi].s5_cteid_ul;
								s11_cteid_mme = ue_ctx[imsi].s11_cteid_mme;
								s11_cteid_sgw = ue_ctx[imsi].s11_cteid_sgw;
								s1_uteid_ul = ue_ctx[imsi].s1_uteid_ul;
								s5_uteid_dl = ue_ctx[imsi].s5_uteid_dl;	
								munlock(uectx_mux);

								TRACE(cout<<"In detach "<<imsi<<" s5 "<< s5_cteid_ul<< "s11 "<<s11_cteid_mme <<" rcv " << pkt.gtp_hdr.teid<<endl;)
								
								pkt.clear_pkt();
								pkt.append_item(eps_bearer_id);
								pkt.append_item(tai);
								pkt.prepend_gtp_hdr(2, 4, pkt.len, s5_cteid_ul);
								pkt.prepend_len();



								//send to pgw
								pgw_fd = fddata.initial_fd;
								returnval = mtcp_write(mctx, pgw_fd, pkt.data , pkt.len);
								if(returnval < 0)
								{
									cout<<"Error: pgw write detach "<<errno<<endl;
									exit(-1);
								}

								epevent.data.sockid = pgw_fd;
								epevent.events = MTCP_EPOLLIN;
								returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, pgw_fd, &epevent);
								if(returnval == -1)
								{
									cout<<"Error: pgw read after detach epoll"<<endl;
									exit(-1);
								}

								fdmap.erase(cur_fd);
								fddata.act = 4;
								fddata.tid = s11_cteid_mme;
								fddata.guti = imsi;
								fddata.buflen = 0;
								fddata.initial_fd = cur_fd;
								memset(fddata.buf, '\0', 500);
								fdmap.insert(make_pair(pgw_fd, fddata));
								//goto case 4

							}
							else
							{
								cout<<"SGW read from MME, wrong header"<<endl;
							}//sgw read from mme

						}//mtcp_epollin
						else
						{
							cout<<"Error: Wrong event in act case 1"<<" Core "<<core<<" ";
							if(revent.events & MTCP_EPOLLIN) cout<<"Epollin ";
							if(revent.events & MTCP_EPOLLOUT) cout<<"EpollOut ";
							cout<<endl;
							//exit(-1);
						}//case 1 wrong event
					break;//case 1

					case 2:
						//connected to pgw
						if(revent.events & MTCP_EPOLLOUT)
						{	
							retval = mtcp_write(mctx, cur_fd, fddata.buf, fddata.buflen);
							if(retval < 0)
							{
								cout<<"Error: pgw write data"<<endl;
								exit(-1);
							}
							epevent.data.sockid = cur_fd;
							epevent.events = MTCP_EPOLLIN;
							returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_MOD, cur_fd, &epevent);
							if(returnval < 0)
							{
								cout<<"Error pgw mod add epoll\n";
								exit(-1);
							}
							fdmap.erase(cur_fd);
							fddata.act = 3;
							fddata.buflen = 0;
							memset(fddata.buf, '\0', 500);
							fdmap.insert(make_pair(cur_fd, fddata));
						}
						else
						{
							cout<<"Wrong event at case 2\n";
							exit(-1);
						}

					break;//case 2

					case 3:
						if(revent.events & MTCP_EPOLLIN)
						{
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error pgw epoll reply delete \n";
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
							if(retval < 0)
							{
								cout<<"Error pgw read data "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}
							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;

							imsi = fddata.guti;

							pkt.extract_gtp_hdr();
							pkt.extract_item(s5_cteid_ul);
							pkt.extract_item(eps_bearer_id);
							pkt.extract_item(s5_uteid_ul);
							pkt.extract_item(ue_ip_addr);

							mlock(uectx_mux);
							ue_ctx[imsi].s5_uteid_ul = s5_uteid_ul;
							ue_ctx[imsi].s5_cteid_ul = s5_cteid_ul;
							munlock(uectx_mux);

							TRACE(cout<<"Attach 3 received from PGW "<< imsi <<" " << fddata.tid<< " rcv "<<s5_cteid_ul<<endl;)
							
							s1_uteid_ul = fddata.tid;
							s11_cteid_sgw = fddata.tid;
							
							pkt.clear_pkt();
							pkt.append_item(s11_cteid_sgw);
							pkt.append_item(ue_ip_addr);
							pkt.append_item(s1_uteid_ul);
							pkt.append_item(s5_uteid_ul);
							pkt.append_item(s5_uteid_dl);
							pkt.prepend_gtp_hdr(2, 1, pkt.len, s11_cteid_mme);
							pkt.prepend_len();
							
							mme_fd = fddata.initial_fd;
							returnval = mtcp_write(mctx, mme_fd, pkt.data, pkt.len);
							if(returnval < 0)
							{
								cout<<"Error: Cant send to mme A3"<<endl;
							
								exit(-1);
							}
							fdmap.erase(cur_fd);
							//mtcp_close(mctx, mme_fd);
							//mtcp_close(mctx, cur_fd);
							//con_processed++;
							//cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;

							fddata.act = 1;
							fddata.buflen = 0;
							fddata.initial_fd = cur_fd;
							memset(fddata.buf, '\0', 500);
							fddata.tid = 0;
							fddata.guti = 0;
							fdmap.insert(make_pair(mme_fd, fddata));

							epevent.events = MTCP_EPOLLIN;
							epevent.data.sockid = mme_fd;
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, mme_fd, &epevent);
							if(retval < 0)
							{
								TRACE(cout<<"Error adding mme fd for attach 3 to epoll"<<endl;)
								exit(-1);
							}
		
						}	
						else
						{
							cout<<"Wrong event at case 3\n";
							exit(-1);
						}
					break;//case 3

					case 4:
						//detach
						if(revent.events & MTCP_EPOLLIN)
						{
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error pgw epoll reply delete \n";
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
							if(retval < 0)
							{
								cout<<"Error pgw read data "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}
							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;

							pkt.extract_gtp_hdr();
							pkt.extract_item(res);
							if (res == false) {
								TRACE(cout << "sgw_handledetach:" << " pgw detach failure: " << imsi << endl;)
								exit(-1);
							}
							s11_cteid_mme = fddata.tid;
							TRACE(cout<<"Detach received "<<pkt.gtp_hdr.teid <<" s11"<<s11_cteid_mme<<endl;)
							
							pkt.clear_pkt();
							pkt.append_item(res);
							pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_mme);
							pkt.prepend_len();
		
							mlock(s11id_mux);
							if (s11_id.find(s11_cteid_mme) != s11_id.end()) {
									imsi = s11_id[s11_cteid_mme];
								}
							else
							{
								cout<<"Detach imsi not found"<<endl;
								exit(-1);
							}
							s11_id.erase(s11_cteid_mme);							
							munlock(s11id_mux);

							mlock(s1id_mux);
							s1_id.erase(s11_cteid_mme);
							munlock(s1id_mux);

							mlock(s5id_mux);
							s5_id.erase(s11_cteid_mme);
							munlock(s5id_mux);
							
							mlock(uectx_mux);
							ue_ctx.erase(imsi);
							munlock(uectx_mux);
							
							mme_fd = fddata.initial_fd;
							returnval = mtcp_write(mctx, mme_fd, pkt.data, pkt.len);
							if(returnval < 0)
							{
								cout<<"Error: Cant send to MME detach "<<errno<<" "<<returnval<<endl;
								exit(-1);
							}

							fdmap.erase(cur_fd);
							mtcp_close(mctx, cur_fd);
							mtcp_close(mctx, mme_fd);
							con_processed++;
							TRACE(cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;)

						}
						else
						{
							cout<<"Wrong event at case 4\n";
							exit(-1);
						}

					break;//case 4

					default:
						cout<<"Error unknown switch case"<<endl;
						cout<<"Act "<<act_type<<" fd "<<cur_fd<<endl;
						if(cur_fd == sgw_fd)
							cout<<"SGW ";
						if(cur_fd == listen_fd)
							cout<<"Ran Linten";
						break;//default
				}//else switch case
			}//else other than listen fd events
		}//for numevents
	}//while
}//run

int main()
{

	char* conf_file = "server.conf";
	int ret;
 
     /* initialize mtcp */
	if(conf_file == NULL)
	{
		cout<<"Forgot to pass the mTCP startup config file!\n";
		exit(EXIT_FAILURE);
	}
	else
	{
		TRACE_INFO("Reading configuration from %s\n",conf_file);
	}

	//step 1. mtcp_init, mtcp_register_signal(optional)
	ret = mtcp_init(conf_file);
	if (ret) {
		cout<<"Failed to initialize mtcp\n";
		exit(EXIT_FAILURE);
	}
	
	/* register signal handler to mtcp */
	mtcp_register_signal(SIGINT, SignalHandler);

	//Initialize state here...
	s11_id.clear();
	s1_id.clear();
	s5_id.clear();
	ue_ctx.clear();

	//Initialize locks here...
	mux_init(s11id_mux);	
	mux_init(s1id_mux);	
	mux_init(s5id_mux);	
	mux_init(uectx_mux);	


	//spawn server threads
	for(int i=0;i<MAX_THREADS;i++){
		arguments[i].coreno = i;
		arguments[i].id = i;
		pthread_create(&servers[i],NULL,run,&arguments[i]);
	}


	//run();
	//Wait for server threads to complete
	for(int i=0;i<MAX_THREADS;i++){
		pthread_join(servers[i],NULL);		
	}

	//run();
	return 0;
}
