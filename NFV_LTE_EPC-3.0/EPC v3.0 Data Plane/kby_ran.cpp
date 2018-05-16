#include "kby_ran.h"


int done = 0;
mctx_t mctx; /*mtcp context*/

struct mdata
{
  
    uint8_t buf[500];
    int len;

};

map<int, class Ran> fdmap;
map<int, mdata> fdmap1;
int ep;





int sock_sink1[no_of_connections];

void CloseConnection()
{

        for(int i=0;i<no_of_connections;i++)
        {
                mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sock_sink1[i], NULL);
                mtcp_close(mctx, sock_sink1[i]);
        }

        done = 1;

}


void
SignalHandler(int signum)
{

  mtcp_destroy_context(mctx); 
  mtcp_destroy();
  cout << "remove mtcp\n";
  done = 1;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv){


  int x = no_of_connections;
  int core = 0;
  int ret = -1;

  char* hostname = mme_ip;
  int portno = mme_my_port;

  char* conf_file = "server.conf";
  int sockid;
  int sock_store[no_of_connections];
  

    /* initialize mtcp */
  if (conf_file == NULL) {
    TRACE_CONFIG("You forgot to pass the mTCP startup config file!\n");
    exit(EXIT_FAILURE);
  }

  //step 1. mtcp_init, mtcp_register_signal(optional)
  ret = mtcp_init(conf_file);
  if (ret) {
    TRACE_CONFIG("Failed to initialize mtcp\n");
    exit(EXIT_FAILURE);
  }
  
  /* register signal handler to mtcp */
  mtcp_register_signal(SIGINT, SignalHandler);

  TRACE_INFO("Application initialization finished.\n");
  
  //step 2. mtcp_core_affinitize
  mtcp_core_affinitize(core);
  
  //step 3. mtcp_create_context. Here order of affinitization and context creation matters.
  // mtcp_epoll_create
  
  mctx = mtcp_create_context(core);
  if (!mctx) {
    TRACE_ERROR("Failed to create mtcp context!\n");
  }

  /* create epoll descriptor */
  ep = mtcp_epoll_create(mctx, MAX_EVENTS);
  if (ep < 0) {
    TRACE_ERROR("Failed to create epoll descriptor!\n");
  }

  
  struct mtcp_epoll_event ev;

  
  for(int k=0;k<no_of_connections;k++)
  {

    	//step 4. mtcp_socket, mtcp_setsock_nonblock,mtcp_bind
    	sockid = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
    	sock_store[k] = sockid;

    	if (sockid < 0) {
      		TRACE_ERROR("Failed to create listening socket!\n");
      		return -1;
    	}
    
    	mtcp_setsock_nonblock(mctx, sockid);
    	int optval = 1;
        //mtcp_setsockopt(mctx, sockid, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval, sizeof(optval));
    	struct sockaddr_in saddr,daddr;
    	bzero((char *)&saddr, sizeof(saddr));
    	bzero((char *)&daddr, sizeof(daddr));

    	//for source
    	saddr.sin_family = AF_INET;
    	saddr.sin_port = htons(ran_my_port+k);

    	inet_aton(ran_ip,&saddr.sin_addr);

    	//for destination
    	daddr.sin_family = AF_INET;
    	daddr.sin_addr.s_addr = inet_addr(hostname);
    	daddr.sin_port = htons(portno);

      	//cout << "before mtcp_bind\n";
      	ret = mtcp_bind(mctx, sockid,(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
      	if (ret < 0) 
      	{
        		TRACE_ERROR("Failed to bind to the socket!\n");
        		return -1;
      	}
        
      	//cout << "Trying to connect." << endl;

        ret = mtcp_connect(mctx, sockid, (struct sockaddr *)&daddr, sizeof(struct sockaddr_in));
        //cout<<"MTCP CONN"<<errno<<endl;
      	if((ret < 0) && (errno == EINPROGRESS))
      	{
        		
        		//cout << "\nEINPROGRESS"<<sockid<<" "<<k<<"\n";
        		ev.events = MTCP_EPOLLOUT ;//|| MTCP_EPOLLET;
        		ev.data.sockid = sockid;
        		int returnval = mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);
        		if(returnval == -1)
        		{
          				
          				cout<<"Error: epoll connection add"<<endl;
          				exit(-1);
        		}

      }
      
      else if (ret < 0) 
      {
        		
        		if (errno != EINPROGRESS) 
        		{
          				
          				printf("mtcp_connect");
          				mtcp_close(mctx, sockid);
          				return -1;
        		}
      }
      
      else
      {
        		
        		//cout << "immediately connected\n";
        		ev.events = MTCP_EPOLLIN;// | MTCP_EPOLLET;
        		ev.data.sockid = sockid;
        		mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);
        		x--;
      
      }

      
  }
  

  
  //step 6. mtcp_epoll_wait
  
  struct mtcp_epoll_event *events;
  int nevents;
  events = (struct mtcp_epoll_event *)calloc(MAX_EVENTS, sizeof(struct mtcp_epoll_event));
  if (!events) {
    TRACE_ERROR("Failed to create event struct!\n");
    exit(-1);
  }
  
  char data[1448];

  x = no_of_connections;
  int retval;
  uint64_t autn_num;
  uint64_t xautn_num;
  uint64_t rand_num;
  uint64_t sqn;
  uint64_t res;
  uint64_t ck;
  uint64_t ik;
  uint8_t *hmac_res;
  uint8_t *hmac_xres;
  struct mdata fddata;
  uint64_t k_enodeb;
  int tai_list_size;
 

  while(1)
  {

    		
    		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, -1);
    		
    		if (nevents < 0) 
    		{
      				
      				if (errno != EINTR)
        			perror("epoll_wait");
      				break;
    		}

    		for(int i=0;i<nevents;i++) 
    		{ 

      
      				if (events[i].events & MTCP_EPOLLOUT) 
      						x--;

    		}

   			if(x==0) 
      			break;

    
  }
 


  for(int i=0; i<no_of_connections; i++)
  {

    		
    		Ran ran;
      		ran.ran_ctx.init(i);
      		int returnval;
		    ran.pkt.clear_pkt();
		    ran.pkt.append_item(ran.ran_ctx.imsi);
		    ran.pkt.append_item(ran.ran_ctx.tai);
		    ran.pkt.append_item(ran.ran_ctx.ksi_asme);
		    ran.pkt.append_item(ran.ran_ctx.nw_capability);
		    ran.pkt.prepend_s1ap_hdr(1, ran.pkt.len, ran.ran_ctx.enodeb_s1ap_ue_id, 0);
		    ran.pkt.prepend_len();
		    returnval = mtcp_write(mctx, sock_store[i], ran.pkt.data, ran.pkt.len);
		    
		    if(returnval < 0)
		    {
		      
		      		cout<<"Error: Cant send to RAN"<<endl;
		      		exit(-1);
		    }

    		mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sock_store[i], &ev);
    		ev.events = MTCP_EPOLLIN;
      		ev.data.sockid = sock_store[i];
      		mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sock_store[i], &ev);
      		memcpy(fddata.buf, ran.pkt.data, ran.pkt.len);
        	fddata.len = ran.pkt.len; 
        	fdmap.insert(make_pair(sock_store[i],ran));
        	fdmap1.insert(make_pair(sock_store[i],fddata));


  }

  

 
        
 x = no_of_connections;
  
 while(1)
 {
      

      		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, 100);
      

      		for(int i=0;i<nevents;i++) 
      		{ 

          

        			if (nevents < 0) 
        			{
          					
          					if (errno != EINTR)
            				printf("epoll_wait");
          					break;
        			}

        			if( (events[i].events & MTCP_EPOLLERR) || (events[i].events & MTCP_EPOLLHUP)) 
        			{
          					
          					cout<<"ERROR: epoll monitoring failed, closing fd"<<'\n';
					        mtcp_close(mctx,events[i].data.sockid);
					        continue;
        			}


      				else if(events[i].events & MTCP_EPOLLIN)
      				{


				            char * dataptr;
				            int cur_fd;
				            Packet packet;
				            Ran ran;  
				            int pkt_len;
				            cur_fd = events[i].data.sockid;
				            ran = fdmap[cur_fd];
				              
				            packet.clear_pkt();

				            retval = mtcp_read(mctx, cur_fd, data, 500);
            				if(retval < 0)
            				{
              		
              						TRACE(cout<<"ERROR: read packet from ran"<<endl;)
              						exit(-1);
            				}
            
				            memcpy(&pkt_len, data, sizeof(int));
				            dataptr = data+sizeof(int);
				            memcpy(packet.data, (dataptr), pkt_len);
				            packet.data_ptr = 0;
				            packet.len = pkt_len;
            
				            packet.extract_s1ap_hdr();
				            //cout << "packetid " <<  packet.s1ap_hdr.mme_s1ap_ue_id << endl;
				            memcpy(fddata.buf, packet.data, packet.len);
				            fddata.len = packet.len;
				            fdmap.erase(cur_fd);
				            fdmap1.erase(cur_fd);
				            fdmap.insert(make_pair(cur_fd, ran));
				            fdmap1.insert(make_pair(cur_fd,fddata));
				            x--;

      				}

    		}

    
    		if(x==0) break;
  }


  for(int i=0; i<no_of_connections; i++)
  {
      

		    Ran ran;  
		    bool ok;
		    ran = fdmap[sock_store[i]];
		    Packet pkt;
		    fddata = fdmap1[sock_store[i]];
		    memcpy(pkt.data, fddata.buf, fddata.len);
		    pkt.len = fddata.len;
		    int returnval;
		    pkt.extract_s1ap_hdr();
		    cout << "packetid " <<  pkt.s1ap_hdr.mme_s1ap_ue_id << endl;

		    ran.ran_ctx.mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
		    pkt.extract_item(xautn_num);
		    pkt.extract_item(rand_num);
		    pkt.extract_item(ran.ran_ctx.ksi_asme);
		  
		    TRACE(cout << "ran_authenticate: " << " autn: " << xautn_num << " rand: " << rand_num  << " ksiasme: " << ran.ran_ctx.ksi_asme << ": " << ran.ran_ctx.imsi << endl;)
		    sqn = rand_num + 1;
		    res = ran.ran_ctx.key + sqn + rand_num;
		    autn_num = res + 1;
    
		    if (autn_num != xautn_num) {
		      TRACE(cout << "ran_authenticate:" << " authentication of MME failure: " << ran.ran_ctx.imsi << endl;)
		      return false;
		    }
		    TRACE(cout << "ran_authenticate:" << " autn success: " << ran.ran_ctx.imsi << endl;)
		    ck = res + 2;
		    ik = res + 3;
		    ran.ran_ctx.k_asme = ck + ik + sqn + ran.ran_ctx.plmn_id;
		    pkt.clear_pkt();
		    pkt.append_item(res);
		    pkt.prepend_s1ap_hdr(2, pkt.len, ran.ran_ctx.enodeb_s1ap_ue_id, ran.ran_ctx.mme_s1ap_ue_id);
		    pkt.prepend_len();
		    returnval = mtcp_write(mctx, sock_store[i], pkt.data, pkt.len);
		    
		    if(returnval < 0)
		    {
		      cout<<"Error: Cant send to RAN"<<endl;
		      exit(-1);
		    }
		    TRACE(cout << "ran_authenticate:" << " autn response sent to mme: " << ran.ran_ctx.imsi << endl;)
		    fdmap.erase(sock_store[i]);
		    fdmap.insert(make_pair(sock_store[i], ran));


  }

  
  x = no_of_connections;

  while(1)
  {
      
      		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, 100);
      

      		for(int i=0;i<nevents;i++) 
      		{ 

        			
        			if (nevents < 0) 
        			{
          	
          					if (errno != EINTR)
            				printf("epoll_wait");
          					break;
        			}

			        if( (events[i].events & MTCP_EPOLLERR) || (events[i].events & MTCP_EPOLLHUP)) 
			        {
			          
			          		cout<<"ERROR: epoll monitoring failed, closing fd"<<'\n';
			          		mtcp_close(mctx,events[i].data.sockid);
			          		continue;
			        }

				    else if(events[i].events & MTCP_EPOLLIN)
				    {



				            char * dataptr;
				            int cur_fd;
				            int pkt_len;
				            Packet packet;
				            Ran ran;  
				            cur_fd = events[i].data.sockid;
				            ran = fdmap[cur_fd];
				            packet.clear_pkt();

				            retval = mtcp_read(mctx, cur_fd, data, 500);
				            if(retval < 0)
				            {
				              TRACE(cout<<"ERROR: read packet from ran"<<endl;)
				              exit(-1);
				            }
				            
				            memcpy(&pkt_len, data, sizeof(int));
				            dataptr = data+sizeof(int);
				            memcpy(packet.data, (dataptr), pkt_len);
				            packet.data_ptr = 0;
				            packet.len = pkt_len;
				            
				            packet.extract_s1ap_hdr(); 
				            memcpy(fddata.buf, packet.data, packet.len);
				            fddata.len = packet.len;
				            fdmap.erase(cur_fd);
				            fdmap1.erase(cur_fd);
				            fdmap.insert(make_pair(cur_fd, ran));
				            fdmap1.insert(make_pair(cur_fd,fddata));
				            x--;


      
      				}

    
    		}

    		
    		if(x==0) break;

  }

  for(int i=0; i<no_of_connections; i++)
  {
   


		    Ran ran;  
		    bool res;
		    int returnval;
		    ran = fdmap[sock_store[i]];
		    Packet pkt;
		    fddata = fdmap1[sock_store[i]];

			memcpy(pkt.data, fddata.buf, fddata.len);
			pkt.len = fddata.len;
			if (pkt.len <= 0) {
			    TRACE(cout << "packet error\n";)
			} 
    		hmac_res = g_utils.allocate_uint8_mem(HMAC_LEN);
  			hmac_xres = g_utils.allocate_uint8_mem(HMAC_LEN);
  			TRACE(cout << "ran_setsecurity: " << " received request for ran: " << pkt.len << ": " << ran.ran_ctx.imsi << endl;)
  			pkt.extract_s1ap_hdr();
  			if (HMAC_ON) {
    			g_integrity.rem_hmac(pkt, hmac_xres);
  			}

		    ran.ran_ctx.mme_s1ap_ue_id = pkt.s1ap_hdr.mme_s1ap_ue_id;
		    pkt.extract_item(ran.ran_ctx.ksi_asme);
		    pkt.extract_item(ran.ran_ctx.nw_capability);
		    pkt.extract_item(ran.ran_ctx.nas_enc_algo);
		    pkt.extract_item(ran.ran_ctx.nas_int_algo);
		    ran.ran_ctx.k_nas_enc = ran.ran_ctx.k_asme + ran.ran_ctx.nas_enc_algo + ran.ran_ctx.count + ran.ran_ctx.bearer + ran.ran_ctx.dir + 1;
		    ran.ran_ctx.k_nas_int = ran.ran_ctx.k_asme + ran.ran_ctx.nas_int_algo + ran.ran_ctx.count + ran.ran_ctx.bearer + ran.ran_ctx.dir + 1;

  
  			if (HMAC_ON) 
  			{
    
    				g_integrity.get_hmac(pkt.data, pkt.len, hmac_res, ran.ran_ctx.k_nas_int);
    				res = g_integrity.cmp_hmacs(hmac_res, hmac_xres);
    				if (res == false) {
      						TRACE(cout << "ran_setsecurity:" << " hmac security mode command failure: " << ran.ran_ctx.imsi << endl;)
      						g_utils.handle_type1_error(-1, "hmac error: ran_setsecurity");
    				}
  			
  			}


			TRACE(cout << "ran_setsecurity:" << " security mode command success: " << ran.ran_ctx.imsi << endl;)
			res = true;
			pkt.clear_pkt();
			pkt.append_item(res);
			if (ENC_ON) {
			    g_crypt.enc(pkt, ran.ran_ctx.k_nas_enc);
			}
			if (HMAC_ON) {
			    g_integrity.add_hmac(pkt, ran.ran_ctx.k_nas_int);
			}
			pkt.prepend_s1ap_hdr(3, pkt.len, ran.ran_ctx.enodeb_s1ap_ue_id, ran.ran_ctx.mme_s1ap_ue_id);
			pkt.prepend_len();
			returnval = mtcp_write(mctx, sock_store[i], pkt.data, pkt.len);
			  
			if(returnval < 0)
			{

			    	cout<<"Error: Cant send to RAN"<<endl;
			    	
			    	exit(-1);
			}
			  
			TRACE(cout << "ran_setsecurity:" << " security mode complete sent to mme: " << pkt.len << ": " << ran.ran_ctx.imsi << endl;)
			free(hmac_res);
			free(hmac_xres);
			fdmap.erase(sock_store[i]);
			fdmap.insert(make_pair(sock_store[i], ran));


  }

  x = no_of_connections;

  while(1)
  {
      

      		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, 100);

      		for(int i=0;i<nevents;i++) 
      		{ 

        	if (nevents < 0) 
        	{
          		
          			if (errno != EINTR)
            		printf("epoll_wait");
          			break;
        	}

        	if( (events[i].events & MTCP_EPOLLERR) || (events[i].events & MTCP_EPOLLHUP)) 
        	{
          
          			cout<<"ERROR: epoll monitoring failed, closing fd"<<'\n';
          			mtcp_close(mctx,events[i].data.sockid);
          			continue;
        	}

      		else if(events[i].events & MTCP_EPOLLIN)
      		{


		            char * dataptr;
		            int cur_fd;
		            Packet packet;
		            Ran ran;  
		            int pkt_len;
		            cur_fd = events[i].data.sockid;
		            ran = fdmap[cur_fd];
		            packet.clear_pkt();

              		retval = mtcp_read(mctx, cur_fd, data, 500);
            		if(retval < 0)
            		{
              
              				TRACE(cout<<"ERROR: read packet from ran"<<endl;)
              				exit(-1);
            
            		}
            
            		memcpy(&pkt_len, data, sizeof(int));
            		dataptr = data+sizeof(int);
            		memcpy(packet.data, (dataptr), pkt_len);
            		packet.data_ptr = 0;
            		packet.len = pkt_len;
            
           		   	packet.extract_s1ap_hdr();
              		memcpy(fddata.buf, packet.data, packet.len);
              		fddata.len = packet.len;
              		fdmap.erase(cur_fd);
              		fdmap1.erase(cur_fd);
              		fdmap.insert(make_pair(cur_fd, ran));
              		fdmap1.insert(make_pair(cur_fd,fddata));
              		x--;


      		}

    }

    if(x==0) break;

}

