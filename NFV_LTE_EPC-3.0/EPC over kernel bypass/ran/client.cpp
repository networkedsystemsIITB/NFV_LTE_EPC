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
#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "cpu.h"
#include "debug.h"

#define MAX_EVENTS 1024

using namespace std;
int done;
mctx_t mctx;
/*----------------------------------------------------------------------------*/
void
SignalHandler(int signum)
{
	//Handle ctrl+C here
	mtcp_destroy_context(mctx);	
	mtcp_destroy();
	done = 1;
}
/*----------------------------------------------------------------------------*/

int main(int argc, char **argv){
	
	int core = 0;
	int ret = -1;
	done = 0;
	
	if (argc != 3) {
       fprintf(stderr,"usage: <hostname> <port>\n", argv[0]);
       exit(0);
    }
    char* hostname = argv[1];
    int portno = atoi(argv[2]);
    
    char* conf_file = "client.conf";
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
		return NULL;
	}

	/* create epoll descriptor */
	int ep = mtcp_epoll_create(mctx, MAX_EVENTS);
	if (ep < 0) {
		TRACE_ERROR("Failed to create epoll descriptor!\n");
		return NULL;
	}
	
	//step 4. mtcp_socket, mtcp_setsock_nonblock,mtcp_bind
	int sockid = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		TRACE_ERROR("Failed to create listening socket!\n");
		return -1;
	}
	ret = mtcp_setsock_nonblock(mctx, sockid);
	if (ret < 0) {
		TRACE_ERROR("Failed to set socket in nonblocking mode.\n");
		return -1;
	}
	
	struct sockaddr_in daddr;
	
	daddr.sin_family = AF_INET;
	daddr.sin_addr.s_addr = inet_addr(hostname);
	daddr.sin_port = htons(portno);
	
	ret = mtcp_connect(mctx, sockid, (struct sockaddr *)&daddr, sizeof(struct sockaddr_in));
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			perror("mtcp_connect");
			mtcp_close(mctx, sockid);
			return -1;
		}
	}
	/*char *buf = "hello";
	ret = mtcp_write(mctx, sockid, buf, 5);
	if (ret < 0) {
		TRACE_CONFIG("Connection closed with client.\n");
	}*/
	struct mtcp_epoll_event ev;
	ev.events = MTCP_EPOLLIN;
	ev.data.sockid = sockid;
	mtcp_epoll_ctl(mctx, ep, MTCP_EPOLL_CTL_ADD, sockid, &ev);
	
	//step 6. mtcp_epoll_wait
	struct mtcp_epoll_event *events;
	int nevents;
	events = (struct mtcp_epoll_event *)calloc(MAX_EVENTS, sizeof(struct mtcp_epoll_event));
	if (!events) {
		TRACE_ERROR("Failed to create event struct!\n");
		exit(-1);
	}
	int newsockfd = -1;
	
	while(!done){
		nevents = mtcp_epoll_wait(mctx, ep, events, MAX_EVENTS, -1);
		
		if (nevents < 0) {
			if (errno != EINTR)
				perror("mtcp_epoll_wait");
			break;
		}
		char data[1024];
		for(int i=0;i<nevents;i++) {
			if (events[i].events & MTCP_EPOLLIN) {
				//receive data over new connection
				bzero(data,1024);
				int rd = mtcp_read(mctx, sockid, data, 1024);
				if (rd <= 0) {
					return rd;
				}
				cout << data << endl;
			}	
		}
	}
	
}
