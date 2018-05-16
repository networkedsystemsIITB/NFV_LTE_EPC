#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <iostream>
#include "mtcp_api.h"
#include "mtcp_epoll.h"

#include <iostream>
#include "cpu.h"
#include "debug.h"
#include "defport.h"
#include <time.h>
#include <unistd.h>

#include "gtp.h"
#include "network.h"
#include "packet.h"
#include "s1ap.h"
#include "sync.h"
#include "utils.h"

#define MAX_EVENTS 65536
#define no_of_connections 5
#define packet_size 1448
#define duration_of_exp 300


using namespace std;
int done;
mctx_t mctx;
int ep;
long long int j;
int listener;
int sock_sink[no_of_connections];
/*----------------------------------------------------------------------------*/

void 
CloseConnection()
{
        for(int i=0;i<no_of_connections;i++)
        {
                mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sock_sink[i], NULL);
                mtcp_close(mctx, sock_sink[i]);
        }

        mtcp_close(mctx, listener);
}

void
SignalHandler(int signum)
{
  //Handle ctrl+C here
  //signal_handler_dpdk(signum);
        for(int i=0;i<no_of_connections;i++)
        {
                mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_DEL, sock_sink[i], NULL);
                mtcp_close(mctx, sock_sink[i]);
        }

        mtcp_close(mctx, listener);
  double numr = j * 8 * packet_size; 
  double demom = duration_of_exp * 1024 * 1024 * 1024;
  cout << ( numr ) / ( denom) << endl;
  //mtcp_destroy_context(mctx); 
  mtcp_destroy();
  cout << "remove mtcp\n";
  done = 1;

}
/*----------------------------------------------------------------------------*/

int main(int argc, char **argv){

  //signal(SIGINT, SignalHandler);
  //float packets=0, sec=0;
  //char buf[MAXBUFLEN];
  //clock_t before,diff;
  //long i=0,numbytes;
  float rate;
  
  int core = 0;
  int ret = -1;
  done = 0;
  int x = 0;

  int portno = sink_portnum;//atoi(argv[1]);
    
  char* conf_file = "server.conf";
    /* initialize mtcp */
  if (conf_file == NULL) {
    TRACE_CONFIG("You forgot to pass the mTCP startup config file!\n");
    exit(EXIT_FAILURE);
  }
  else {
    TRACE_INFO("Reading configuration from %s\n",conf_file);
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
    //return NULL;
  }
  else{
    TRACE_INFO("mtcp context created.\n");
  }
  /* create epoll descriptor */

  //sleep(10);
  ep = mtcp_epoll_create(mctx, MAX_EVENTS);
  if (ep < 0) {
    TRACE_ERROR("Failed to create epoll descriptor!\n");
    //return NULL;
  }
  
  //step 4. mtcp_socket, mtcp_setsock_nonblock,mtcp_bind
  listener = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
  if (listener < 0) {
    TRACE_ERROR("Failed to create listening socket!\n");
    //return -1;
  }
  ret = mtcp_setsock_nonblock(mctx, listener);
  if (ret < 0) {
    TRACE_ERROR("Failed to set socket in nonblocking mode.\n");
    //return -1;
  }
  
  struct sockaddr_in saddr;
  
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = inet_addr(sink_ip);
  saddr.sin_port = htons(portno);
  
  ret = mtcp_bind(mctx, listener,(struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
  if (ret < 0) {
    TRACE_ERROR("Failed to bind to the listening socket!\n");
    //return -1;
  }

  //step 5. mtcp_listen, mtcp_epoll_ctl
  /* listen (backlog: 4K) */
  ret = mtcp_listen(mctx, listener, 1000);
  if (ret < 0) {
    TRACE_ERROR("mtcp_listen() failed!\n");
    //return -1;
  }
    
  /* wait for incoming accept events */
  struct mtcp_epoll_event ev;
  ev.events = MTCP_EPOLLIN ;//|| MTCP_EPOLLET;
  ev.data.sockid = listener;
  mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, listener, &ev);
  
  //step 6. mtcp_epoll_wait
  struct mtcp_epoll_event *events;
  int nevents;
  //int newsockfd = -1;
  events = (struct mtcp_epoll_event *)calloc(MAX_EVENTS, sizeof(struct mtcp_epoll_event));
  if (!events) {
    TRACE_ERROR("Failed to create event struct!\n");
    exit(-1);
  }
  cout << "Waiting for events" << endl;
  //FILE * pFile;
  long lSize;
  char data[packet_size];
  bzero(data,packet_size);
  char write_data[packet_size];

  for(int i=0;i<packet_size;i++)
        write_data[i] = '2';

  lSize = packet_size;

  j = 0;
  while(1)
  {

  //while(dpdk_i<30){

        nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS , -1);
        //cout << nevents << endl;
    
        if (nevents < 0) 
        { 
                if (errno != EINTR)
                perror("mtcp_epoll_wait");
                break;
        }
    
    
        for(int i=0;i<nevents;i++) 
        {
      
        if( (events[i].events & MTCP_EPOLLERR) || (events[i].events & MTCP_EPOLLHUP)) 
        {

                  cout<<"ERROR: epoll monitoring failed, closing fd"<<'\n';
                  mtcp_close(mctx, events[i].data.sockid);
             
        }
      
        else if (events[i].data.sockid == listener) 
        {

                cout << "\nAccept connection\n";
                int newsockfd = mtcp_accept(mctx, listener, NULL, NULL);
                sock_sink[x++] = newsockfd;
                printf("New connection %d accepted.\n",newsockfd);
                ev.events = MTCP_EPOLLIN ;//|| MTCP_EPOLLET;
                ev.data.sockid = newsockfd;
                mtcp_setsock_nonblock(mctx, newsockfd);
                mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, newsockfd, &ev);
      
        
        }

        else if (events[i].events & MTCP_EPOLLIN) 
        {



                int ptr;
                int retval;
                int read_bytes;
                int remaining_bytes;
                ptr = 0;
                remaining_bytes = packet_size;
                
                read_bytes = mtcp_read(mctx,events[i].data.sockid,data,packet_size);


               j++;
        }

        }
  } 

  return 0;
}