for(int i=0; i<no_of_connections; i++)
{
      
    

	    Ran ran;  
	    EpcAddrs epc_addrs;
	    ran = fdmap[sock_store[i]];
	    Packet pkt;
	    fddata = fdmap1[sock_store[i]];
	    memcpy(pkt.data, fddata.buf, fddata.len);
	    pkt.len = fddata.len;
	    int returnval;
      
    	if (pkt.len <= 0) {
    		return false;
   	 	}

    	TRACE(cout << "ran_setepssession:" << " attach accept received from mme: " << pkt.len << ": " << ran.ran_ctx.imsi << endl;)
    	pkt.extract_s1ap_hdr();
    	if (HMAC_ON) {
      		res = g_integrity.hmac_check(pkt, ran.ran_ctx.k_nas_int);
      	if (res == false) {
        	TRACE(cout << "ran_setepssession:" << " hmac attach accept failure: " << ran.ran_ctx.imsi << endl;)
        	g_utils.handle_type1_error(-1, "hmac error: ran_setepssession");
      	}
    	}
    	if (ENC_ON) {
      		g_crypt.dec(pkt, ran.ran_ctx.k_nas_enc);  
    	}
	    pkt.extract_item(ran.ran_ctx.guti);
	    pkt.extract_item(ran.ran_ctx.eps_bearer_id);
	    pkt.extract_item(ran.ran_ctx.e_rab_id);
	    pkt.extract_item(ran.ran_ctx.s1_uteid_ul);
	    pkt.extract_item(k_enodeb);
	    pkt.extract_item(ran.ran_ctx.nw_capability);
	    pkt.extract_item(tai_list_size);
	    pkt.extract_item(ran.ran_ctx.tai_list, tai_list_size);
	    pkt.extract_item(ran.ran_ctx.tau_timer);
	    pkt.extract_item(ran.ran_ctx.ip_addr);
	    pkt.extract_item(epc_addrs.sgw_s1_ip_addr);
	    pkt.extract_item(epc_addrs.sgw_s1_port);
	    pkt.extract_item(res);  
	    if (res == false) {
      	TRACE(cout << "ran_setepssession:" << " attach request failure: " << ran.ran_ctx.imsi << endl;)
      		return false;
    	} 
    
    	//traf_mon.update_uplink_info(ran_ctx.ip_addr, ran_ctx.s1_uteid_ul, epc_addrs.sgw_s1_ip_addr, epc_addrs.sgw_s1_port);
    	ran.ran_ctx.s1_uteid_dl = ran.ran_ctx.s1_uteid_ul;
    	pkt.clear_pkt();
    	pkt.append_item(ran.ran_ctx.eps_bearer_id);
    	pkt.append_item(ran.ran_ctx.s1_uteid_dl);
    
    	if (ENC_ON) {
      		g_crypt.enc(pkt, ran.ran_ctx.k_nas_enc);
    	}
    	if (HMAC_ON) {
      		g_integrity.add_hmac(pkt, ran.ran_ctx.k_nas_int);
    	}
    	pkt.prepend_s1ap_hdr(4, pkt.len, ran.ran_ctx.enodeb_s1ap_ue_id, ran.ran_ctx.mme_s1ap_ue_id);
    	pkt.prepend_len();
    	returnval = mtcp_write(mctx, sock_store[i], pkt.data, pkt.len);
  
    	if(returnval < 0)
    	{
      
      			cout<<"Error: Cant send to RAN"<<endl;
      			exit(-1);
    	}
    

    	TRACE(cout << "ran_setepssession:" << " attach complete sent to mme: " << pkt.len << ": " << ran.ran_ctx.imsi << endl;)
    	ran.ran_ctx.emm_state = 1;
    	ran.ran_ctx.ecm_state = 1;
    	uint64_t detach_type;
    	bool res;
    	fdmap.erase(sock_store[i]);
    	fdmap.insert(make_pair(sock_store[i], ran));
  

}

