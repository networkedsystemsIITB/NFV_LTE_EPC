#include "defport.h"
#include "common.h"
#include <sys/epoll.h>
//#include "mme.cpp"

void run()
{
	int ran_listen_fd;
	int ran_accept_fd;
	int hss_fd;
	int sgw_fd;
	int cur_fd;
	int udp_fd;
	int act_type;
	int portnum;
	int err;
	int lflag,uflag;
	long long ecount = 0;
	socklen_t len;
		
	int closedconn=0;

	//	udp attach 3 ports setup	---
	int udp_attach_begin = udp_slot_one;
	int udp_attach_end = udp_slot_one + udp_slot_size;
	int udp_attach_port;

	queue<int> udp_attach_free_ports;
	set<int> udp_attach_use_ports;

	for(int i = udp_attach_begin; i<= udp_attach_end; i++ )
		udp_attach_free_ports.push(i);
	

	// udp attach 4 port setup
	int udp_afour_begin = udp_slot_two;	
	int udp_afour_end = udp_slot_two + udp_slot_size;
	int udp_afour_port;
	queue<int> udp_afour_free_ports;
	set<int> udp_afour_use_ports;

	for(int i = udp_afour_begin; i <= udp_afour_end; i++)
		udp_afour_free_ports.push(i);



	// udp detach port setup
	int udp_detach_begin = udp_slot_three;
	int udp_detach_end = udp_slot_three + udp_slot_size;
	int udp_detach_port;
	queue<int> udp_detach_free_ports;
	set<int> udp_detach_use_ports;

	for(int i = udp_detach_begin; i <= udp_detach_end; i++)
		udp_detach_free_ports.push(i);

	//	sgw udp setup
	int nbytes;
	struct sockaddr_in udp_sock_bind;
	struct sockaddr_in sgw_addr;
	socklen_t sgwsocklen = sizeof(udp_sock_bind);
	

	memset((char *) &sgw_addr, 0 ,sizeof(sgw_addr));
	sgw_addr.sin_family = AF_INET;
	sgw_addr.sin_port = htons(sgw_s11_port);
	sgw_addr.sin_addr.s_addr = inet_addr(sgw_ip);
	memset(sgw_addr.sin_zero, '\0', sizeof(sgw_addr.sin_zero));
	sgwsocklen = sizeof(sgw_addr);
	//sgw udp setup done



	int epollfd;
	int retval;
	int returnval;
	struct epoll_event epevent;
	struct epoll_event repevent;
	struct epoll_event *return_events;
	int numevents;
	
	map<int, mdata> fdmap;
	
	//MME data class Mme mme.h l:113
	MmeIds mme_ids;
	uint64_t ue_count = 0;
	unordered_map<uint32_t, uint64_t> s1mme_id; /* S1_MME UE identification table: mme_s1ap_ue_id -> guti */
	unordered_map<uint64_t, UeContext> ue_ctx; /* UE context table: guti -> UeContext */
	s1mme_id.clear();
	ue_ctx.clear();

	/* Lock parameters */
	pthread_mutex_t s1mmeid_mux; /* Handles s1mme_id and ue_count */
	pthread_mutex_t uectx_mux; /* Handles ue_ctx */

	mux_init(s1mmeid_mux);
	mux_init(uectx_mux);

	Packet pkt;
	int pkt_len;
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
	uint32_t s1_uteid_dl;	//	case attach 4
	uint32_t s5_uteid_ul;
	uint32_t s5_uteid_dl;
	uint64_t detach_type;	//  case detach 
	uint8_t eps_bearer_id;
	uint8_t e_rab_id;
	string pgw_s5_ip_addr;
	string ue_ip_addr;
	int tai_list_size;
	int pgw_s5_port;
	string tem;

	num_autn_vectors = 1;

	//case specific variables
	bool epsres;



	struct mdata fddata;

	//	mme listen setup
	struct sockaddr_in mme_server_addr;
	struct sockaddr_in hss_server_addr;
	struct hostent *hss_ipaddr;

	
	ran_listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(ran_listen_fd < 0)
		{
			TRACE(cout<<"Error: RAN socket call"<<endl;)
			exit(-1);
		}
	


		bzero((char *) &mme_server_addr, sizeof(mme_server_addr));
		mme_server_addr.sin_family = AF_INET;
		mme_server_addr.sin_addr.s_addr = inet_addr(mme_ip);
		mme_server_addr.sin_port = htons(mme_my_port);

	if(bind(ran_listen_fd, (struct sockaddr *) &mme_server_addr, sizeof(mme_server_addr)) < 0)
	{
		TRACE(cout<<"Error: RAN bind call"<<endl;)
		exit(-1);
	}

	listen(ran_listen_fd, MAXCONN);
	//mme listen setup done



	


	//	epoll setup
	epollfd = epoll_create(MAXEVENTS);
	if(epollfd == -1)
	{
		TRACE(cout<<"Error: RAN epoll_create"<<endl;)
		exit(-1);
	}

	epevent.data.fd = ran_listen_fd;
	epevent.events = EPOLLIN | EPOLLET;
	retval = epoll_ctl(epollfd, EPOLL_CTL_ADD, ran_listen_fd, &epevent);
	if(retval == -1)
	{
		TRACE(cout<<"Error: RAN epoll_ctl"<<endl;)
		exit(-1);
	}

	return_events = (epoll_event *) malloc (sizeof (struct epoll_event) * MAXEVENTS);
	if (!return_events) 
	{
		TRACE(cout<<"Error: malloc failed for revents"<<endl;)
		exit(-1);
	}
	// epoll setup done
	lflag = 1;
	
	//MAIN LOOP
	while(1)
	{

		if(lflag == 1)
		{
			//TRACEP(cout<<"Going to While : "<<endl;)
			cout<<"Going to While : "<<endl;
			lflag = 0;
		}
		numevents = epoll_wait(epollfd, return_events, MAXEVENTS, -1);
		if(numevents < 0)
		{
			cout<<"Error: Epoll Wait : errno";
			cout<<errno<<endl;
			exit(-1);
		}

		if(numevents == 0)
		{
			//TRACE(cout<<"Info: Tick Epoll Wait"<<endl;)
		}

		for(int i = 0; i < numevents; ++i)
		{
			//check for errors
			if( (return_events[i].events & EPOLLERR) ||
				(return_events[i].events & EPOLLHUP)
				)
			{

				TRACE(cout<<"\n\nError: epoll event returned : "<<return_events[i].data.fd<<" errno :"<<errno<<endl;)
				if(return_events[i].data.fd == ran_listen_fd)
				{
					TRACE(cout<<"Error: Its Ran Listen fd"<<endl;)
					cout<<"Error if in while"<<endl;
				}
				close(return_events[i].data.fd);
				continue;
			}
		
							/*		
							if((return_events[i].events & EPOLLIN))	
								cout<<"EPOLLIN"<<endl;
							if((return_events[i].events & EPOLLERR))	
								cout<<"EPOLLERR"<<endl;
							if((return_events[i].events & EPOLLHUP))	
								cout<<"EPOLLHUP"<<endl;
							if((return_events[i].events & EPOLLRDHUP))	
								cout<<"EPOLLRDHUP"<<endl;
							if((return_events[i].events & EPOLLONESHOT))	
								cout<<"EPOLLONESHOT"<<endl;
							if((return_events[i].events & EPOLLOUT))	
								cout<<"EPOLLOUT"<<endl;
							if((return_events[i].events & EPOLLPRI))	
								cout<<"EPOLLPRI"<<endl;
							if((return_events[i].events & EPOLLET))	
								cout<<"EPOLLET"<<endl;
							//cout<<"errno"<<errno<<endl;
							//cout<<"sgw_fd"<<sgw_fd<<endl;
							*/
			repevent = return_events[i];
			if(repevent.data.fd == ran_listen_fd)
			{
				//CASE 0;  RAN--<Accept>-->MME
				while(1)
				{

					ran_accept_fd = accept(ran_listen_fd, NULL, NULL);
					if(ran_accept_fd < 0)
					{
						if((errno == EAGAIN) ||
							(errno == EWOULDBLOCK))
						{
							break;
						}
						else
						{
							cout<<"Error on accept"<<endl;
							break;
							//exit(-1);
						}
					}

				
					epevent.data.fd = ran_accept_fd;
					epevent.events = EPOLLIN | EPOLLET;
					retval = epoll_ctl( epollfd, EPOLL_CTL_ADD, ran_accept_fd, &epevent);
					if(retval == -1)
					{ 
						TRACE(cout<<"Error: Adding ran accept to epoll"<<endl;)
					}
				
					fddata.act = 1;
					fddata.initial_fd = 0;
					fddata.msui = 0;
					memset(fddata.buf,0,500);
					fddata.buflen = 0;
					fdmap.insert(make_pair(ran_accept_fd,fddata));
					
				}
			}
			else
			{	
				cur_fd = repevent.data.fd;
				fddata = fdmap[cur_fd];
				act_type = fddata.act;
				switch(act_type) {

					case 1:
						//CASE 1; Data from RAN--<data>-->MME
						//TRACE(cout<<"Info: Data Read From RAN"<<endl;)
						pkt.clear_pkt();
						retval = read_stream(cur_fd, pkt.data, sizeof(int));	//network.cpp line 176
						
						if(retval == 0)
						{
							close(cur_fd);
							fdmap.erase(cur_fd);		
							closedconn++;
							TRACE(cout<<"Info:Closing connection :"<<closedconn<<endl;)
							//continue;
						}
						else
						if(retval < 0)
						{
							TRACE(cout<<"Error: Read length of packet in case 1, exit for now"<<endl;)
							exit(-1);
						}
						else
						{
							memmove(&pkt_len, pkt.data, sizeof(int) * sizeof(uint8_t));
							pkt.clear_pkt();
							retval = read_stream(cur_fd, pkt.data, pkt_len);
							pkt.data_ptr = 0;
							pkt.len = retval;
							if(retval == 0)
								{
									cout<<"ERROR TTTHERE ret2 = 0"<<endl;
								}
							else
							if(retval < 0)
							{
								TRACE(cout<<"Error: Packet from Ran Corrupt, breaking:"<<errno<<endl;)
								//lflag = 1;
								//exit(-1);
							}
						

							//Ran->MME |Processing|
							pkt.extract_s1ap_hdr();	  //Switch begins here mme_server.cpp	l:45
							
							if (pkt.s1ap_hdr.mme_s1ap_ue_id == 0) {
							//initial attach: Ran<--->MME<--->HSS
									num_autn_vectors = 1;
								pkt.extract_item(imsi);
								pkt.extract_item(tai);
								pkt.extract_item(ksi_asme); /* No use in this case */
								pkt.extract_item(nw_capability); /* No use in this case */
								//cout<<"IMSI from RAN :"<<imsi<<endl;
								enodeb_s1ap_ue_id = pkt.s1ap_hdr.enodeb_s1ap_ue_id;
								guti = g_telecom.get_guti(mme_ids.gummei, imsi);

								TRACE(cout << "mme_handleinitialattach:" << " initial attach req received: " << guti << endl;)

								//Locks here
								//mlock(s1mmeid_mux);
								ue_count++;
								mme_s1ap_ue_id = ue_count;
								//cout<<mme_s1ap_ue_id<<":"<<ue_count<<endl;
								s1mme_id[mme_s1ap_ue_id] = guti;
								//munlock(s1mmeid_mux);

								//IMPORTANT mme.cpp distributed	l:165
								//Locks here
								//mlock(uectx_mux);
								ue_ctx[guti].init(imsi, enodeb_s1ap_ue_id, mme_s1ap_ue_id, tai, nw_capability);
								nw_type = ue_ctx[guti].nw_type;
								TRACE(cout << "mme_handleinitialattach:" << ":ue entry added: " << guti << endl;)
								//munlock(uectx_mux);

								pkt.clear_pkt();
								pkt.append_item(imsi);
								pkt.append_item(mme_ids.plmn_id);
								pkt.append_item(num_autn_vectors);
								pkt.append_item(nw_type);
								pkt.prepend_diameter_hdr(1, pkt.len);	//mme.cpp line 	146

								//send to HSS now
								//cout<<"Info: Going to connect to hss"<<endl;

								hss_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
								if(hss_fd < 0)
								{
									cout<<"Error: Hss socket open";
									exit(-1);
								}


									bzero((char *) &hss_server_addr, sizeof(hss_server_addr));
									hss_server_addr.sin_family = AF_INET;
									hss_server_addr.sin_addr.s_addr = inet_addr(hss_ip);
									hss_server_addr.sin_port = htons(hss_my_port);
				
								retval = connect(hss_fd, (struct sockaddr*)&hss_server_addr, sizeof(hss_server_addr));
								if((retval == -1) && (errno == EINPROGRESS))
								{
									//Connect to HSS in epoll
									epevent.data.fd = hss_fd;
									epevent.events = EPOLLOUT | EPOLLET;
									returnval = epoll_ctl(epollfd, EPOLL_CTL_ADD, hss_fd, &epevent);
									if(returnval == -1)
									{
										
										cout<<"Error: Adding hss connect to epoll failed"<<endl;
										exit(-1);
									} 
									
									fdmap.erase(cur_fd);
									fddata.act = 2;
									fddata.initial_fd = cur_fd;
									fddata.msui = mme_s1ap_ue_id;
									pkt.prepend_len();
									memcpy(fddata.buf, pkt.data, pkt.len);
									fddata.buflen = pkt.len;	

									fdmap.insert(make_pair(hss_fd,fddata)); 
								
								}
								else
								if(retval == -1)
								{
									TRACE(cout<<"Error: Connect to HSS Failed"<<endl;)
									continue;
									//exit(-1);
								}
								else
								{
									//Connection Established
									//send to hss and wait for response.....
									cout<<"Write HSS immediately, NEVER HERE"<<endl;
									pkt.prepend_len();
									returnval = write_stream(hss_fd, pkt.data, pkt.len);
									if(returnval < 0)
									{
										TRACE(cout<<"Error: Hss send packet"<<endl;)
										exit(-1);
									}

									epevent.data.fd = hss_fd;
									epevent.events = EPOLLIN | EPOLLET;
									returnval = epoll_ctl(epollfd, EPOLL_CTL_ADD, hss_fd, &epevent);
									if(returnval < 0)
									{
										TRACE(cout<<"Error: Epoll add hss read"<<endl;)
										exit(-1);
									}

									fddata.act = 3;
									fddata.initial_fd = cur_fd;
									fddata.buflen = 0;
									memset(fddata.buf,0,500);
									fdmap.insert(make_pair(hss_fd,fddata));
								}
							}	//1st attach  ends here....
							
							else
							if(pkt.s1ap_hdr.msg_type == 2)	//mme_server.cpp	l:62
							{	//Authentication response
								//RAN--->MME--->RAN
								//TRACE(cout << "mmeserver_handleue:" << " case 2: authentication response" << endl;)

								guti = 0;
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
								
								//Locks here
								//mlock(s1mmeid_mux);
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								//munlock(s1mmeid_mux);

								if (guti == 0) {
									cout<<"Error: guit 0 ran_accept_fd case 2"<<endl;
									exit(-1);
								}
								pkt.extract_item(res);
								
								//Locks here
								//mlock(uectx_mux);
								xres = ue_ctx[guti].xres;
								//munlock(uectx_mux);
								
								if (res == xres) {
									TRACE(cout << "mme_handleautn:" << " Authentication successful: " << guti << endl;)
									//cout<<"Info: Auth succ raf case2"<<endl;
									
								

								//set_crypt_context(guti);
								//Locks here
								//mlock(uectx_mux);
								ue_ctx[guti].nas_enc_algo = 1;
								ue_ctx[guti].k_nas_enc = ue_ctx[guti].k_asme + ue_ctx[guti].nas_enc_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
								//munlock(uectx_mux);

								//set_integrity_context(guti);
								
								//Locks here
								//mlock(uectx_mux);
								ue_ctx[guti].nas_int_algo = 1;
								ue_ctx[guti].k_nas_int = ue_ctx[guti].k_asme + ue_ctx[guti].nas_int_algo + ue_ctx[guti].count + ue_ctx[guti].bearer + ue_ctx[guti].dir;
								//munlock(uectx_mux);

								//Locks here
								//mlock(uectx_mux);
								ksi_asme = ue_ctx[guti].ksi_asme;
								nw_capability = ue_ctx[guti].nw_capability;
								nas_enc_algo = ue_ctx[guti].nas_enc_algo;
								nas_int_algo = ue_ctx[guti].nas_int_algo;
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								//munlock(uectx_mux);

								pkt.clear_pkt();
								
								
								pkt.append_item(ksi_asme);
								pkt.append_item(nw_capability);
								pkt.append_item(nas_enc_algo);
								pkt.append_item(nas_int_algo);
								
								if (HMAC_ON) {//Check This;;
									g_integrity.add_hmac(pkt, k_nas_int);
									//add_hmac(pkt, k_nas_int);
								
								}
								//cout<<"Info: Attach 2 done: "<<mme_s1ap_ue_id<<endl;
								pkt.prepend_s1ap_hdr(2, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
								
								pkt.prepend_len();
								
								returnval = write_stream(cur_fd, pkt.data, pkt.len);
								if(returnval < 0)
								{
									cout<<"Error: Auth send to RAN"<<endl;
									exit(-1);
								}
								
								}
								else {
									
									cout<<"\n msui "<<mme_s1ap_ue_id<<" socket "<<cur_fd<<" Failed res: "<<res<<"Failed xres: "<<xres<<endl;
									cout<<"erasing :"<<ecount++<<'\n';
									exit(-1);
								}
							}	//2nd Attach done
							else
							if(pkt.s1ap_hdr.msg_type == 3)//Ran -- MMe --Sgw -- MMe -- Ran
							{
								guti = 0;
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
								
								//Locks Deep
								//g_sync.mlock(s1mmeid_mux);
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								//g_sync.munlock(s1mmeid_mux);
								//	g_sync.mlock(uectx_mux);
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								//	g_sync.munlock(uectx_mux);

								if (HMAC_ON) {
									res = g_integrity.hmac_check(pkt, k_nas_int);
									if (res == false) {
										cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;
										//cout<<"Error: Hmac failed 3rd attach "<<endl;
										exit(-1);
									}		
								}
								if (ENC_ON) {
									g_crypt.dec(pkt, k_nas_enc);
								}
								pkt.extract_item(res);
								if (res == false) {
									cout << "mme_handlesecuritymodecomplete:" << " security mode complete failure: " << guti << endl;
									//cout<<"Error: res failed at 3rd attach"<<endl;
									exit(-1);
								}
								//else {
									//TRACE(cout << "mme_handlesecuritymodecomplete:" << " security mode complete success: " << guti << endl;)
									//Success, proceed to UDP
								//}
								
								eps_bearer_id = 5;	//mme.cpp	l:353
								//Locks Here
								//set_pgw_info(guti);
								ue_ctx[guti].pgw_s5_port = g_pgw_s5_port;
								ue_ctx[guti].pgw_s5_ip_addr = g_pgw_s5_ip_addr;
								//g_sync.mlock(uectx_mux);
								//ue_ctx[guti].s11_cteid_mme = get_s11cteidmme(guti);
								tem = to_string(guti);
								tem = tem.substr(7, -1); /* Extracting only the last 9 digits of UE MSISDN */
								s11_cteid_mme = stoull(tem);
								ue_ctx[guti].s11_cteid_mme = s11_cteid_mme;

								ue_ctx[guti].eps_bearer_id = eps_bearer_id;
								//s11_cteid_mme = ue_ctx[guti].s11_cteid_mme;
								imsi = ue_ctx[guti].imsi;
								eps_bearer_id = ue_ctx[guti].eps_bearer_id;
								pgw_s5_ip_addr = ue_ctx[guti].pgw_s5_ip_addr;
								pgw_s5_port = ue_ctx[guti].pgw_s5_port;
								ue_ctx[guti].apn_in_use = 0;	//Added by trishal
								apn_in_use = ue_ctx[guti].apn_in_use;
								tai = ue_ctx[guti].tai;
								//g_sync.munlock(uectx_mux);

								pkt.clear_pkt();
								pkt.append_item(s11_cteid_mme);
								pkt.append_item(imsi);
								pkt.append_item(eps_bearer_id);
								pkt.append_item(pgw_s5_ip_addr);
								pkt.append_item(pgw_s5_port);
								pkt.append_item(apn_in_use);
								pkt.append_item(tai);
								pkt.prepend_gtp_hdr(2, 1, pkt.len, 0);

								//send to sgw
								
								sgw_fd = socket(PF_INET, SOCK_DGRAM, 0);
								
								if(sgw_fd < 0)
								{
									cout<<"Error sgw socket"<<endl;
									exit(-1);
								}

								uflag = make_socket_nb(sgw_fd);
								if(uflag < 0)
								{
									cout<<"Error sgw make NB"<<endl;
									exit(-1);
								}
		
						        int uflag = 1;
						        if (setsockopt(sgw_fd, SOL_SOCKET, SO_REUSEADDR, &uflag, sizeof(uflag)) < 0)
						        {
						                cout<<"Error : sgw setsockopt reuse"<<endl;
						                exit(-2);
						        }
								
						        udp_attach_port = udp_attach_free_ports.front();
						        udp_attach_free_ports.pop();
								
								memset((char *) &udp_sock_bind, 0 ,sizeof(udp_sock_bind));
						        udp_sock_bind.sin_family = AF_INET;
	        					udp_sock_bind.sin_port = htons(udp_attach_port);
	        					udp_sock_bind.sin_addr.s_addr = inet_addr(mme_ip);
	        					memset(udp_sock_bind.sin_zero, '\0', sizeof(udp_sock_bind.sin_zero));
	        					
	        					if (bind(sgw_fd, (struct sockaddr *) &udp_sock_bind, sizeof(udp_sock_bind)) < 0)
	        					{
	                				cout<<"Error: UDP attach bind error"<<endl;
	                				exit(-1);
						        }

						        udp_attach_use_ports.insert(udp_attach_port);
						        
								//continue 377
						        while(1){
									retval = sendto( sgw_fd, pkt.data, pkt.len, 0, (sockaddr*)&sgw_addr, sgwsocklen);
									if(errno < 0)
									cout<<"Errno "<<errno<<endl;
									if(retval < 0)
										cout<<"UDP Send Error"<<retval<<endl;
									if(errno == EPERM)
									{
										cout<<"error : EPERM"<<endl;
										errno = 0;
										continue;
									}
									else
									{
										break;
									}
								}

								
								epevent.data.fd = sgw_fd;
								epevent.events = EPOLLIN | EPOLLET;
								returnval = epoll_ctl(epollfd, EPOLL_CTL_ADD, sgw_fd, &epevent);
								if(returnval < 0)
								{
									TRACE(cout<<"Error: Epoll add sgw read"<<endl;)
									cout<<"ret"<<returnval<<":"<<errno<<endl;
									exit(-1);
								}
								fddata.act = 4;
								fddata.initial_fd = cur_fd;
								fddata.msui = mme_s1ap_ue_id;
								
								memset(fddata.buf,0,500);
								memcpy(fddata.buf, pkt.data, pkt.len);
								fddata.buflen = pkt.len;
								fdmap.insert(make_pair(sgw_fd,fddata));



								if(retval < 0)
								{
									TRACE(cout<<"Error: Sgw send packet"<<endl;)
									exit(-1);
								}
								//cout<<"SGW FD: "<<sgw_fd<<": curfd "<<cur_fd<<endl;
								TRACE(cout<<"Attach3: sent to sgw"<<endl;)
								//TRACEM(cout<<"Info: Sent to sgw Waiting for sgw_fd:"<<sgw_fd<<endl;)
								//	*/

								//Editeid
							

							}	//3rd attach Done
							else
							if(pkt.s1ap_hdr.msg_type == 4)
							{	
								//4th attach RAN
								guti = 0;		//mme.cpp	l:438
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
								
								//Locks Deep
								//g_sync.mlock(s1mmeid_mux);
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								//g_sync.munlock(s1mmeid_mux);
								//	g_sync.mlock(uectx_mux);
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								//	g_sync.munlock(uectx_mux);

								if (HMAC_ON) {
									res = g_integrity.hmac_check(pkt, k_nas_int);
									if (res == false) {
										TRACE(cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;)
										//cout<<"Error: Hmac failed 3rd attach "<<endl;
										exit(-1);
									}		
								}
								if (ENC_ON) {
									g_crypt.dec(pkt, k_nas_enc);
								}

								pkt.extract_item(eps_bearer_id);
								pkt.extract_item(s1_uteid_dl);
								g_sync.mlock(uectx_mux);
								ue_ctx[guti].s1_uteid_dl = s1_uteid_dl;
								ue_ctx[guti].emm_state = 1;
								g_sync.munlock(uectx_mux);
								TRACE(cout << "mme_handleattachcomplete:" << " attach completed: " << guti << endl;)

								//Locks here
								//g_sync.mlock(uectx_mux);
								eps_bearer_id = ue_ctx[guti].eps_bearer_id;
								s1_uteid_dl = ue_ctx[guti].s1_uteid_dl;
								s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
								//g_sync.munlock(uectx_mux);

								// add to close connection...
								//close(cur_fd);
								//cout<<"closed conn cur_fd;"<<endl;
								//if(close(cur_fd) < 0)
								//{
								//	cout<<"error on close"<<endl;
								//}
								/*
								fdmap.erase(cur_fd);
								fddata.act = 1;
								fddata.initial_fd = 0;	//not needed;
								fddata.msui = 0;//mme_s1ap_ue_id;
								memset(fddata.buf,0,500);
								fddata.buflen = 0;//pkt.len;
								//cout<<"bf4"<<fdmap.size()<<endl;

								fdmap.insert(make_pair(cur_fd,fddata));
								//cout<<"aft"<<fdmap.size()<<endl;
								*/
								
								//MME-SGW
								pkt.clear_pkt();
								pkt.append_item(eps_bearer_id);
								pkt.append_item(s1_uteid_dl);
								pkt.append_item(g_trafmon_ip_addr);
								pkt.append_item(g_trafmon_port);
								pkt.prepend_gtp_hdr(2, 2, pkt.len, s11_cteid_sgw);

								//Begin udp sendto...
								//mme.cpp l:504

								sgw_fd = socket(PF_INET, SOCK_DGRAM, 0);
								
								if(sgw_fd < 0)
								{
									cout<<"Error sgw socket"<<endl;
									exit(-1);
								}

								uflag = make_socket_nb(sgw_fd);
								if(uflag < 0)
								{
									cout<<"Error sgw make NB"<<endl;
									exit(-1);
								}



						        int uflag = 1;
						        if (setsockopt(sgw_fd, SOL_SOCKET, SO_REUSEADDR, &uflag, sizeof(uflag)) < 0)
						        {
						                cout<<"Error : sgw setsockopt reuse"<<endl;
						                exit(-2);
						        }
								

								udp_afour_port = udp_afour_free_ports.front();
								udp_afour_free_ports.pop();
								 
						        memset((char *) &udp_sock_bind, 0 ,sizeof(udp_sock_bind));
						        udp_sock_bind.sin_family = AF_INET;
	        					udp_sock_bind.sin_port = htons(udp_afour_port);
	        					udp_sock_bind.sin_addr.s_addr = inet_addr(mme_ip);
	        					memset(udp_sock_bind.sin_zero, '\0', sizeof(udp_sock_bind.sin_zero));
	        					
	        					if (bind(sgw_fd, (struct sockaddr *) &udp_sock_bind, sizeof(udp_sock_bind)) < 0)
	        					{
	                				cout<<"Error: UDP attach four bind error"<<endl;
	                				exit(-1);
						        }

						        
						        udp_afour_use_ports.insert(udp_afour_port);
								//continue 377
						        while(1){
									retval = sendto( sgw_fd, pkt.data, pkt.len, 0, (sockaddr*)&sgw_addr, sgwsocklen);
									if(errno < 0)
									cout<<"Errno "<<errno<<endl;
									if(retval < 0)
										cout<<"UDP Send a4 Error"<<retval<<endl;
									if(errno == EPERM)
									{
										cout<<"error : EPERM"<<endl;
										errno = 0;
										continue;
									}
									else
									{
										break;
									}
								}
								if(retval < 0)
								{
									cout<<"Error: Sgw send packet"<<endl;
									exit(-1);
								}
								
								epevent.data.fd = sgw_fd;
								epevent.events = EPOLLIN | EPOLLET;
								returnval = epoll_ctl(epollfd, EPOLL_CTL_ADD, sgw_fd, &epevent);
								if(returnval < 0)
								{
									cout<<"Error: Epoll add sgw a4 read"<<endl;
									cout<<"ret"<<returnval<<":"<<errno<<endl;
									exit(-1);
								}
								fddata.act = 5;
								fddata.initial_fd = cur_fd;	//not needed;
								fddata.msui = mme_s1ap_ue_id;
								memset(fddata.buf,0,500);
								//memcpy(fddata.buf, pkt.data, pkt.len);
								fddata.buflen = 0;//pkt.len;
								fdmap.insert(make_pair(sgw_fd,fddata));
								//fdmap.erase(cur_fd);
								TRACE(cout<<"Attach 4 : sent to sgw"<<endl;)
								
								}	//4th attach Done
								else
								if(pkt.s1ap_hdr.msg_type == 5)
								{	
								//Detach request
								guti = 0;		//mme.cpp	l:438
								mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
								
								//Locks Deep
								//g_sync.mlock(s1mmeid_mux);
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
								//g_sync.munlock(s1mmeid_mux);
								//	g_sync.mlock(uectx_mux);
								k_nas_enc = ue_ctx[guti].k_nas_enc;
								k_nas_int = ue_ctx[guti].k_nas_int;
								eps_bearer_id = ue_ctx[guti].eps_bearer_id;
								tai = ue_ctx[guti].tai;
								s11_cteid_sgw = ue_ctx[guti].s11_cteid_sgw;
								//	g_sync.munlock(uectx_mux);

								if (HMAC_ON) {
									res = g_integrity.hmac_check(pkt, k_nas_int);
									if (res == false) {
										cout << "mme_handlesecuritymodecomplete:" << " hmac failure: " << guti << endl;
										//cout<<"Error: Hmac failed 3rd attach "<<endl;
										exit(-1);
									}		
								}
								if (ENC_ON) {
									g_crypt.dec(pkt, k_nas_enc);
								}

								pkt.extract_item(guti); //Check Same???	mme.cpp-l558
								pkt.extract_item(ksi_asme);
								pkt.extract_item(detach_type);	
								
								
								//MME-SGW
								pkt.clear_pkt();
								pkt.append_item(eps_bearer_id);
								pkt.append_item(tai);
								pkt.prepend_gtp_hdr(2, 3, pkt.len, s11_cteid_sgw);

								//Begin udp sendto...
								//mme.cpp l:569

								sgw_fd = socket(PF_INET, SOCK_DGRAM, 0);
								
								if(sgw_fd < 0)
								{
									cout<<"Error sgw socket"<<endl;
									exit(-1);
								}

								uflag = make_socket_nb(sgw_fd);
								if(uflag < 0)
								{
									cout<<"Error sgw make NB"<<endl;
									exit(-1);
								}



						        int uflag = 1;
						        if (setsockopt(sgw_fd, SOL_SOCKET, SO_REUSEADDR, &uflag, sizeof(uflag)) < 0)
						        {
						                cout<<"Error : sgw setsockopt reuse"<<endl;
						                exit(-2);
						        }
								

								udp_detach_port = udp_detach_free_ports.front();
								udp_detach_free_ports.pop();
								 
						        memset((char *) &udp_sock_bind, 0 ,sizeof(udp_sock_bind));
						        udp_sock_bind.sin_family = AF_INET;
	        					udp_sock_bind.sin_port = htons(udp_detach_port);
	        					udp_sock_bind.sin_addr.s_addr = inet_addr(mme_ip);
	        					memset(udp_sock_bind.sin_zero, '\0', sizeof(udp_sock_bind.sin_zero));
	        					
	        					if (bind(sgw_fd, (struct sockaddr *) &udp_sock_bind, sizeof(udp_sock_bind)) < 0)
	        					{
	                				cout<<"Error: UDP attach four bind error"<<endl;
	                				exit(-1);
						        }

						        
						        udp_detach_use_ports.insert(udp_detach_port);
								//continue 377
						        while(1){
									retval = sendto( sgw_fd, pkt.data, pkt.len, 0, (sockaddr*)&sgw_addr, sgwsocklen);
									if(errno < 0)
									cout<<"Errno "<<errno<<endl;
									if(retval < 0)
										cout<<"UDP Send a4 Error"<<retval<<endl;
									if(errno == EPERM)
									{
										cout<<"error : EPERM"<<endl;
										errno = 0;
										continue;
									}
									else
									{
										break;
									}
								}
								if(retval < 0)
								{
									cout<<"Error: Sgw send packet"<<endl;
									exit(-1);
								}
								
								epevent.data.fd = sgw_fd;
								epevent.events = EPOLLIN | EPOLLET;
								returnval = epoll_ctl(epollfd, EPOLL_CTL_ADD, sgw_fd, &epevent);
								if(returnval < 0)
								{
									cout<<"Error: Epoll add sgw a4 read"<<endl;
									cout<<"ret"<<returnval<<":"<<errno<<endl;
									exit(-1);
								}
								fddata.act = 6;
								fddata.initial_fd = cur_fd;	//not needed;
								fddata.msui = mme_s1ap_ue_id;
								memset(fddata.buf,0,500);
								fddata.buflen = 0;//pkt.len;
								fdmap.insert(make_pair(sgw_fd,fddata));
								TRACE(cout<<"Detach: Sent to sgw"<<endl;)
								}	//Detach Done
								else
								{
									cout<<"error s1ap_hdr.mme_s1ap_ue_id val read"<<endl; 
								}
								
							}
							break;

					case 2:
						
						if(return_events[i].events & EPOLLOUT)
						{	// attach 1  mme->hss accept in delay
							err = 0;
							len = sizeof(int);
							returnval = getsockopt(cur_fd, SOL_SOCKET, SO_ERROR, &err, &len);
							if( (returnval != -1) && (err == 0))	// Conn estd;
							{
								//pkt.prepend_len();	//Added Later.
								returnval = write_stream(cur_fd, fddata.buf, fddata.buflen);
								if(returnval < 0)
								{
									TRACE(cout<<"Error: Hss data not sent after accept"<<endl;)
									exit(-1);
								}
								
								/*	NOT	MOVED TO CASE 1								
								pc.extract_diameter_hdr();
								pc.extract_item(imsi);
								*/
								TRACE(cout<<"Info: Data send to Hss after connect :"<<imsi<<endl;)

								epevent.data.fd = cur_fd;
								epevent.events = EPOLLIN | EPOLLET;
								returnval = epoll_ctl(epollfd, EPOLL_CTL_MOD, cur_fd, &epevent);
								if(returnval == -1)
								{
									cout<<"Error: Adding Epoll MOD hss rcv"<<endl;
									exit(-1);
								}

								//cout<<"At 2 : "<<fddata.msui<<endl;
								fdmap.erase(cur_fd);
								fddata.act = 3;
								fddata.buflen = 0;
								memset(fddata.buf,0,500);
								//fddata.pktcp = NULL;
								fdmap.insert(make_pair(cur_fd, fddata));
							}
							else
							{
								cout<<"Error: Accept after connect Failed "<<endl;
								exit(-1);
							}
						}
						else
						{
							//NOt Epollout???
							cout<<"Error: Not Epoll Out in case 2"<<endl;
							exit(-1);
						}

						break;

					case 3:
						//Response from HSS 		mme.cpp 150
						pkt.clear_pkt();
						retval = read_stream(cur_fd, pkt.data, sizeof(int));
						if(retval < 0)
						{
							cout<<"Error: Read pkt len case 3, exit for now"<<endl;
							exit(-1);
						}
						else
						{
							memmove(&pkt_len, pkt.data, sizeof(int) * sizeof(uint8_t));
							pkt.clear_pkt();
							retval = read_stream(cur_fd, pkt.data, pkt_len);
							pkt.data_ptr = 0;
							pkt.len = retval;
							if(retval < 0)
							{
								cout<<"Error: Packet from HSS Corrupt, break"<<endl;
								lflag = 1;
								break;
							}
						}						
						close(cur_fd);

						pkt.extract_diameter_hdr();
						pkt.extract_item(autn_num);
						pkt.extract_item(rand_num);
						pkt.extract_item(xres);
						pkt.extract_item(k_asme);

						//We r Storing msui, from msui get guti and esui...	
						mme_s1ap_ue_id = fddata.msui;
						//cout<<"Stored msui"<<fddata.msui<<" hss "<<cur_fd<<" xres "<<xres<<endl;
						
						//mlock(s1mmeid_mux);
							if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
								guti = s1mme_id[mme_s1ap_ue_id];
							}
						//munlock(s1mmeid_mux);
						
						//Locks here


						//mlock(uectx_mux);
						ue_ctx[guti].xres = xres;
						ue_ctx[guti].k_asme = k_asme;
						ue_ctx[guti].ksi_asme = 1;
						ksi_asme = ue_ctx[guti].ksi_asme;
						enodeb_s1ap_ue_id = ue_ctx[guti].enodeb_s1ap_ue_id;
						//munlock(uectx_mux);

						TRACE(cout << "Info:mme_handleinitialattach:" << " autn:" << autn_num <<" rand:" << rand_num << " xres:" << xres << " k_asme:" << k_asme << " " << guti << endl;)

						pkt.clear_pkt();
						pkt.append_item(autn_num);
						pkt.append_item(rand_num);
						pkt.append_item(ksi_asme);
						//cout<<"mme->ran"<<mme_s1ap_ue_id<<endl;
						pkt.prepend_s1ap_hdr(1, pkt.len, enodeb_s1ap_ue_id, mme_s1ap_ue_id);
						//server.snd(conn_fd, pkt);

						ran_accept_fd = fddata.initial_fd;
								
						pkt.prepend_len();
						returnval = write_stream(ran_accept_fd, pkt.data, pkt.len);
						if(returnval < 0)
						{
							cout<<"Error: Cant send to RAN"<<endl;
							exit(-1);
						}
						//TRACE(cout<<"Info: Data send to RAN, Woo!! :"<<guti<<endl;)
						//close(ran_accept_fd);
						fdmap.erase(cur_fd);
						fdmap.erase(ran_accept_fd);
						
						fddata.act = 1;
						fddata.initial_fd = 0; 
						memset(fddata.buf,0,500);
						fddata.buflen = 0;
						fdmap.insert(make_pair(ran_accept_fd,fddata));
						
						break;

					case 4:
						// sgw -> mme -> ran
						//need to restore guti
						
						if((return_events[i].events & EPOLLIN))
						{
								
							mme_s1ap_ue_id = fddata.msui;
							if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
								guti = s1mme_id[mme_s1ap_ue_id];
							}
										
							pkt.clear_pkt();

							retval = recvfrom(cur_fd, pkt.data, BUF_SIZE, 0, (sockaddr*)&sgw_addr, &sgwsocklen);
							if(retval < 0)
							{								
								cout<<"***Error data rcv from sgw";
								cout<<"sock: "<<cur_fd<<"ret: "<<retval<<": err "<<errno<<endl;
								
								exit(-1);
							}

							if (getsockname(cur_fd, (struct sockaddr *)&udp_sock_bind, &sgwsocklen) == -1)
	    					{
	    						cout<<"Error: getsockname error "<<errno<<endl;
								exit(-1);
							}
							else	
	    						udp_attach_port = ntohs(udp_sock_bind.sin_port);
	    					//cout<<"At 3"<<udp_attach_port<<endl;
							close(cur_fd);
							udp_attach_use_ports.erase(udp_attach_port);
							udp_attach_free_ports.push(udp_attach_port);

							ran_accept_fd = fddata.initial_fd;
							pkt.data_ptr = 0;	//from udp_client.cpp	l:68
							pkt.len = retval;

							pkt.extract_gtp_hdr();
							pkt.extract_item(s11_cteid_sgw);
							pkt.extract_item(ue_ip_addr);
							pkt.extract_item(s1_uteid_ul);
							pkt.extract_item(s5_uteid_ul);
							pkt.extract_item(s5_uteid_dl);

							//Locks here
							//g_sync.mlock(uectx_mux);
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
							//g_sync.munlock(uectx_mux);

							epsres = true;
							tai_list_size = 1;
							
							pkt.clear_pkt();
							//pkt.data_ptr = 0;
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
							//headers not verified at client side...
							//server.snd(conn_fd, pkt);
							//l:434	mme.cpp
							pkt.prepend_len();
							returnval = write_stream(ran_accept_fd, pkt.data, pkt.len);
							//char buf2[] = "hello";
							//returnval = write(cur_fd, buf2, 6);
							if(returnval < 0)
							{
								cout<<"Error: Cant send to RAN"<<endl;
								if(errno == EPERM)
									cout<<"eper,"<<endl;
								cout<<errno<<endl;
								cout<<"msg :"<<to_string(errno)<<endl;
								exit(-1);
							}
							//cout<<"Sent: guti: "<<guti<<" res :"<<res<<endl;
							TRACE(cout<<"Info Attach 3: Data send to RAN:"<<guti<<endl;)
							//close(ran_accept_fd);
							
							fdmap.erase(cur_fd);

							//close(ran_accept_fd);
							
							fdmap.erase(ran_accept_fd);
							
							
							fddata.act = 1;
							fddata.initial_fd = 0; 
							memset(fddata.buf,0,500);
							fddata.msui = 0;
							fddata.buflen = 0;
							fdmap.insert(make_pair(ran_accept_fd,fddata));
							
							//TRACEM(cout<<"Info: Sent to RAN from... "<<cur_fd<<endl;)
						}
						else
						{
							cout<<"\n\nError Case 4: udp sgw Wrong Event"<<endl;
							if((return_events[i].events & EPOLLIN))	
								cout<<"EPOLLIN"<<endl;
							if((return_events[i].events & EPOLLERR))	
								cout<<"EPOLLERR"<<endl;
							if((return_events[i].events & EPOLLHUP))	
								cout<<"EPOLLHUP"<<endl;
							if((return_events[i].events & EPOLLONESHOT))	
								cout<<"EPOLLONESHOT"<<endl;
							if((return_events[i].events & EPOLLOUT))	
								cout<<"EPOLLOUT"<<endl;
							if((return_events[i].events & EPOLLPRI))	
								cout<<"EPOLLPRI"<<endl;
							if((return_events[i].events & EPOLLET))	
								cout<<"EPOLLET"<<endl;
							cout<<"errno"<<errno<<endl;
							cout<<"sgw_fd"<<sgw_fd<<endl;

						}

						break;

					case 5:		
							if((return_events[i].events & EPOLLIN))
							{
								//Attach 4 Sgw->MME
								mme_s1ap_ue_id = fddata.msui;
								//ran_accept_fd = fddata.initial_fd;
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
											
								pkt.clear_pkt();

								retval = recvfrom(cur_fd, pkt.data, BUF_SIZE, 0, (sockaddr*)&sgw_addr, &sgwsocklen);
								if(retval < 0)
								{								
									cout<<"***Error data rcv from sgw";
									cout<<"sock: "<<cur_fd<<"ret: "<<retval<<": err "<<errno<<endl;
									exit(-1);
								}

								if (getsockname(cur_fd, (struct sockaddr *)&udp_sock_bind, &sgwsocklen) == -1)
		    					{
		    						cout<<"Error: getsockname error "<<errno<<endl;
									exit(-1);
								}
								else	
		    						udp_afour_port = ntohs(udp_sock_bind.sin_port);
		    					//cout<<"At 4"<<udp_afour_port<<endl;

								
								
								udp_afour_use_ports.erase(udp_afour_port);
								udp_afour_free_ports.push(udp_afour_port);


								pkt.data_ptr = 0;	//from udp_client.cpp	l:68
								pkt.len = retval;

								pkt.extract_gtp_hdr();
								pkt.extract_item(res);
								if (res == false) {
									cout << "mme_handlemodifybearer:" << " modify bearer failure: " << guti << endl;
								}
								else {
									//g_sync.mlock(uectx_mux);
									ue_ctx[guti].ecm_state = 1;
									nw_capability = ue_ctx[guti].nw_capability;
									k_nas_enc = ue_ctx[guti].k_nas_enc;
									k_nas_int = ue_ctx[guti].k_nas_int;
									//g_sync.munlock(uectx_mux);
									TRACE(cout << "mme_handlemodifybearer:" << " eps session setup success: " << guti << endl;)
									//cout << "mme_handlemodifybearer:" << " eps session setup success: " << guti << endl;
								}
								
								//closedconn++;	
								
								ran_accept_fd = fddata.initial_fd;

								//Added by Trishal
								
								if (getpeername(ran_accept_fd, (struct sockaddr *)&udp_sock_bind, &sgwsocklen) == -1)
		    					{
		    						cout<<"Error: getsockname ran  error "<<errno<<endl;
									exit(-1);
								}
								else	
		    						udp_afour_port = ntohs(udp_sock_bind.sin_port);


								epsres = true;
								//cout<<"sending "<<guti<<" port "<<udp_afour_port<<endl;
								pkt.clear_pkt();
								//pkt.data_ptr = 0;
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
								//server.snd(conn_fd, pkt);
								//headers not verified at client side...
								pkt.prepend_len();
								returnval = write_stream(ran_accept_fd, pkt.data, pkt.len);
								//char buf2[] = "hello";
								//returnval = write(cur_fd, buf2, 6);
								if(returnval < 0)
								{
									cout<<"Error: Cant send to RAN"<<endl;
									if(errno == EPERM)
										cout<<"eper,"<<endl;
									cout<<errno<<endl;
									cout<<"msg :"<<to_string(errno)<<endl;
									exit(-1);
								}
								
								
								//cout<<" 	sent"<<endl;
								close(cur_fd);
								fdmap.erase(cur_fd);
								//close(ran_accept_fd);
								
								fdmap.erase(ran_accept_fd);
								
								
								fddata.act = 1;
								fddata.initial_fd = 0; 
								memset(fddata.buf,0,500);
								fddata.msui = 0;
								fddata.buflen = 0;
								fdmap.insert(make_pair(ran_accept_fd,fddata));
									//attf++;
									//fdmap.erase(ran_accept_fd);
									//if(attf != attt)
									//cout<<"At 3 "<<attt<<" At 4 "<<attf<<" numev "<<numevents<<endl;
								}
							else
							{
								cout<<"Wrong Event a4 rcv !!"<<endl;
								exit(-1);
							}	

						break;

						case 6:		
							if((return_events[i].events & EPOLLIN))
							{
								//Detach Sgw->MME->Ran
								mme_s1ap_ue_id = fddata.msui;
								//ran_accept_fd = fddata.initial_fd;
								if (s1mme_id.find(mme_s1ap_ue_id) != s1mme_id.end()) {
									guti = s1mme_id[mme_s1ap_ue_id];
								}
											
								pkt.clear_pkt();

								retval = recvfrom(cur_fd, pkt.data, BUF_SIZE, 0, (sockaddr*)&sgw_addr, &sgwsocklen);
								if(retval < 0)
								{								
									cout<<"***Error data rcv from sgw";
									cout<<"sock: "<<cur_fd<<"ret: "<<retval<<": err "<<errno<<endl;
									exit(-1);
								}

								if (getsockname(cur_fd, (struct sockaddr *)&udp_sock_bind, &sgwsocklen) == -1)
		    					{
		    						cout<<"Error: getsockname error "<<errno<<endl;
									exit(-1);
								}
								else	
		    						udp_detach_port = ntohs(udp_sock_bind.sin_port);
		    					//cout<<"At 4"<<udp_afour_port<<endl;

								
								
								udp_detach_use_ports.erase(udp_detach_port);
								udp_detach_free_ports.push(udp_detach_port);


								pkt.data_ptr = 0;	//from udp_client.cpp	l:68
								pkt.len = retval;

								pkt.extract_gtp_hdr();
								pkt.extract_item(res);
								if (res == false) {
									cout << "mme_handlemodifybearer:" << " modify bearer failure: " << guti << endl;
								}
								else {
									//g_sync.mlock(uectx_mux);
									//ue_ctx[guti].ecm_state = 1;
									//nw_capability = ue_ctx[guti].nw_capability;
									k_nas_enc = ue_ctx[guti].k_nas_enc;
									k_nas_int = ue_ctx[guti].k_nas_int;
									//g_sync.munlock(uectx_mux);
									//TRACE(cout << "mme_handlemodifybearer:" << " eps session setup success: " << guti << endl;		)
									//cout << "mme_handlemodifybearer:" << " eps session setup success: " << guti << endl;
								}
								
								//closedconn++;	
								
								ran_accept_fd = fddata.initial_fd;

								//Added by Trishal
								/*
								if (getpeername(ran_accept_fd, (struct sockaddr *)&udp_sock_bind, &sgwsocklen) == -1)
		    					{
		    						cout<<"Error: getsockname ran  error "<<errno<<endl;
									exit(-1);
								}
								else	
		    						udp_afour_port = ntohs(udp_sock_bind.sin_port);

								*/
								epsres = true;
								//cout<<"sending "<<guti<<" port "<<udp_detach_port<<endl;
								pkt.clear_pkt();
								//pkt.data_ptr = 0;
								pkt.append_item(epsres);
								//pkt.append_item(guti);
								//pkt.append_item(nw_capability);
								if (ENC_ON) {
									g_crypt.enc(pkt, k_nas_enc);
								}
								if (HMAC_ON) {
									g_integrity.add_hmac(pkt, k_nas_int);
								}
								pkt.prepend_s1ap_hdr(5, pkt.len, pkt.s1ap_hdr.enodeb_s1ap_ue_id, pkt.s1ap_hdr.mme_s1ap_ue_id);
								//server.snd(conn_fd, pkt);
								//headers not verified at client side...
								pkt.prepend_len();
								returnval = write_stream(ran_accept_fd, pkt.data, pkt.len);
								if(returnval < 0)
								{
									cout<<"Error: Cant send to RAN"<<endl;
									if(errno == EPERM)
										cout<<"eper,"<<endl;
									cout<<errno<<endl;
									cout<<"msg :"<<to_string(errno)<<endl;
									exit(-1);
								}
								TRACE(cout<<"Info:Done detach to RAN"<<endl;)
								
								//cout<<" 	sent"<<endl;
								close(cur_fd);
								//mme.cpp l:591, mme.cpp l:618-624
								//g_sync.mlock(s1mmeid_mux);
								s1mme_id.erase(mme_s1ap_ue_id);
								//g_sync.munlock(s1mmeid_mux);

								//g_sync.mlock(uectx_mux);
								ue_ctx.erase(guti);
								//g_sync.munlock(uectx_mux);
								
								fdmap.erase(cur_fd);
								//close(ran_accept_fd);
								
								fdmap.erase(ran_accept_fd);
								
								
								fddata.act = 1;
								fddata.initial_fd = 0; 
								memset(fddata.buf,0,500);
								fddata.msui = 0;
								fddata.buflen = 0;
								fdmap.insert(make_pair(ran_accept_fd,fddata));
									//attf++;
									//fdmap.erase(ran_accept_fd);
									//if(attf != attt)
									//cout<<"At 3 "<<attt<<" At 4 "<<attf<<" numev "<<numevents<<endl;
								}
							else
							{
								cout<<"Wrong Event at detach rcv !!"<<endl;
								exit(-1);
							}	

						break;
					default:
						TRACE(cout<<"Error: Switch Packet Condition not known??"<<cur_fd<<endl;)
						break;
				}
			}



		}
	}

}




