#include "kby_pgw.h"

void run()
{

	/*
		MTCP Setup
	*/
		char* conf_file = "server.conf";
	    	/* initialize mtcp */
		int core = 0;
		int retval;
		if (conf_file == NULL) {
			TRACE("Forgot to pass the mTCP startup config file!\n";)
			exit(EXIT_FAILURE);
		}

		//step 1. mtcp_init, mtcp_register_signal(optional)
		retval = mtcp_init(conf_file);
		if (retval) {
			TRACE("Failed to initialize mtcp\n";)
			exit(EXIT_FAILURE);
		}
		
		TRACE("Application initialization finished.\n";)
		
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


	/*
		PGW Specific data
	*/
	unordered_map<uint32_t, uint64_t> s5_id; /* S5 UE identification table: s5_cteid_ul -> imsi */
	unordered_map<string, uint64_t> sgi_id; /* SGI UE identification table: ue_ip_addr -> imsi */
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: imsi -> UeContext */

	/* IP addresses table - Write once, Read always table - No need to put mlock */ 
	unordered_map<uint64_t, string> ip_addrs;


	//set ip address
	uint64_t imsi;
	int subnet;
	int host;
	string prefix;
	string ip_addr;

	prefix = "172.16.";
	subnet = 1;
	host = 3;
	for (int i = 0; i < 2000; i++) {	//MAX_UE_COUNT
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


	uint8_t eps_bearer_id;
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint32_t s5_cteid_ul;
	uint32_t s5_cteid_dl;
	uint64_t apn_in_use;
	uint64_t tai;
	string ue_ip_addr;
	bool res;

	//program specific
	int i,returnval,cur_fd, act_type;
	struct mdata fddata;
	Packet pkt;
	int pkt_len;

	/*
		Server side initialization
	*/
	int listen_fd,pgw_fd, sgw_fd;
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


	//	if(bind(ran_listen_fd, (struct sockaddr *) &mme_server_addr, sizeof(mme_server_addr)) < 0)
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
	//pgw listen setup done

	/*
		Epoll Setup
	*/
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

	/*
	MAIN EVENT LOOP
	*/
	while(1)
	{
		//watch for events
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
			if( (return_events[i].events & EPOLLERR) ||
				(return_events[i].events & EPOLLHUP) )
			{

				TRACE(cout<<"\n\nError: epoll event returned : "<<return_events[i].data.sockid<<" errno :"<<errno<<endl;)
				if(return_events[i].data.sockid == listen_fd)
				{
					TRACE(cout<<"Error: Its Listen fd"<<endl;)
				}
				close(return_events[i].data.sockid);
				continue;
			}

			revent = return_events[i];
			/*
				Check type of event
			*/
			if(revent.data.sockid == listen_fd) 
			{
				handle_pgw_accept(listen_fd);
			}//	accept new connections
			else
			{
				cur_fd = revent.data.sockid;
				fddata = fdmap[cur_fd];
				act_type = fddata.act;

				//Check action type
				//For detailed flow, check kby_pgw.h FDMAP comment line 40
				switch(act_type)
				{
					case 1:

						retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, &epevent);
						if(retval < 0)
						{
							cout<<"Error pgw epoll read delete from epoll"<<endl;
							exit(-1);
						}

						pkt.clear_pkt();
						retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
						if(retval < 0)
						{
							TRACE(cout<<"ERROR: read packet from SGW"<<endl;)
							exit(-1);
						}
						
						memcpy(&pkt_len, data, sizeof(int));
						dataptr = data+sizeof(int);
						memcpy(pkt.data, (dataptr), pkt_len);
						pkt.data_ptr = 0;
						pkt.len = pkt_len;

						pkt.extract_gtp_hdr();	

						if(pkt.gtp_hdr.msg_type == 1)
						{//attach 3 request from sgw
							
							pkt.extract_item(s5_cteid_dl);
							pkt.extract_item(imsi);
							pkt.extract_item(eps_bearer_id);
							pkt.extract_item(s5_uteid_dl);
							pkt.extract_item(apn_in_use);
							pkt.extract_item(tai);
							s5_cteid_ul = s5_cteid_dl;
							s5_uteid_ul = s5_cteid_dl;
							ue_ip_addr = ip_addrs[imsi];
							TRACE(cout<<"3rd Attach imsi"<<imsi<<" key "<<s5_cteid_dl<<endl;)
							s5_id[s5_uteid_ul] = imsi;
							sgi_id[ue_ip_addr] = imsi;
							ue_ctx[imsi].init(ue_ip_addr, tai, apn_in_use, eps_bearer_id, s5_uteid_ul, s5_uteid_dl, s5_cteid_ul, s5_cteid_dl);

							pkt.clear_pkt();
							pkt.append_item(s5_cteid_ul);
							pkt.append_item(eps_bearer_id);
							pkt.append_item(s5_uteid_ul);
							pkt.append_item(ue_ip_addr);
							pkt.prepend_gtp_hdr(2, 1, pkt.len, s5_cteid_dl);
							pkt.prepend_len();

							pgw_send_a3(cur_fd, pkt);
							//reply sent to sgw
							//Dataplace specific, only attach part
						}//attach 3
						else
						if(pkt.gtp_hdr.msg_type == 4)
						{	
							//detach request from sgw
							TRACE(cout<<"In detach "<<pkt.gtp_hdr.teid<<endl;)
							res = true;
							
							if (s5_id.find(pkt.gtp_hdr.teid) != s5_id.end()) {
								imsi = s5_id[pkt.gtp_hdr.teid];
							}
							else
							{
								cout<<"Error: imsi not found in detach "<<pkt.gtp_hdr.teid<<endl;
								exit(-1);
							}
							pkt.extract_item(eps_bearer_id);
							pkt.extract_item(tai);
							
							if(gettid(imsi) != pkt.gtp_hdr.teid)
							{
								cout<<"GUTI not equal Detach"<<imsi<<" "<<pkt.gtp_hdr.teid<<endl;
								exit(-1);
							}
							
							s5_cteid_ul = ue_ctx[imsi].s5_cteid_ul;
							s5_cteid_dl = ue_ctx[imsi].s5_cteid_dl;
							ue_ip_addr = ue_ctx[imsi].ip_addr;

							pkt.clear_pkt();
							pkt.append_item(res);
							pkt.prepend_gtp_hdr(2, 4, pkt.len, s5_cteid_dl);
							pkt.prepend_len();

							s5_id.erase(s5_cteid_ul);
							sgi_id.erase(ue_ip_addr);
							ue_ctx.erase(imsi);

							retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
							if(retval < 0)
							{
								cout<<"Error PGW write back to SGW detach"<<endl;
								exit(-1);
							}
							

							fdmap.erase(cur_fd);
							mtcp_close(mctx, cur_fd);
							//PGW processing for current UE ends,
							//closed connection to sgw, destroyed states.
							//Epoll waiting for new connection on listen socket

						}
						else
						{
							cout<<"PGW read from SGW, wrong header"<<endl;
						}
					break;//case 1

				}//switch close
			}//else close
		}//for close
	}//while close
}


