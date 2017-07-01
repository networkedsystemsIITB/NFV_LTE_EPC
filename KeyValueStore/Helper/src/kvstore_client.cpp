#ifndef __KVSTORE_CLIENT_H__
#define __KVSTORE_CLIENT_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <vector>

#define MAX_INPUT_SIZE 1048576 //1MB

#define CREATE_TABLE 0
#define GET 1
#define PUT 2
#define DEL 3
#define CLEAR 4

using namespace std;
inline void itoba(uint32_t i,char *ba){
  ba[0]=(i>>24)&255;
  ba[1]=(i>>16)&255;
  ba[2]=(i>>8)&255;
  ba[3]=(i)&255;
}

inline uint32_t batoi(char *ba){
  uint32_t i=0;
  int iba[4];
  iba[0]=ba[0];
  iba[1]=ba[1];
  iba[2]=ba[2];
  iba[3]=ba[3];
  i=(uint32_t)((iba[0]&255)<<24)|((iba[1]&255)<<16)|((iba[2]&255)<<8)|(iba[3]&255);
}

class KVStoreClient{
private:
  uint32_t num_bytes;
  int portnum;
  string ip;
  int sockfd, n;
  struct sockaddr_in server_addr;
  char inputbuf[MAX_INPUT_SIZE],*more_buf;
  char outbuf[MAX_INPUT_SIZE];

private:
public:
  KVStoreClient(string ip_addr,int port){
    ip=ip_addr;
    portnum=port;
    /* Create client socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
      fprintf(stderr, "ERROR in opening socket\n");
      exit(1);
    }

    /* Fill in server address */
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if(!inet_aton(ip.c_str(), &server_addr.sin_addr))
    {
      fprintf(stderr, "ERROR invalid server IP address\n");
      exit(1);
    }
    server_addr.sin_port = htons(portnum);

    /* Connect to server */
    if (connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
    {
      fprintf(stderr, "ERROR connecting\n");
      exit(1);
    }
  }

  void send(vector<string> &v){
    int n;
    int len=v.size();
    int sum=1; //First byte as Number of stings
    for(int i=0;i<len;i++){
      sum+=(v[i].size()+4); //4 byte for string length
    }
    int idx=0;
    char *data=outbuf;
    if(MAX_INPUT_SIZE<sum){
      cout<<"Alloc data sum="<<sum<<endl;
      data = new char[sum];
    }
    data[idx]=(char)len;
    idx++;
    for(int i=0;i<len;i++){
      itoba((uint32_t)v[i].size(),data+idx);
      idx+=4;
      strncpy(data+idx,v[i].c_str(),v[i].size());
      idx+=v[i].size();
    }
    n=-1;
    int sent=0;
    while(sent<sum){
      n = write(sockfd,data+sent,sum-sent);

      if (errno == EPERM) {
        errno = 0;
        usleep(1000);
        continue;
      }
      else if(n<0) {
        if ((errno == EAGAIN) || (errno == EINTR)) {
          errno = 0;
          usleep(1000);
          continue;
        } else {
            fprintf(stderr, "ERROR writing to socket %d\n",errno);
            exit(1);
        }
      }
      sent+=n;
    }
    if(MAX_INPUT_SIZE<sum){

      cout<<"Delete data sum="<<sum<<endl;
      delete(data);
    }
  }

  vector<string> receive(){
    int n;
    int sumn=0;
    int sz=0;
    sz=1;
    sumn = 0;
    while(sumn!=sz){
      n = read(sockfd,inputbuf+sumn,sz-sumn);
      sumn+=n;
      if (n < 0)
      {
        if ((errno == EAGAIN) || (errno == EINTR) || (errno == EPERM)) {
          errno = 0;
          usleep(1000);
          continue;
        } else {
            fprintf(stderr, "ERROR(1) reading from socket\n");
            exit(1);
        }
      }
    }
    int len=(int)inputbuf[0];
    std::vector<string> v(len);
    for(int i=0;i<len;i++){
      sz=4;
      sumn = 0;
      while(sumn!=sz){
        n = read(sockfd,inputbuf+sumn,sz-sumn);
        sumn+=n;
        if (n < 0)
        {
          if ((errno == EAGAIN) || (errno == EINTR) || (errno == EPERM)) {
            errno = 0;
            usleep(1000);
            continue;
          } else {
              fprintf(stderr, "ERROR(2) reading from socket\n");
              exit(1);
          }
        }
      }
      sz=(int)batoi(inputbuf);
      sumn = 0;
      while(sumn!=sz){
        n = read(sockfd,inputbuf+sumn,sz-sumn);
        sumn+=n;
        if (n < 0)
        {
          if ((errno == EAGAIN) || (errno == EINTR) || (errno == EPERM)) {
            errno = 0;
            usleep(1000);
            continue;
          } else {
              fprintf(stderr, "ERROR(3) reading from socket\n");
              exit(1);
          }
        }
      }
      v[i]=string(inputbuf,sz);
    }
    return v;
  }
};
#endif
