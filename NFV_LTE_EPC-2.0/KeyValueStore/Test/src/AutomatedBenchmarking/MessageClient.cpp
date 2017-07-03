#ifndef __MESSAGE_CLIENT_H__
#define __MESSAGE_CLIENT_H__

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

namespace MessageClientNS{

#define MAX_INPUT_SIZE 1048576 //1MB

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

class MessageClient{
private:
  uint32_t num_bytes;
  char op_code;
  int portnum;
  string ip;
  int sockfd, n;
  struct sockaddr_in server_addr;
  char inputbuf[MAX_INPUT_SIZE],*more_buf;
  char outbuf[MAX_INPUT_SIZE];

private:


  /*
  void write_string(string str){
}
string read_string(){
//bzero(inputbuf,4);
n = read(sockfd,inputbuf,4);
if (n < 0)
{
fprintf(stderr, "ERROR reading from socket\n");
exit(1);
}
num_bytes=(inputbuf[0]<<24)+(inputbuf[1]<<16)+(inputbuf[2]<<8)+(inputbuf[3]);
if(num_bytes>=MAX_INPUT_SIZE){
more_buf=new char[num_bytes+1];

n = read(sockfd,more_buf,(int)num_bytes);
if (n < 0)
{
fprintf(stderr, "ERROR reading from socket\n");
exit(1);
}
more_buf[num_bytes]='\0';
string str(more_buf);
delete more_buf;
return str;
} else {

n = read(sockfd,inputbuf,num_bytes);
if (n < 0)
{
fprintf(stderr, "ERROR reading from socket\n");
exit(1);
}
inputbuf[num_bytes]='\0';
return string(inputbuf);
}
return NULL;
}
*/
public:
  MessageClient(string ip_addr,int port){
    ip=ip_addr;
    portnum=port;
    /* Create client socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
      fprintf(stderr, "ERROR opening socket\n");
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
    //printf("Connected to server\n");
  }
  /*
  bool create_table_reply(){
  n = read(sockfd,inputbuf,1);
  if (n < 0)
  {
  fprintf(stderr, "ERROR reading from socket\n");
  exit(1);
}
if(inputbuf[0]==CREATE_TABLE){

n = read(sockfd,inputbuf,5);
if (n < 0)
{
fprintf(stderr, "ERROR reading from socket\n");
exit(1);
}
if(inputbuf[4]==0){
return false;
} else {
return true;
}
}else{
fprintf(stderr, "ERROR in create table reply\n");
exit(1);
}
return false;
}
*/
void send(vector<string> &v){
  int len=v.size();
  int sum=1; //First byte as Number of stings
  for(int i=0;i<len;i++){
    sum+=(v[i].size()+4);
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
	    fprintf(stderr, "ERROR writing to socket %d\n",errno);
	    exit(1);
//		break;
	}
//  if (n < 0)
//  {
//    fprintf(stderr, "ERROR writing to socket\n",);
//    exit(1);
//  }
  sent+=n;
  }
  if(MAX_INPUT_SIZE<sum){

	  cout<<"Delete data sum="<<sum<<endl;
   delete(data);
  }
}

vector<string> receive(){
  n = read(sockfd,inputbuf,1);
  if (n < 0)
  {
    fprintf(stderr, "ERROR reading from socket\n");
    exit(1);
  }
  int len=(int)inputbuf[0];
  std::vector<string> v(len);
  for(int i=0;i<len;i++){
    n = read(sockfd,inputbuf,4);
    if (n < 0)
    {
      fprintf(stderr, "ERROR reading from socket\n");
      exit(1);
    }
    int sz=(int)batoi(inputbuf);
    int sumn=0;
    while(sumn!=sz){
      n = read(sockfd,inputbuf+sumn,sz-sumn);
      sumn+=n;
      //printf("SZ:%d  N:%d\n",sz,n);
      if (n < 0)
      {
        fprintf(stderr, "ERROR reading from socket\n");
        exit(1);
      }
    }
    v[i]=string(inputbuf,sz);
    // cout<<v[i]<<endl;
  }
  return v;
}
};

/*
int main(int argc, char *argv[])
{
// char chx[4];
// for(int i=0;i<1024;i++){
//   itoba(i,chx);
//   cout<<batoi(chx)<< "  ";
// }

// std::vector<string> v;
// v.push_back("CreateTable");
// v.push_back("shreeganesh");
// string byte10="1234567890";
// string mb1="";
// for(long long i=0;i<100*100;i++){
//   mb1+=byte10;
// }
// mb1+="abcd";
// cout<<"MB1:"<<mb1<<endl;
// v.push_back(mb1);
// v.push_back("Pratik");
//   KVStoreClient k("127.1.1.1",8090);
//   for(int i=0;i<1;i++){
//   k.send(v);
//   std::vector<string> r=k.receive();
//   int l=r.size();
//   for(int i=0;i<l;i++){
//     printf("%s\n",r[i].c_str());
//     cout<<r[i]<<" sz:"<<r[i].size()<<"\n";
//   }
//
//   for(int i=88;i<100;i++){
//     cout<<int(r[2][i]);
//   }
//
//   cout<<endl;
//sleep(1);

std::vector<string> v1;
v1.push_back("CreateTable");
v1.push_back("shreeganesh");
//
// std::vector<string> v2;
// v2.push_back("Put");
// v2.push_back("shreeganesh");
// v2.push_back("Om Nama Shivay");

std::vector<string> v3;
v3.push_back("Get");
v3.push_back("shreeganesh");


KVStoreClient k("127.1.1.1",8090);
k.send(v1);
std::vector<string> r=k.receive();
int l=r.size();
for(int i=0;i<l;i++){
cout<<r[i]<<" sz:"<<r[i].size()<<"\n";
}
// sleep(1);
// k.send(v2);
// r=k.receive();
// l=r.size();
// for(int i=0;i<l;i++){
//   cout<<r[i]<<" sz:"<<r[i].size()<<"\n";
// }
// sleep(1);
k.send(v3);
r=k.receive();
l=r.size();
for(int i=0;i<l;i++){
cout<<r[i]<<" sz:"<<r[i].size()<<"\n";
}

//  }
return 0;
}
*/
};
#endif