int main()
{
//Initialize locks, used for multicore
	mux_init(s5id_mux);	
	mux_init(sgiid_mux);	
	mux_init(uectx_mux);	

	run();
	return 0;
}


void handle_pgw_accept(int pgw_listen_fd)
{
	int pgw_accept_fd, retval;
	struct mdata fddata;
	TRACE(cout<<"In attach"<<endl;)
	while(1)
	{

		pgw_accept_fd = mtcp_accept(mctx, pgw_listen_fd, NULL, NULL);
		if(pgw_accept_fd < 0)
			{
			if((errno == EAGAIN) ||	(errno == EWOULDBLOCK))
			{
				break;
			}
			else
			{
				cout<<"Error on accept"<<endl;
				exit(-1);
			}
		}

		epevent.events = MTCP_EPOLLIN;
		epevent.data.sockid = pgw_accept_fd;
		mtcp_setsock_nonblock(mctx, pgw_accept_fd);
		retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, pgw_accept_fd, &epevent);
		if(retval < 0)
		{
			cout<<"Error adding sgw accept to epoll"<<endl;
			exit(-1);
		}			
		fddata.act = 1;
		fddata.initial_fd = 0;
		fddata.idfr = 0;
		memset(fddata.buf,'\0',500);
		fddata.buflen = 0;
		fdmap.insert(make_pair(pgw_accept_fd, fddata));
	}
}

void pgw_send_a3(int cur_fd, Packet pkt)
{
	int retval;
	struct mdata fddata;
	retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
	if(retval < 0)
	{
		cout<<"Error PGW write back to SGW"<<endl;
		exit(-1);
	}
	TRACE(cout<<"Sent 3rd attach response to sgw"<<endl;)

	fdmap.erase(cur_fd);
	mtcp_close(mctx, cur_fd);
}
