#include "kby_sgw.h"


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
		SGW Specific data
	*/
	unordered_map<uint32_t, uint64_t> s11_id; /* S11 UE identification table: s11_cteid_sgw -> imsi */
	unordered_map<uint32_t, uint64_t> s1_id; /* S1 UE identification table: s1_uteid_ul -> imsi */
	unordered_map<uint32_t, uint64_t> s5_id; /* S5 UE identification table: s5_uteid_dl -> imsi */
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: imsi -> UeContext */
	s11_id.clear();
	s1_id.clear();
	s5_id.clear();
	ue_ctx.clear();

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


	//program specific
	int i,returnval,cur_fd, act_type;
	struct mdata fddata;
	Packet pkt;
	int pkt_len;

	/*
		Server side initialization
	*/
	int listen_fd, mme_fd,pgw_fd, sgw_fd;
	struct sockaddr_in sgw_server_addr;

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
					TRACE(cout<<"Error: Its MME Listen fd"<<endl;)
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
				handle_sgw_accept(listen_fd);
			}//			
			else
			{
				cur_fd = revent.data.sockid;
				fddata = fdmap[cur_fd];
				act_type = fddata.act;

				//Check action type
				//For detailed flow, check kby_sgw.h FDMAP comment line 41
				switch(act_type)
				{
					case 1:
						//request from mme
						retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, &epevent);
						if(retval < 0)
						{
							cout<<"Error mme epoll read delete from epoll"<<endl;
							exit(-1);
						}

						pkt.clear_pkt();
						retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
						if(retval < 0)
						{
							TRACE(cout<<"ERROR: read packet from MME"<<endl;)
							exit(-1);
						}
						
						memcpy(&pkt_len, data, sizeof(int));
						dataptr = data+sizeof(int);
						memcpy(pkt.data, (dataptr), pkt_len);
						pkt.data_ptr = 0;
						pkt.len = pkt_len;
						//Extract headers to find type of request from MME
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

							s11_id[s11_cteid_sgw] = imsi;
							s1_id[s1_uteid_ul] = imsi;
							s5_id[s5_uteid_dl] = imsi;
							TRACE(cout<<"Attach 3 in"<<imsi<< " "<<s11_cteid_mme<<endl;)
							ue_ctx[imsi].init(tai, apn_in_use, eps_bearer_id, s1_uteid_ul, s5_uteid_dl, s11_cteid_mme, s11_cteid_sgw, s5_cteid_dl, pgw_s5_ip_addr, pgw_s5_port);
							ue_ctx[imsi].tai = tai;
							
							pkt.clear_pkt();
							pkt.append_item(s5_cteid_dl);
							pkt.append_item(imsi);
							pkt.append_item(eps_bearer_id);
							pkt.append_item(s5_uteid_dl);
							pkt.append_item(apn_in_use);
							pkt.append_item(tai);
							pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);
							pkt.prepend_len();

							fddata.tid = s11_cteid_mme;
							fddata.guti = imsi;
							handle_pgw_connect(cur_fd, pkt, fddata, pgw_s5_ip_addr, pgw_s5_port);
							//go to act_type case 2
						}//attach 3
						else
						if(pkt.gtp_hdr.msg_type == 2)
						{//attach 4 from mme
							if (s11_id.find(pkt.gtp_hdr.teid) != s11_id.end()) {
								imsi = s11_id[pkt.gtp_hdr.teid];
							}
							else
							{
								cout<<"IMSI error"<<endl;
								exit(-1);
							}
							
							if (imsi == 0) {
								TRACE(cout << "sgw_handlemodifybearer:" << " :zero imsi " << pkt.gtp_hdr.teid << " " << pkt.len << ": " << imsi << endl;)
								exit(-1);
							}
								
								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(s1_uteid_dl);
								pkt.extract_item(enodeb_ip_addr);
								pkt.extract_item(enodeb_port);	
								
								ue_ctx[imsi].s1_uteid_dl = s1_uteid_dl;
								ue_ctx[imsi].enodeb_ip_addr = enodeb_ip_addr;
								ue_ctx[imsi].enodeb_port = enodeb_port;
								s11_cteid_mme = ue_ctx[imsi].s11_cteid_mme;
								TRACE(cout<<"In attach 4 "<< imsi<< " "<<s11_cteid_mme << "recv "<< s1_uteid_dl<<endl;)
								res = true;
								pkt.clear_pkt();
								pkt.append_item(res);
								pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_mme);
								pkt.prepend_len();

								send_attach_four(cur_fd, pkt);
								//go to act_type case 1, next header = 3
						}//attach 4
						else
						if(pkt.gtp_hdr.msg_type == 3)
						{
							//detach request from mme
							
							if (s11_id.find(pkt.gtp_hdr.teid) != s11_id.end()) {
								imsi = s11_id[pkt.gtp_hdr.teid];
							}
							else
							{
								cout<<"IMSI error"<<endl;
								exit(-1);
							}
							pkt.extract_item(eps_bearer_id);
							pkt.extract_item(tai);
							
							if (ue_ctx.find(imsi) == ue_ctx.end()) {
								TRACE(cout << "sgw_handledetach:" << " no uectx: " << imsi << endl;)
								exit(-1);
							}
							if(gettid(imsi) != pkt.gtp_hdr.teid)
							{
								cout<<"GUTI not equal Detach acc"<<imsi<<" "<<pkt.gtp_hdr.teid<<endl;
								exit(-1);
							}	
							pgw_s5_ip_addr = ue_ctx[imsi].pgw_s5_ip_addr;
							pgw_s5_port = ue_ctx[imsi].pgw_s5_port;
							s5_cteid_ul = ue_ctx[imsi].s5_cteid_ul;
							s11_cteid_mme = ue_ctx[imsi].s11_cteid_mme;
							s11_cteid_sgw = ue_ctx[imsi].s11_cteid_sgw;
							s1_uteid_ul = ue_ctx[imsi].s1_uteid_ul;
							s5_uteid_dl = ue_ctx[imsi].s5_uteid_dl;	
							TRACE(cout<<"In detach "<<imsi<<" s5 "<< s5_cteid_ul<< "s11 "<<s11_cteid_mme <<" rcv " << pkt.gtp_hdr.teid<<endl;)
							
							pkt.clear_pkt();
							pkt.append_item(eps_bearer_id);
							pkt.append_item(tai);
							pkt.prepend_gtp_hdr(2, 4, pkt.len, s5_cteid_ul);
							pkt.prepend_len();
							fddata.tid = s11_cteid_mme;
							fddata.guti = imsi;
							send_pgw_detach(cur_fd, fddata, pkt);
							//go to act_type case 4
						}//detach 
						else
						{
							cout<<"SGW read from MME, wrong header"<<endl;
						}//sgw read from mme close

					break; // case 1;

					case 2:
						//Connect to pgw done

						if(return_events[i].events & MTCP_EPOLLOUT)
						{
							send_request_pgw(cur_fd, fddata);
							//go to act_type case 3
						}
						else
						{
							cout<<"Error: Case 2 pgw connect Wrong Event";
							exit(-1);
						}

					break; // case 2;

					case 3:
						//Reply from pgw
						retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, &epevent);
						if(retval < 0)
						{
							cout<<"Error pgw epoll read delete from epoll"<<endl;
							exit(-1);
						}

						pkt.clear_pkt();
						retval = mtcp_read(mctx, cur_fd, data,BUF_SIZE);
						if(retval <= 0)
						{
							cout<<"Error: pgw read packet"<<errno<<endl;
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

						
						ue_ctx[imsi].s5_uteid_ul = s5_uteid_ul;
						ue_ctx[imsi].s5_cteid_ul = s5_cteid_ul;

						TRACE(cout<<"Attach 3 received from PGW "<<imsi << fddata.tid<< " rcv "<<s5_cteid_ul<<endl;)
						
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
						epevent.data.sockid = mme_fd;
						epevent.events = MTCP_EPOLLIN;
						returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, mme_fd, &epevent);
						if(returnval == -1)
						{
							cout<<"Error: ran after detach add mtcp error"<<endl;
							exit(-1);
						}	

						fddata.act = 1;
						fddata.initial_fd = cur_fd; 
						memset(fddata.buf,0,500);
						fddata.tid = 0;
						fddata.guti = 0;
						fddata.buflen = 0;
						fdmap.insert(make_pair(mme_fd,fddata));
						//go to act_type case 1, next header = 2

					break;//case 3

					case 4:
						//detach rcv form pgw
						retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, &epevent);
						if(retval < 0)
						{
							cout<<"Error pgw epoll read delete from epoll"<<endl;
							exit(-1);
						}

						pkt.clear_pkt();
						retval = mtcp_read(mctx, cur_fd, data,BUF_SIZE);
						if(retval <= 0)
						{
							cout<<"Error: pgw read packet"<<errno<<endl;
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
						TRACE(cout<<"Detach rcv "<<pkt.gtp_hdr.teid <<" s11"<<s11_cteid_mme<<endl;)
						
						pkt.clear_pkt();
						pkt.append_item(res);
						pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_mme);
						pkt.prepend_len();
	
						if (s11_id.find(s11_cteid_mme) != s11_id.end()) {
								imsi = s11_id[s11_cteid_mme];
							}
						else
						{
							cout<<"Detach imsi not found"<<endl;
							exit(-1);
						}
						s11_id.erase(s11_cteid_mme);
						s1_id.erase(s11_cteid_mme);
						s5_id.erase(s11_cteid_mme);
						ue_ctx.erase(imsi);
						
						mme_fd = fddata.initial_fd;

						//close pgw connection and context
						fdmap.erase(cur_fd);
						mtcp_close(mctx, cur_fd);

						returnval = mtcp_write(mctx, mme_fd, pkt.data, pkt.len);
						if(returnval < 0)
						{
							cout<<"Error: Cant send to MME detach"<<endl;
							exit(-1);
						}

						fdmap.erase(mme_fd);
						mtcp_close(mctx, mme_fd);
						//SGW processing for current UE ends,
						//closed all connections, destroyed states.
						//Epoll waiting for new connection on listen socket
					
				}//close switch
			}//end other events;
		}//end for i events
	}//close while
}//end run()