for(int i=0;i<no_of_connections;i++)
{

        mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sock_store[i], NULL);
        mtcp_close(mctx, sock_store[i]);
}


//connection with sink/////////////////////////////////////////////////////////////////////////////////////////
  
  

for(int k=0;k<no_of_connections;k++)
{



        //step 4. mtcp_socket, mtcp_setsock_nonblock,mtcp_bind
        int sockid = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
        sock_sink1[k] = sockid;

        if (sockid < 0) {
          TRACE_ERROR("Failed to create listening socket!\n");
          return -1;
        }
        
        mtcp_setsock_nonblock(mctx, sockid);
        int optval = 1;
        //mtcp_setsockopt(mctx, sockid, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &optval, sizeof(optval));
        struct sockaddr_in saddr,daddr;
        bzero((char *)&saddr, sizeof(saddr));
        bzero((char *)&daddr, sizeof(daddr));

        //for destination
        daddr.sin_family = AF_INET;
        daddr.sin_addr.s_addr = inet_addr(sink_ip);
        daddr.sin_port = htons(sink_portnum);
        ret = mtcp_connect(mctx, sockid, (struct sockaddr *)&daddr, sizeof(struct sockaddr_in));
 
	    if((ret < 0) && (errno == EINPROGRESS))
	    {
	                
	                

	            ev.events = MTCP_EPOLLOUT ;//|| MTCP_EPOLLET;
	            ev.data.sockid = sockid;
	            int returnval = mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);
	            if(returnval == -1)
	            {
	                    cout<<"Error: epoll connection add"<<endl;
	                    exit(-1);
	            }

	    }

	    else if (ret < 0) 
	    {

                if (errno != EINPROGRESS) 
                {
                        printf("mtcp_connect");
                        mtcp_close(mctx, sockid);
                        return -1;
                }
	    }

	      else
	      {

                ev.events = MTCP_EPOLLIN ;//|| MTCP_EPOLLET;
                ev.data.sockid = sockid;
                mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);
                //x--;

	      }

      
}