int main()
{
	run();
	return 0;
}



int read_stream(int conn_fd, uint8_t *buf, int len) {
	int ptr;
	int retval;
	int read_bytes;
	int remaining_bytes;

	ptr = 0;
	remaining_bytes = len;
	if (conn_fd < 0 || len <= 0) {
		return -1;
	}
	while (1) {
		read_bytes = read(conn_fd, buf + ptr, remaining_bytes);
		if (read_bytes <= 0) {
			retval = read_bytes;
			break;
		}
		ptr += read_bytes;
		remaining_bytes -= read_bytes;
		if (remaining_bytes == 0) {
			retval = len;
			break;
		}
	}
	return retval;
}

int write_stream(int conn_fd, uint8_t *buf, int len) {
	int ptr;
	int retval;
	int written_bytes;
	int remaining_bytes;

	ptr = 0;
	remaining_bytes = len;
	if (conn_fd < 0 || len <= 0) {
		return -1;
	}	
	while (1) {
		written_bytes = write(conn_fd, buf + ptr, remaining_bytes);
		if (written_bytes <= 0) {
			retval = written_bytes;
			break;
		}
		ptr += written_bytes;
		remaining_bytes -= written_bytes;
		if (remaining_bytes == 0) {
			retval = len;
			break;
		}
	}
	return retval;
}