int main()
{
//Initialize locks, used for multicore
	mux_init(s11id_mux);	
	mux_init(s1id_mux);	
	mux_init(s5id_mux);	
	mux_init(uectx_mux);	

	run();
	return 0;
}


void handle_sgw_accept(int sgw_listen_fd)
{
	int sgw_accept_fd, retval;
	struct mdata fddata;
	TRACE(cout<<"In attach"<<endl;)
	while(1)
	{

		sgw_accept_fd = mtcp_accept(mctx, sgw_listen_fd, NULL, NULL);
		if(sgw_accept_fd < 0)
			{
			if((errno == EAGAIN) ||	(errno == EWOULDBLOCK))
			{
				break;
			}
			else
			{
				perror("mtcp error : error on accetpt ");
				cout<<"Error on accept"<<endl;
				exit(-1);
				//break;
			}
		}
		
		epevent.events = MTCP_EPOLLIN;
		epevent.data.sockid = sgw_accept_fd;
		mtcp_setsock_nonblock(mctx, sgw_accept_fd);
		retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, sgw_accept_fd, &epevent);
		if(retval < 0)
		{
			cout<<"Error adding ran accept to epoll"<<endl;
			exit(-1);
		}			
		fddata.act = 1;
		fddata.initial_fd = 0;
		fddata.tid = 0;
		memset(fddata.buf,'\0',500);
		fddata.buflen = 0;
		fdmap.insert(make_pair(sgw_accept_fd, fddata));
		}
}