char data1[packet_size];
char read_data[packet_size];
size_t result;
for(int i=0;i<packet_size;i++)
data1[i] = '1';
clock_t start, end;
double cpu_time_used;

x = no_of_connections;

//start = clock();
std::chrono::time_point<std::chrono::system_clock> startt, endt;
startt = std::chrono::system_clock::now();
cout<<"Going to Send\n";
int sentpkts = 0;
while(1)
{


        nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, -1);
        if (nevents < 0) 
        {
                if (errno != EINTR)
                perror("epoll_wait");
                break;
        }

        for(int i=0;i<nevents;i++) 
        { 


                if( (events[i].events & MTCP_EPOLLERR) || (events[i].events & MTCP_EPOLLHUP)) 
                {

                          cout<<"ERROR: epoll monitoring failed, closing fd"<<'\n';
                          mtcp_close(mctx,events[i].data.sockid);
                          continue;
                }
      
                        else if (events[i].events & MTCP_EPOLLOUT) 
                        {


                                int written_bytes;
                                written_bytes = mtcp_write(mctx, events[i].data.sockid, data1,packet_size);
                                sentpkts++;
       

                        }

 

                }


                endt = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds = endt - startt;
                cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
                if(elapsed_seconds.count() > transfer_time) 
                break;

                
}

TRACE(cout<<"Closing\n";)
for(int i=0;i<no_of_connections;i++)
{
            mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sock_sink1[i], NULL);
            mtcp_close(mctx, sock_sink1[i]);
}

return 0;

}