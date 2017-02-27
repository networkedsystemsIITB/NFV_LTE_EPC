// g++ -std=c++11 -I/usr/local/include/r3c simple_use.cpp -lr3c /usr/local/lib/libhiredis.a

#include "r3c.h"
#include <thread>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string.h>

using namespace r3c;
using namespace std;

  string config="127.0.0.1:7000,127.0.0.1:7001";


static __thread r3c::CRedisClient* sg_redis_client = NULL; // thread level gobal variable
  void release_redis_client(void*)
  {
      delete sg_redis_client;
      sg_redis_client = NULL;
  }

r3c::CRedisClient* get_redis_client()
{
    if (NULL == sg_redis_client)
    {
        sg_redis_client = new r3c::CRedisClient(config);
    }
    return sg_redis_client;
}


  //CRedisClient crc(config);
void doSomething(int tid){
  try{
  // string config="127.0.0.1:7000,127.0.0.1:7001,127.0.0.1:7002,127.0.0.1:7003,127.0.0.1:7004,127.0.0.1:7005";
  CRedisClient*  crc=get_redis_client();
  //pthread_cleanup_push(release_redis_client, NULL);
cout<<"DP"<<__LINE__<<endl;
    string key="key";
    string value="something";
    string retval;

    cout<<"DP"<<__LINE__<<endl;
    crc->set(key, value);
cout<<"DP"<<__LINE__<<endl;
    bool g=crc->get(key, &retval);
cout<<"DP"<<__LINE__<<endl;
    bool d=crc->del(key);
cout<<"DP"<<__LINE__<<endl;

    cout<<"Get:"<<g<<endl;
    if(value!=retval){
      cout<<"retvalue :"<<retval<<endl;
    }

    g=crc->get(key, &retval);
    cout<<"Get2:"<<g<<endl;
    //pthread_cleanup_pop(1);
 } catch (CRedisException e){
    cout<<"ERROR"<<endl;
 }
}


int main(){
/*
 int i;
 int thread_count=1;
 thread threads[thread_count];
 auto fp=doSomething;
 for (i = 0; i < thread_count; i++) {
   threads[i] = thread(fp,i);
 }
 for (i = 0; i < thread_count; i++) {
   if (threads[i].joinable()) {
     threads[i].join();
   }
 }
*/
doSomething(1);



return 0;
}