void handle_pgw_connect(int cur_fd,Packet pkt,struct mdata fddata, string pgw_s5_ip_addr,int pgw_s5_port)
{
	int pgw_fd,retval, returnval;
	struct sockaddr_in pgw_server_addr;

	bzero((char *) &pgw_server_addr, sizeof(pgw_server_addr));
	pgw_server_addr.sin_family = AF_INET;
	pgw_server_addr.sin_addr.s_addr = inet_addr(pgw_s5_ip_addr.c_str());
	pgw_server_addr.sin_port = htons(pgw_s5_port);

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
	
	retval = mtcp_connect(mctx, pgw_fd, (struct sockaddr *)&pgw_server_addr, sizeof(struct sockaddr_in));
	
	if((retval < 0) && (errno == EINPROGRESS))
	{
		epevent.events =  MTCP_EPOLLOUT;
		epevent.data.sockid = pgw_fd;
		returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, pgw_fd, &epevent);
		if(returnval == -1)
		{
			cout<<"Error: epoll pgw add"<<endl;
			exit(-1);
		}
		fdmap.erase(cur_fd);
		fddata.act = 2;
		fddata.initial_fd = cur_fd;
		memcpy(fddata.buf, pkt.data, pkt.len);
		fddata.buflen = pkt.len;
		fdmap.insert(make_pair(pgw_fd, fddata));
	}
	else
	if(retval < 0)
	{
		cout<<"Error: mtcp pgw connect error"<<endl;
		exit(-1);
	}
	else
	{
		cout<<"PGW connection immedietly\n";
		exit(-1);
	}
}

void send_request_pgw(int cur_fd, struct mdata fddata)
{
	int returnval;
	returnval = mtcp_write(mctx, cur_fd, fddata.buf, fddata.buflen);
	if(returnval < 0)
	{
		cout<<"Error: pgw write after connect"<<errno<<endl;
		exit(-1);
	}
	epevent.data.sockid = cur_fd;
	epevent.events = MTCP_EPOLLIN;
	returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_MOD, cur_fd, &epevent);
	if(returnval == -1)
	{
		cout<<"Error: pgw listen add mtcp error"<<endl;
		exit(-1);
	}
	fdmap.erase(cur_fd);
	fddata.act = 3;
	fddata.buflen = 0;
	memset(fddata.buf, '\0', 500);
	fdmap.insert(make_pair(cur_fd, fddata));
}

void send_attach_four(int cur_fd, Packet pkt)
{
	int retval;
	retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
	if(retval < 0)
	{
		cout<<"Error MME write back to RAN"<<endl;
		exit(-1);
	}

	epevent.events = MTCP_EPOLLIN;
	epevent.data.sockid = cur_fd;
	retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, cur_fd, &epevent);
	if(retval < 0)
	{
		TRACE(cout<<"Error adding ran for attach 2 to epoll"<<endl;)
		exit(-1);
	}
	TRACE(cout<<"Attach 4 sent to mme"<<endl;)
}

void send_pgw_detach(int sgw_fd, struct mdata fddata, Packet pkt)
{
	int pgw_fd, returnval;
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
	fdmap.erase(sgw_fd);
	fddata.act = 4;
	fddata.buflen = 0;
	fddata.initial_fd = sgw_fd;
	memset(fddata.buf, '\0', 500);
	fdmap.insert(make_pair(pgw_fd, fddata));
}