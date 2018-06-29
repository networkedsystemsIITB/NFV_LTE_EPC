#include "mkb_pgw.h"

#define MAX_THREADS 1

struct arg arguments[MAX_THREADS]; // Arguments sent to Pthreads
pthread_t servers[MAX_THREADS]; // Threads 

void SignalHandler(int signum)
{
	//Handle ctrl+C here
	done = 1;
	mtcp_destroy();
	
}

//kanase mtcp edits...
//static in_addr_t daddr;
//static in_port_t dport;
//static in_addr_t saddr;
//kanase mtcp done

//State 
unordered_map<uint32_t, uint64_t> s5_id; /* S5 UE identification table: s5_cteid_ul -> imsi */
unordered_map<string, uint64_t> sgi_id; /* SGI UE identification table: ue_ip_addr -> imsi */
unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: imsi -> UeContext */
/* IP addresses table - Write once, Read always table - No need to put mlock */ 
unordered_map<uint64_t, string> ip_addrs;

//Not needed for single core
pthread_mutex_t s5id_mux; /* Handles s5_id */
pthread_mutex_t sgiid_mux; /* Handles sgi_id */
pthread_mutex_t uectx_mux; /* Handles ue_ctx */

void *run(void* args)
{
	/*
		MTCP Setup
	*/

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
	//not needed in servers
	//daddr = inet_addr(HSSADDR);
	//dport = htons(HSSPORTNO);
	//saddr = INADDR_ANY;	
	//mtcp_init_rss(mctx, saddr, IP_RANGE, daddr, dport);
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

	//pgw specific
	uint8_t eps_bearer_id;
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint32_t s5_cteid_ul;
	uint32_t s5_cteid_dl;
	uint64_t apn_in_use;
	uint64_t tai;
	string ue_ip_addr;
	uint64_t imsi;
	bool res;

	/*
		Server side initialization
	*/
	int listen_fd, pgw_fd, sgw_fd;
	struct sockaddr_in pgw_server_addr;

	listen_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if(listen_fd < 0)
	{
		TRACE(cout<<"Error: PGW listen socket call"<<endl;)
		exit(-1);
	}
	
	retval = mtcp_setsock_nonblock(mctx, listen_fd);
	if(retval < 0)
	{
		TRACE(cout<<"Error: mtcp make nonblock"<<endl;)
		exit(-1);
	}

	bzero((char *) &pgw_server_addr, sizeof(pgw_server_addr));
	pgw_server_addr.sin_family = AF_INET;
	pgw_server_addr.sin_addr.s_addr = inet_addr(pgw_ip);
	pgw_server_addr.sin_port = htons(pgw_s5_port);

	retval = mtcp_bind(mctx, listen_fd, (struct sockaddr *) &pgw_server_addr, sizeof(pgw_server_addr));
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
		TRACE(cout<<"Error: mtcp epoll_ctl_add pgw"<<endl;)
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
					cout<<"Error: In MME Listen fd"<<endl;
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
				//SGW ACCEPTS
				while(1)
				{
					sgw_fd = mtcp_accept(mctx, listen_fd, NULL, NULL);
					if(sgw_fd < 0)
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
					epevent.data.sockid = sgw_fd;
					mtcp_setsock_nonblock(mctx, sgw_fd);
					retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, sgw_fd, &epevent);
					if(retval < 0)
					{
						cout<<"SGW accept epoll add error "<<errno<<" retval "<<retval<<" core "<<core<<endl;
						exit(-1);
					}
					fddata.act = 1;
					fddata.initial_fd = -1;
					memset(fddata.buf,'\0',500);
					fddata.buflen = 0;
					fdmap.insert(make_pair(sgw_fd, fddata));
					con_accepts++;
				}//while accepts
				TRACE(cout<<" Core "<<core<<" accepted "<<con_accepts<<" till now "<<endl;)
				//go to act_type case 1
			}//if listen fd
			else
			{	
				//Check for action type
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
								cout<<"Error sgw epoll read delete from epoll"<<endl;
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);							
							if(retval == 0)
							{
								cout<<"Connection closed by SGW, core "<<core<<endl;
								
								mtcp_close(mctx, cur_fd);
								fdmap.erase(cur_fd);
								continue;
							}
							else
							if(retval < 0)
							{
								cout<<"Error: SGW read data case 1 "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}

							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;

							pkt.extract_gtp_hdr();	

							if(pkt.gtp_hdr.msg_type == 1)
							{
								//attach 3 request from sgw
								pkt.extract_item(s5_cteid_dl);
								pkt.extract_item(imsi);
								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(s5_uteid_dl);
								pkt.extract_item(apn_in_use);
								pkt.extract_item(tai);

								s5_cteid_ul = s5_cteid_dl;
								s5_uteid_ul = s5_cteid_dl;

								ue_ip_addr = ip_addrs[imsi];//locks not needed, read only

								TRACE(cout<<"3rd Attach imsi"<<imsi<<" key "<<s5_cteid_dl<<endl;)

								mlock(s5id_mux);
								s5_id[s5_uteid_ul] = imsi;
								munlock(s5id_mux);

								mlock(sgiid_mux);
								sgi_id[ue_ip_addr] = imsi;
								munlock(sgiid_mux);

								mlock(uectx_mux);
								ue_ctx[imsi].init(ue_ip_addr, tai, apn_in_use, eps_bearer_id, s5_uteid_ul, s5_uteid_dl, s5_cteid_ul, s5_cteid_dl);
								munlock(uectx_mux);

								pkt.clear_pkt();
								pkt.append_item(s5_cteid_ul);
								pkt.append_item(eps_bearer_id);
								pkt.append_item(s5_uteid_ul);
								pkt.append_item(ue_ip_addr);
								pkt.prepend_gtp_hdr(2, 1, pkt.len, s5_cteid_dl);
								pkt.prepend_len();

								retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
								if(retval < 0)
								{
									cout<<"Error MME write back to RAN A2"<<endl;
									exit(-1);
								}

								//fdmap.erase(cur_fd);
								//mtcp_close(mctx, cur_fd);
								
								epevent.events = MTCP_EPOLLIN;
								epevent.data.sockid = cur_fd;
								retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, cur_fd, &epevent);
								if(retval < 0)
								{
									TRACE(cout<<"Error adding sgw fd after a3 to epoll"<<endl;)
									exit(-1);
								}

							}
							else
							if(pkt.gtp_hdr.msg_type == 4)
							{

								TRACE(cout<<"In detach "<<pkt.gtp_hdr.teid<<endl;)
								res = true;
								
								mlock(s5id_mux);
								if (s5_id.find(pkt.gtp_hdr.teid) != s5_id.end()) {
									imsi = s5_id[pkt.gtp_hdr.teid];
								}
								else
								{
									cout<<"Error: imsi not found in detach "<<pkt.gtp_hdr.teid<<endl;
									exit(-1);
								}
								munlock(s5id_mux);

								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(tai);
								
								if(gettid(imsi) != pkt.gtp_hdr.teid)
								{
									cout<<"GUTI not equal Detach"<<imsi<<" "<<pkt.gtp_hdr.teid<<endl;
									exit(-1);
								}

								mlock(uectx_mux);
								s5_cteid_ul = ue_ctx[imsi].s5_cteid_ul;
								s5_cteid_dl = ue_ctx[imsi].s5_cteid_dl;
								ue_ip_addr = ue_ctx[imsi].ip_addr;
								ue_ctx.erase(imsi);
								munlock(uectx_mux);
								
								mlock(sgiid_mux);
								sgi_id.erase(ue_ip_addr);
								munlock(sgiid_mux);

								mlock(s5id_mux);
								s5_id.erase(s5_cteid_ul);
								munlock(s5id_mux);

								pkt.clear_pkt();
								pkt.append_item(res);
								pkt.prepend_gtp_hdr(2, 4, pkt.len, s5_cteid_dl);
								pkt.prepend_len();

								retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
								if(retval < 0)
								{
									cout<<"Error PGW write back to SGW detach"<<endl;
									exit(-1);
								}
								

								fdmap.erase(cur_fd);
								mtcp_close(mctx, cur_fd);
								con_processed++;
								TRACE(cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;)
								

							}
							else
							{
								cout<<"PGW read from SGW, wrong header"<<endl;
							}

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

					default:

					break;//default
				}//switch
			}//else action type events
		}//for numevents	
	}//while !done

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
	s5_id.clear();
	sgi_id.clear();
	ue_ctx.clear();
	ip_addrs.clear();
	//set_ip_addrs();
	uint64_t imsi;
	int i;
	int subnet;
	int host;
	string prefix;
	string ip_addr;
	prefix = "172.16.";
	subnet = 1;
	host = 3;
	for (i = 0; i < MAX_UE_COUNT; i++) {
		imsi = 119000000000 + i;
		ip_addr = prefix + to_string(subnet) + "." + to_string(host);
		ip_addrs[imsi] = ip_addr;
		if (host == 254) {
			subnet++;
			host = 3;
		}
		else {
			host++;
		}
	}

	//Initialize locks here...
	mux_init(s5id_mux);	
	mux_init(sgiid_mux);	
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
