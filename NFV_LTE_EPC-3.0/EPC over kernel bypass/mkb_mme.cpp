#include "mkb_mme.h"

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
uint64_t ue_count;							/* For locks couple with uectx */
unordered_map<uint32_t, uint64_t> s1mme_id; /* S1_MME UE identification table: mme_s1ap_ue_id -> guti */
unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */

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
	//unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */
	//ue_ctx.clear();
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
	//for hss ie 1st server
	daddr = inet_addr(hss_ip);
	dport = htons(hss_my_port);
	saddr = INADDR_ANY;	
	mtcp_init_rss(mctx, saddr, IP_RANGE, daddr, dport);

	//for sgw ie 2nd server
	daddr = inet_addr(sgw_ip);
	dport = htons(sgw_s11_portnum);
	saddr = inet_addr(mme_ip);	

	/* Create Address Pool */
    addr_pool_t ap;
	ap = CreateAddressPoolPerCore(core, MAX_THREADS, saddr, IP_RANGE, daddr, dport); // SADDR BASE
	if (!ap) {
		errno = ENOMEM;
		cout<<"OUT OF MEMEORY WHILE CREATING PER CORE ADDRESS POOL";
	}

	/* MAP For storing connection id to saddr*/
	map<int,sockaddr_in> srcaddrmap;
	//edits done

	int retval;
	map<int, mdata> fdmap;
	int i,returnval,cur_fd, act_type;
	struct mdata fddata;
	Packet pkt;
	int pkt_len;
	char * dataptr;
	unsigned char data[BUF_SIZE];
	/*
		MME Specific data
	*/
	MmeIds mme_ids;
	uint64_t imsi;
	uint64_t tai;
	uint64_t ksi_asme;
	uint16_t nw_type;
	uint16_t nw_capability;
	uint64_t autn_num;
	uint64_t rand_num;
	uint64_t xres;
	uint64_t res;
	uint64_t k_asme;
	uint32_t enodeb_s1ap_ue_id;
	uint32_t mme_s1ap_ue_id;
	uint64_t guti;
	uint64_t num_autn_vectors;
	uint64_t nas_enc_algo;
	uint64_t nas_int_algo;
	uint64_t k_nas_enc;
	uint64_t k_nas_int;
	vector<uint64_t> tai_list;
	uint64_t apn_in_use;
	uint64_t k_enodeb;
	uint64_t tau_timer;
	uint32_t s11_cteid_mme;
	uint32_t s11_cteid_sgw;
	uint32_t s1_uteid_ul;
	uint32_t s1_uteid_dl;
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint64_t detach_type;
	uint8_t eps_bearer_id;
	uint8_t e_rab_id;
	string pgw_s5_ip_addr;
	string ue_ip_addr;
	int tai_list_size;
	int pgw_s5_port;
	string tem;
	num_autn_vectors = 1;
	bool epsres;
	uint64_t bkp_imsi;


	/*
		Server side initialization
	*/
	int ran_listen_fd, ran_fd;
	struct sockaddr_in mme_server_addr, hss_server_addr;
	int sgw_fd,hss_fd;


	ran_listen_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if(ran_listen_fd < 0)
	{
		TRACE(cout<<"Error: RAN socket call"<<endl;)
		exit(-1);
	}
	
	retval = mtcp_setsock_nonblock(mctx, ran_listen_fd);
	if(retval < 0)
	{
		TRACE(cout<<"Error: mtcp make nonblock"<<endl;)
		exit(-1);
	}

	bzero((char *) &mme_server_addr, sizeof(mme_server_addr));
	mme_server_addr.sin_family = AF_INET;
	mme_server_addr.sin_addr.s_addr = inet_addr(mme_ip);
	mme_server_addr.sin_port = htons(mme_my_port);

	bzero((char *) &hss_server_addr, sizeof(hss_server_addr));
	hss_server_addr.sin_family = AF_INET;
	hss_server_addr.sin_addr.s_addr = inet_addr(hss_ip);
	hss_server_addr.sin_port = htons(hss_my_port);




	retval = mtcp_bind(mctx, ran_listen_fd, (struct sockaddr *) &mme_server_addr, sizeof(mme_server_addr));
	if(retval < 0)
	{
		TRACE(cout<<"Error: mtcp RAN bind call"<<endl;)
		exit(-1);
	}

	retval = mtcp_listen(mctx, ran_listen_fd, MAXCONN);
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

	epevent.data.sockid = ran_listen_fd;
	epevent.events = MTCP_EPOLLIN | MTCP_EPOLLET | MTCP_EPOLLRDHUP;
	retval = mtcp_epoll_ctl(mctx, epollfd, EPOLL_CTL_ADD, ran_listen_fd, &epevent);
	if(retval == -1)
	{
		TRACE(cout<<"Error: mtcp epoll_ctl_add ran"<<endl;)
		exit(-1);
	}

	/*
	MAIN LOOP
	*/
	int con_accepts = 0;
	int con_processed = 0;

	while(!done)
	{
		//could use done

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
			if( (return_events[i].events & MTCP_EPOLLERR) ||
				(return_events[i].events & MTCP_EPOLLHUP))
			{

				cout<<"\n\nError: epoll event returned : "<<return_events[i].data.sockid<<" errno :"<<errno<<endl;
				if(return_events[i].data.sockid == ran_listen_fd)
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
			if(revent.data.sockid == ran_listen_fd) 
			{	
				//If event in listening fd, its new connection
				//RAN ACCEPTS
				while(1)
				{
					ran_fd = mtcp_accept(mctx, ran_listen_fd, NULL, NULL);
					if(ran_fd < 0)
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
					epevent.data.sockid = ran_fd;
					mtcp_setsock_nonblock(mctx, ran_fd);
					retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, ran_fd, &epevent);
					if(retval < 0)
					{
						cout<<"Ran accept epoll add error "<<errno<<" retval "<<retval<<" core "<<core<<endl;
						exit(-1);
					}
					fddata.act = 1;
					fddata.initial_fd = -1;
					fddata.msui = -1;
					memset(fddata.buf,'\0',500);
					fddata.buflen = 0;
					fdmap.insert(make_pair(ran_fd, fddata));
					con_accepts++;
				}//while accepts
				TRACE(cout<<" Core "<<core<<" accepted "<<con_accepts<<" till now "<<endl;)
				//go to act_type case 1
			}
			else
			{
				cur_fd = revent.data.sockid;
				fddata = fdmap[cur_fd];
				act_type = fddata.act;

				//Check action type
				switch(act_type)
				{
					case 1:
						if(revent.events & MTCP_EPOLLIN)
						{
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error ran epoll read delete from epoll"<<endl;
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);							
							if(retval == 0)
							{
								cout<<"Connection closed by RAN, core: "<<core<<endl;
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

							pkt.extract_s1ap_hdr();

							if (pkt.s1ap_hdr.mme_s1ap_ue_id == 0) {
							//1st attach from ran
								num_autn_vectors = 1;
								pkt.extract_item(imsi);
								pkt.extract_item(tai);
								pkt.extract_item(ksi_asme); /* No use in this case */
								pkt.extract_item(nw_capability); /* No use in this case */
								
								enodeb_s1ap_ue_id = pkt.s1ap_hdr.enodeb_s1ap_ue_id;
								guti = g_telecom.get_guti(mme_ids.gummei, imsi);
								TRACE(cout<<"A1: IMSI from RAN :"<<imsi<<" guti "<<guti<<" Core "<<core<<endl;)
								mlock(uectx_mux);
								ue_count++;
								mme_s1ap_ue_id = ue_count;
								//TRACE(cout<<"assigned:"<<guti<<":"<<ue_count<<endl;)
								ue_ctx[guti].init(imsi, enodeb_s1ap_ue_id, mme_s1ap_ue_id, tai, nw_capability);
								nw_type = ue_ctx[guti].nw_type;
								if(ue_count == 0)
									cout<<"Trigger ue count 0 "<<imsi<<"\n\n";
								munlock(uectx_mux);

								mlock(s1mmeid_mux);
								s1mme_id[mme_s1ap_ue_id] = guti;
								munlock(s1mmeid_mux);

								//connect to hss
								pkt.clear_pkt();
								pkt.append_item(imsi);
								pkt.append_item(mme_ids.plmn_id);
								pkt.append_item(num_autn_vectors);
								pkt.append_item(nw_type);
								pkt.prepend_diameter_hdr(1, pkt.len);
								pkt.prepend_len();
								

								hss_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
								if(hss_fd < 0)
								{
									cout<<"Error: hss new socket fd error"<<endl;
									exit(-1);
								}
								retval = mtcp_setsock_nonblock(mctx, hss_fd);
								if(retval < 0)
								{
									cout<<"Error: create hss nonblock"<<endl;
									exit(-1);
								}
								retval = mtcp_connect(mctx, hss_fd, (struct sockaddr*) &hss_server_addr, sizeof(struct sockaddr_in));
								if((retval < 0 ) && (errno == EINPROGRESS))
								{
									epevent.events = MTCP_EPOLLOUT;
									epevent.data.sockid = hss_fd;
									returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, hss_fd, &epevent);
									if(returnval < 0)
									{
										cout<<"Error: epoll hss add"<<endl;
										exit(-1);
									}
									fdmap.erase(cur_fd);
									fddata.act = 2;
									fddata.initial_fd = cur_fd;
									fddata.msui = mme_s1ap_ue_id;
									memcpy(fddata.buf, pkt.data, pkt.len);
									fddata.buflen = pkt.len;
									fdmap.insert(make_pair(hss_fd, fddata));	
								}
								else
								if(retval < 0)
								{
									cout<<"ERror: hss connect error"<<endl;
									exit(-1);
								}
								else
								{
									cout<<"Hss connected immidietly, handle now\n";
									exit(-1);
								}

							}//1st attach ends
							else
							if (pkt.s1ap_hdr.msg_type == 2) {
							//2 attach from ran
								guti = 0;
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
								
								mlock(s1mmeid_mux);
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								munlock(s1mmeid_mux);
								
								if (guti == 0) {
									cout<<"Error: guit 0 ran_accept_fd case 2 "<<endl;
									exit(-1);
								}

								pkt.extract_item(res);
								
								mlock(uectx_mux);
								xres = ue_ctx[guti].xres;
								munlock(uectx_mux);

								if (res == xres)
								{
									TRACE(cout << "mme_handleautn:" << " Authentication successful: " << guti << endl;)
								}	
								else
								{
									cout<<"mme_handleautn: Failed to authenticate !"<<guti<<" "<<xres<<" "<<res<<endl;
									exit(-1);
								}

								mlock(uectx_mux);
								ue_ctx[guti].nas_enc_algo = 1;
								ue_ctx[guti].k_nas_enc = ue_ctx[guti].k_asme + ue_ctx[guti].nas_enc_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
								ksi_asme = ue_ctx[guti].ksi_asme;
								nw_capability = ue_ctx[guti].nw_capability;
								nas_enc_algo = ue_ctx[guti].nas_enc_algo;
								nas_int_algo = ue_ctx[guti].nas_int_algo;
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								munlock(uectx_mux);


								pkt.clear_pkt();
								pkt.append_item(ksi_asme);
								pkt.append_item(nw_capability);
								pkt.append_item(nas_enc_algo);
								pkt.append_item(nas_int_algo);
									
								if (HMAC_ON) {
									g_integrity.add_hmac(pkt, k_nas_int);
								
								}
								
								pkt.prepend_s1ap_hdr(2, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
								pkt.prepend_len();

								retval = mtcp_write(mctx, cur_fd,  pkt.data, pkt.len);
								if(retval < 0)
								{
									cout<<"Error MME write back to RAN A2"<<endl;
									exit(-1);
								}
															
								
								
								fdmap.erase(cur_fd);
								//mtcp_close(mctx, ran_fd);
								//con_processed++;
								//cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;
								
								fddata.act = 1;
								fddata.buflen = 0;
								fddata.initial_fd = -1;
								fddata.msui = 0;
								memset(fddata.buf, '\0', 500);
								fdmap.insert(make_pair(cur_fd, fddata));

								epevent.events = MTCP_EPOLLIN;
								epevent.data.sockid = cur_fd;
								retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, cur_fd, &epevent);
								if(retval < 0)
								{
									TRACE(cout<<"Error adding ran for attach 2 to epoll"<<endl;)
									exit(-1);
								}
								

							}//if a2
							else
							if (pkt.s1ap_hdr.msg_type == 3) {
								//Attach third
								guti = 0;
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
								
								mlock(s1mmeid_mux);
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								else
								{
									cout<<"Error finding s1apid"<<endl;
									exit(-1);
								}
								munlock(s1mmeid_mux);

								mlock(uectx_mux);
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								munlock(uectx_mux);					

								if (HMAC_ON) {
									res = g_integrity.hmac_check(pkt, k_nas_int);
									if (res == false) {
										cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;
										exit(-1);
									}		
								}
								
								if (ENC_ON) {
									g_crypt.dec(pkt, k_nas_enc);
								}
								pkt.extract_item(res);
								if (res == false) {
									cout << "mme_handlesecuritymodecomplete:" << " security mode complete failure: " << guti << endl;
									exit(-1);
								}
								else {
									TRACE(cout << "mme_handlesecuritymodecomplete:" << " security mode complete success: " << guti << endl;)
								}
	
								eps_bearer_id = 5;
								mlock(uectx_mux);
								ue_ctx[guti].pgw_s5_port = g_pgw_s5_port;
								ue_ctx[guti].pgw_s5_ip_addr = g_pgw_s5_ip_addr;
								tem = to_string(guti);
								tem = tem.substr(7, -1);
								s11_cteid_mme = stoull(tem);
								ue_ctx[guti].s11_cteid_mme = s11_cteid_mme;
								ue_ctx[guti].eps_bearer_id = eps_bearer_id;
								imsi = ue_ctx[guti].imsi;
								eps_bearer_id = ue_ctx[guti].eps_bearer_id;
								pgw_s5_ip_addr = ue_ctx[guti].pgw_s5_ip_addr;
								pgw_s5_port = ue_ctx[guti].pgw_s5_port;
								ue_ctx[guti].apn_in_use = 0;
								apn_in_use = ue_ctx[guti].apn_in_use;
								tai = ue_ctx[guti].tai;	
								munlock(uectx_mux);

								pkt.clear_pkt();
								pkt.append_item(s11_cteid_mme);
								pkt.append_item(imsi);
								pkt.append_item(eps_bearer_id);
								pkt.append_item(pgw_s5_ip_addr);
								pkt.append_item(pgw_s5_port);
								pkt.append_item(apn_in_use);
								pkt.append_item(tai);
								pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);
								pkt.prepend_len();
								//connect to sgw
								
								//kanase edits

								struct sockaddr_in sgw_server_addr, client_addr;

								bzero((char *) &client_addr, sizeof(client_addr));								

								bzero((char *) &sgw_server_addr, sizeof(sgw_server_addr));
								sgw_server_addr.sin_family = AF_INET;
								sgw_server_addr.sin_addr.s_addr = inet_addr(sgw_ip);
								sgw_server_addr.sin_port = htons(sgw_s11_portnum);

								returnval = FetchAddressPerCore(ap, core, MAX_THREADS, &sgw_server_addr, &client_addr);

								if(returnval == -1) 
								{
									cout<<"FetchAddressPerCore error\n";
									exit(-1);
								}

								sgw_fd = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
								if(sgw_fd < 0)
								{
									cout<<"Error: sgw new socket fd error"<<endl;
									exit(-1);
								}

								srcaddrmap[sgw_fd] = client_addr;
								//done kanase edits

								retval = mtcp_setsock_nonblock(mctx, sgw_fd);
								if(retval < 0)
								{
									cout<<"Error: create sgw nonblock"<<endl;
									exit(-1);
								}

								client_addr.sin_family = AF_INET;
								retval = mtcp_bind(mctx, sgw_fd,(struct sockaddr *)&client_addr, sizeof(struct sockaddr_in));
						        if (retval < 0) {
						                cout<<"Error failed binding\n";
						                exit(-1);
						        }
						        else
						        {
						        	TRACE(cout<<"Bound successfully\n";)
						        }

								retval = mtcp_connect(mctx, sgw_fd, (struct sockaddr*) &sgw_server_addr, sizeof(struct sockaddr_in));
								if((retval < 0 ) && (errno == EINPROGRESS))
								{
									epevent.events = MTCP_EPOLLOUT;
									epevent.data.sockid = sgw_fd;
									returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, sgw_fd, &epevent);
									if(returnval < 0)
									{
										cout<<"Error: epoll sgw add"<<endl;
										exit(-1);
									}
									fdmap.erase(cur_fd);
									fddata.act = 4;
									fddata.initial_fd = cur_fd;
									fddata.msui = mme_s1ap_ue_id;
									memcpy(fddata.buf, pkt.data, pkt.len);
									fddata.buflen = pkt.len;
									fdmap.insert(make_pair(sgw_fd, fddata));
									//cout<<"Conn to sgw\n";	
								}
								else
								if(retval < 0)
								{
									cout<<"ERror: sgw connect error"<<endl;
									exit(-1);
								}
								else
								{
									cout<<"Sgw connected immidietly, handle now\n";
									exit(-1);
								}

							}//if A3
							else
							if (pkt.s1ap_hdr.msg_type == 4) {
								//Attach fourth

								guti = 0;
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;

								mlock(s1mmeid_mux);									
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								else
								{
									cout<<"s1mme mme  s1ueid not found"<<endl;
									exit(-1);
								}
								munlock(s1mmeid_mux);

								mlock(uectx_mux);
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								imsi = ue_ctx[guti].imsi;
								munlock(uectx_mux);
								
								if (HMAC_ON) {
									res = g_integrity.hmac_check(pkt, k_nas_int);
									if (res == false) {
										TRACE(cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;)
										exit(-1);
									}		
								}
								if (ENC_ON) {
									g_crypt.dec(pkt, k_nas_enc);
								}

								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(s1_uteid_dl);

								mlock(uectx_mux);
								ue_ctx[guti].s1_uteid_dl = s1_uteid_dl;
								ue_ctx[guti].emm_state = 1;
								
								eps_bearer_id = ue_ctx[guti].eps_bearer_id;
								s1_uteid_dl = ue_ctx[guti].s1_uteid_dl;
								s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
								munlock(uectx_mux);

								TRACE(cout<<" Attach 4 "<<guti<<" s11_cteid_sgw "<<s11_cteid_sgw<<endl;)
								pkt.clear_pkt();
								pkt.append_item(eps_bearer_id);
								pkt.append_item(s1_uteid_dl);
								pkt.append_item(g_trafmon_ip_addr);
								pkt.append_item(g_trafmon_port);
								pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_sgw);
								pkt.prepend_len();

								sgw_fd = fddata.initial_fd;
								returnval = mtcp_write(mctx, sgw_fd, pkt.data, pkt.len);
								if(returnval < 0)
								{
									cout<<"Error: sgw write attach four"<<errno<<endl;
									exit(-1);
								}
								
								epevent.data.sockid = sgw_fd;
								epevent.events = MTCP_EPOLLIN;
								returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, sgw_fd, &epevent);
								if(returnval == -1)
								{
									cout<<"Error: sgw add a4 mtcp error"<<endl;
									exit(-1);
								}

								fdmap.erase(cur_fd);
								fddata.act = 6;
								fddata.msui = mme_s1ap_ue_id;
								fddata.buflen = 0;
								memset(fddata.buf, '\0', 500);
								fddata.initial_fd = cur_fd;
								fdmap.insert(make_pair(sgw_fd, fddata));

							}//if A4
							else
							if (pkt.s1ap_hdr.msg_type == 5) {
								//Detach
								guti = 0;
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;

								mlock(s1mmeid_mux);									
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								else
								{
									cout<<"s1mme mme  s1ueid not found"<<endl;
									exit(-1);
								}
								munlock(s1mmeid_mux);

								mlock(uectx_mux);
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								eps_bearer_id = ue_ctx[guti].eps_bearer_id;
								tai = ue_ctx[guti].tai;
								s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
								munlock(uectx_mux);

								TRACE(cout<<"GUTI "<<guti<<" s11 "<<s11_cteid_sgw<<endl;)
								
								if (HMAC_ON) {
									res = g_integrity.hmac_check(pkt, k_nas_int);
									if (res == false) {
										cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;
										exit(-1);
									}		
								}
								if (ENC_ON) {
									g_crypt.dec(pkt, k_nas_enc);
								}

								pkt.extract_item(guti);
								pkt.extract_item(ksi_asme);
								pkt.extract_item(detach_type);	
									
								pkt.clear_pkt();
								pkt.append_item(eps_bearer_id);
								pkt.append_item(tai);
								pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_sgw);
								pkt.prepend_len();

								TRACE(cout<<"Detach Packet "<<guti<<" "<<s11_cteid_sgw<<endl;)
								//send to sgw

								sgw_fd = fddata.initial_fd;
								returnval = mtcp_write(mctx, sgw_fd, pkt.data, pkt.len);
								if(returnval < 0)
								{
									cout<<"Error: sgw write detach"<<errno<<endl;
									exit(-1);
								}
								
								epevent.data.sockid = sgw_fd;
								epevent.events = MTCP_EPOLLIN;
								returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, sgw_fd, &epevent);
								if(returnval == -1)
								{
									cout<<"Error: sgw add detach mtcp error"<<endl;
									exit(-1);
								}

								fdmap.erase(cur_fd);
								fddata.act = 7;
								fddata.msui = mme_s1ap_ue_id;
								fddata.buflen = 0;
								memset(fddata.buf, '\0', 500);
								fddata.initial_fd = cur_fd;
								fdmap.insert(make_pair(sgw_fd, fddata));

							}
							else
							{
								cout<<"Error ran pkt Wrong Header "<<endl;
								exit(-1);
							}
						
						}// case 1 right event
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
						//connected to hss
					if(revent.events & MTCP_EPOLLOUT)
					{	
						retval = mtcp_write(mctx, cur_fd, fddata.buf, fddata.buflen);
						if(retval < 0)
						{
							cout<<"Error: Hss write data"<<endl;
							exit(-1);
						}
						epevent.data.sockid = cur_fd;
						epevent.events = MTCP_EPOLLIN;
						returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_MOD, cur_fd, &epevent);
						if(returnval < 0)
						{
							cout<<"Error hss mod add epoll\n";
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
					//data from hss
					if(revent.events & MTCP_EPOLLIN)
					{
						retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
						if(retval < 0)
						{
							cout<<"Error hss epoll reply delete \n";
							exit(-1);
						}

						pkt.clear_pkt();
						retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
						if(retval < 0)
						{
							cout<<"Error hss read data "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
							exit(-1);
						}
						memcpy(&pkt_len, data, sizeof(int));
						dataptr = data+sizeof(int);
						memcpy(pkt.data, (dataptr), pkt_len);
						pkt.data_ptr = 0;
						pkt.len = pkt_len;

						mtcp_close(mctx, cur_fd);

						pkt.extract_diameter_hdr();
						pkt.extract_item(autn_num);
						pkt.extract_item(rand_num);
						pkt.extract_item(xres);
						pkt.extract_item(k_asme);

						mme_s1ap_ue_id = fddata.msui;

						mlock(s1mmeid_mux);
						if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
							guti = s1mme_id[mme_s1ap_ue_id];
						}
						munlock(s1mmeid_mux);
						
						mlock(uectx_mux);
						ue_ctx[guti].xres = xres;
						ue_ctx[guti].k_asme = k_asme;
						ue_ctx[guti].ksi_asme = 1;
						ksi_asme = ue_ctx[guti].ksi_asme;
						enodeb_s1ap_ue_id = ue_ctx[guti].enodeb_s1ap_ue_id;
						imsi = ue_ctx[guti].imsi;
						if(imsi == 0)
						{
							bkp_imsi = guti % 1000000000000;
							cout<<"\n\n msui "<<mme_s1ap_ue_id<<" guti "<<guti<<" imsi "<<imsi<<" bkp "<<bkp_imsi<<endl;
							ue_ctx[guti].imsi = bkp_imsi;
						}
						munlock(uectx_mux);

						pkt.clear_pkt();
						pkt.append_item(autn_num);
						pkt.append_item(rand_num);
						pkt.append_item(ksi_asme);
						
						pkt.prepend_s1ap_hdr(1, pkt.len, enodeb_s1ap_ue_id, mme_s1ap_ue_id);
						pkt.prepend_len();
						ran_fd = fddata.initial_fd;
						
						retval = mtcp_write(mctx, ran_fd, pkt.data, pkt.len);
						if(retval < 0)
						{
							cout<<"Error: Ran send attach 1 error\n";
							exit(-1);
						}


						fdmap.erase(cur_fd);
						//mtcp_close(mctx, ran_fd);
						//con_processed++;
						//cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;
						
						fddata.act = 1;
						fddata.buflen = 0;
						fddata.initial_fd = -1;
						fddata.msui = 0;
						memset(fddata.buf, '\0', 500);
						fdmap.insert(make_pair(ran_fd, fddata));

						epevent.events = MTCP_EPOLLIN;
						epevent.data.sockid = ran_fd;
						retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, ran_fd, &epevent);
						if(retval < 0)
						{
							TRACE(cout<<"Error adding ran for attach 2 to epoll"<<endl;)
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
						//connected to sgw
						if(revent.events & MTCP_EPOLLOUT)
						{	
							//cout<<"Done conn sgw\n";
							retval = mtcp_write(mctx, cur_fd, fddata.buf, fddata.buflen);
							if(retval < 0)
							{
								cout<<"Error: sgw write data"<<endl;
								exit(-1);
							}
							epevent.data.sockid = cur_fd;
							epevent.events = MTCP_EPOLLIN;
							returnval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_MOD, cur_fd, &epevent);
							if(returnval < 0)
							{
								cout<<"Error sgw mod add epoll\n";
								exit(-1);
							}
							fdmap.erase(cur_fd);
							fddata.act = 5;
							fddata.buflen = 0;
							memset(fddata.buf, '\0', 500);
							fdmap.insert(make_pair(cur_fd, fddata));

						}
						else
						{
							cout<<"Wrong event at case 4\n";
							exit(-1);
						}
					break;//case 4

					case 5:
						//data from sgw
						if(revent.events & MTCP_EPOLLIN)
						{
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error sgw epoll reply delete \n";
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
							if(retval < 0)
							{
								cout<<"Error sgw read data "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}
							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;

							

							ran_fd = fddata.initial_fd;
							mme_s1ap_ue_id = fddata.msui;
							
							mlock(s1mmeid_mux);
							if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
								guti = s1mme_id[mme_s1ap_ue_id];
							}
							else
							{
								cout<<"Error A3 response"<<endl;
								exit(-1);
							}
							munlock(s1mmeid_mux);

							pkt.extract_gtp_hdr();
							pkt.extract_item(s11_cteid_sgw);
							pkt.extract_item(ue_ip_addr);
							pkt.extract_item(s1_uteid_ul);
							pkt.extract_item(s5_uteid_ul);
							pkt.extract_item(s5_uteid_dl);
							
							if(gettid(guti) != s11_cteid_sgw)
							{
								cout<<"GUTI not equal A3response"<<guti<<" "<<s11_cteid_sgw<<endl;
								exit(-1);
							}

							mlock(uectx_mux);
							ue_ctx[guti].ip_addr = ue_ip_addr;
							ue_ctx[guti].s11_cteid_sgw = s11_cteid_sgw;
							ue_ctx[guti].s1_uteid_ul = s1_uteid_ul;
							ue_ctx[guti].s5_uteid_ul = s5_uteid_ul;
							ue_ctx[guti].s5_uteid_dl = s5_uteid_dl;
							ue_ctx[guti].tai_list.clear();
							ue_ctx[guti].tai_list.push_back(ue_ctx[guti].tai);
							ue_ctx[guti].tau_timer = g_timer;
							ue_ctx[guti].e_rab_id = ue_ctx[guti].eps_bearer_id;
							ue_ctx[guti].k_enodeb = ue_ctx[guti].k_asme;
							e_rab_id = ue_ctx[guti].e_rab_id;
							k_enodeb = ue_ctx[guti].k_enodeb;
							nw_capability = ue_ctx[guti].nw_capability;
							tai_list = ue_ctx[guti].tai_list;
							tau_timer = ue_ctx[guti].tau_timer;
							k_nas_enc = ue_ctx[guti].k_nas_enc;
							k_nas_int = ue_ctx[guti].k_nas_int;
							imsi = ue_ctx[guti].imsi;
							munlock(uectx_mux);

							epsres = true;
							tai_list_size = 1;	

							pkt.clear_pkt();							
							
							pkt.append_item(guti);
							pkt.append_item(eps_bearer_id);
							pkt.append_item(e_rab_id);
							pkt.append_item(s1_uteid_ul);
							pkt.append_item(k_enodeb);
							pkt.append_item(nw_capability);
							pkt.append_item(tai_list_size);
							pkt.append_item(tai_list);
							pkt.append_item(tau_timer);
							pkt.append_item(ue_ip_addr);
							pkt.append_item(g_sgw_s1_ip_addr);
							pkt.append_item(g_sgw_s1_port);
							pkt.append_item(epsres);

							if (ENC_ON) {
								g_crypt.enc(pkt, k_nas_enc);
							}
							if (HMAC_ON) {
								g_integrity.add_hmac(pkt, k_nas_int);
							}
							pkt.prepend_s1ap_hdr(3, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
							pkt.prepend_len();
							

							retval = mtcp_write(mctx, ran_fd, pkt.data, pkt.len);
							if(retval < 0)
							{
								cout<<"Error: Ran send attach 3 error\n";
								exit(-1);
							}

							fdmap.erase(cur_fd);
							
							//mtcp_close(mctx, cur_fd);
							//mtcp_close(mctx, ran_fd);
							//con_processed++;
							//cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;
							
							fddata.act = 1;
							fddata.buflen = 0;
							fddata.initial_fd = cur_fd;
							fddata.msui = 0;
							memset(fddata.buf, '\0', 500);
							fdmap.insert(make_pair(ran_fd, fddata));

							epevent.events = MTCP_EPOLLIN;
							epevent.data.sockid = ran_fd;
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, ran_fd, &epevent);
							if(retval < 0)
							{
								TRACE(cout<<"Error adding ran after attach 3 to epoll"<<endl;)
								exit(-1);
							}
							
						}
						else
						{
							cout<<"Wrong event at case 5\n";
							exit(-1);
						}	
					break;//case 5

					case 6:
						//data from sgw
						if(revent.events & MTCP_EPOLLIN)
						{

							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error sgw epoll reply delete \n";
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
							if(retval < 0)
							{
								cout<<"Error sgw read data "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}

							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;
							
							ran_fd = fddata.initial_fd;
							mme_s1ap_ue_id = fddata.msui;
							
							mlock(s1mmeid_mux);
							if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
								guti = s1mme_id[mme_s1ap_ue_id];
							}
							else
							{
								cout<<"Error A4 response"<<endl;
								exit(-1);
							}
							munlock(s1mmeid_mux);

							pkt.extract_gtp_hdr();
							pkt.extract_item(res);
							if (res == false) {
								cout << "mme_handlemodifybearer:" << " modify bearer failure: " << guti << endl;
								exit(-1);
							}
							else {
								mlock(uectx_mux);
								ue_ctx[guti].ecm_state = 1;
								nw_capability = ue_ctx[guti].nw_capability;
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								imsi = ue_ctx[guti].imsi;
								munlock(uectx_mux);
							}

							epsres = true;
							pkt.clear_pkt();
							
							pkt.append_item(epsres);
							pkt.append_item(guti);
							pkt.append_item(nw_capability);
							if (ENC_ON) {
								g_crypt.enc(pkt, k_nas_enc);
							}
							if (HMAC_ON) {
								g_integrity.add_hmac(pkt, k_nas_int);
							}
							pkt.prepend_s1ap_hdr(4, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
							pkt.prepend_len();

							returnval = mtcp_write(mctx, ran_fd, pkt.data, pkt.len);						
							if(returnval < 0)
							{
								cout<<"Error: Cant send to RAN A4"<<endl;
								exit(-1);
							}

							fdmap.erase(cur_fd);
							//mtcp_close(mctx, cur_fd);
							//mtcp_close(mctx, ran_fd);
							//con_processed++;
							//cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;
							
							fddata.act = 1;
							fddata.buflen = 0;
							fddata.initial_fd = cur_fd;
							fddata.msui = 0;
							memset(fddata.buf, '\0', 500);
							fdmap.insert(make_pair(ran_fd, fddata));

							epevent.events = MTCP_EPOLLIN;
							epevent.data.sockid = ran_fd;
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, ran_fd, &epevent);
							if(retval < 0)
							{
								TRACE(cout<<"Error adding ran after attach 4 to epoll"<<endl;)
								exit(-1);
							}
							
	
						}//if event
						else
						{
							cout<<"Wrong event at case 6\n";
							exit(-1);
						}
					break;
					
					case 7:
						//data from sgw
						if(revent.events & MTCP_EPOLLIN)
						{
							retval = mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, cur_fd, NULL);
							if(retval < 0)
							{
								cout<<"Error sgw epoll reply delete \n";
								exit(-1);
							}

							pkt.clear_pkt();
							retval = mtcp_read(mctx, cur_fd, data, BUF_SIZE);
							if(retval < 0)
							{
								cout<<"Error sgw read data "<<errno<<" retval "<<retval<<" Core "<<core<<endl;
								exit(-1);
							}

							memcpy(&pkt_len, data, sizeof(int));
							dataptr = data+sizeof(int);
							memcpy(pkt.data, (dataptr), pkt_len);
							pkt.data_ptr = 0;
							pkt.len = pkt_len;
							
							ran_fd = fddata.initial_fd;
							mme_s1ap_ue_id = fddata.msui;
							
							mlock(s1mmeid_mux);
							if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
								guti = s1mme_id[mme_s1ap_ue_id];
							}
							else
							{
								cout<<"Error detach response"<<endl;
								exit(-1);
							}
							munlock(s1mmeid_mux);

							pkt.extract_gtp_hdr();
							pkt.extract_item(res);
							if (res == false) {
								cout << "mme_handlemodifybearer:" << " modify bearer failure: " << guti << endl;
							}
							else {
								mlock(uectx_mux);								
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								munlock(uectx_mux);
							}

							epsres = true;
							pkt.clear_pkt();
							pkt.append_item(epsres);
							if (ENC_ON) {
								g_crypt.enc(pkt, k_nas_enc);
							}
							if (HMAC_ON) {
								g_integrity.add_hmac(pkt, k_nas_int);
							}
							pkt.prepend_s1ap_hdr(5, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
							pkt.prepend_len();
							//cout<<"Sent Detach \n";
							returnval = mtcp_write(mctx, ran_fd, pkt.data, pkt.len);						
							if(returnval < 0)
							{
								cout<<"Error: Cant send to RAN detach"<<endl;
								exit(-1);
							}

							fdmap.erase(cur_fd);
							fdmap.erase(ran_fd);
							mtcp_close(mctx, cur_fd);
							mtcp_close(mctx, ran_fd);

							if(srcaddrmap.find(cur_fd) != srcaddrmap.end())
							{
								struct sockaddr_in sgw_addr = srcaddrmap[cur_fd];
								FreeAddress(ap, &sgw_addr);
								srcaddrmap.erase(cur_fd);
							}
							else
							{
								cout<<"Address to free not found\n";
								exit(-1);
							}

							mlock(s1mmeid_mux);
							s1mme_id.erase(mme_s1ap_ue_id);
							munlock(s1mmeid_mux);

							mlock(uectx_mux);
							ue_ctx.erase(guti);
							munlock(uectx_mux);

							con_processed++;
							TRACE(cout<<"Conn Processed "<<con_processed<<" Core "<<core<<endl;)
							

						}//if event
						else
						{
							cout<<"Wrong event at case 6\n";
							exit(-1);
						}

					break;
					default:
						cout<<"Error unknown switch case"<<endl;
						cout<<"Act "<<act_type<<" fd "<<cur_fd<<endl;
						if(cur_fd == sgw_fd)
							cout<<"SGW ";
						if(cur_fd == ran_fd)
							cout<<"Ran ";
						if(cur_fd == ran_listen_fd)
							cout<<"Ran Linten";
						if(cur_fd == hss_fd)
							cout<<"hss ";
					break;//default
				}//switch close;
			}//close for action events
		}//for i-numevents loop ends
	}//end while(1);
}//end run()


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

	//Initialize locks, used for multicore
	s1mme_id.clear();
	ue_ctx.clear(); //for testing a4 sgw
	ue_count = 0;
	done = 0;

	mux_init(s1mmeid_mux);
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
