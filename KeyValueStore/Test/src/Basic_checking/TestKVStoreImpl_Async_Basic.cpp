/*
  g++ -std=c++11 TestKVStoreImpl_Async_Basic.cpp -lkvstore_v2 -lboost_serialization -pthread -lkvs_redis_v2
*/

// #define CONF string("10.129.28.44:8091")
// #define CONF string("10.129.28.141:7003")
#define TABLE string("TestTable123")

// #define JDEBUG

#include "../jutils.h"
#include <iostream>
#include <cassert>
#include <kvstore/KVStoreHeader.h>

using namespace std;
using namespace kvstore;

// void successfullGetCallBack(KVData<string> r){
//   cout<<"DP:"<<" file:"<<__FILE__<<" line:"<<__LINE__<<endl;
// }

void successfullGetCallBack(void *data,KVData<string> r){
  // cout<<"DP:"<<" file:"<<__FILE__<<" line:"<<__LINE__<<endl;
  jAssert(r.ierr!=0, cout<<" Error in get(1):"<<r.serr<<" called from line "<<*((int*)data)<<endl;)
  jAssert(r.value!="One", cout<<"Incorrect value from get(1) got:"<<r.value<<" called from line "<<*((int*)data)<<endl;)
  TRACE(cout<<"successfull get :"<<r.value<<endl;)
}

void unsuccessfullGetCallBack(void *data,KVData<string> r){
  // cout<<"DP:"<<" file:"<<__FILE__<<" line:"<<__LINE__<<endl;
  jAssert(r.ierr==0, cout<<" Error in unsuccessfull get(1) got:"<<r.value<<" called from line "<<*((int*)data)<<endl;)
  TRACE(cout<<"unsuccessfull get."<<endl;)
}

void successfullCallBack(void *data,KVData<string> r){
  jAssert(r.ierr!=0, cout<<"Serr:"<<r.serr<<" called from line "<<*((int*)data)<<endl;)
  TRACE(cout<<"successfull callback."<<endl;)
}

void unsuccessfullCallBack(void *data,KVData<string> r){
  jAssert(r.ierr==0, cout<<"Error in unsuccessfull del called from line "<<*((int*)data)<<endl;)
  TRACE(cout<<"unsuccessfull callback."<<endl;)
}


int main(){
  KVData<string> r;

  /* Create connection */
  KVStore<int,string> ks;
  IS_REACHABLE
  bool succ = ks.bind(CONF,TABLE);
  jAssert(!succ,cout<<"Connection error"<<endl;);

  /* Check successfull put */
  IS_REACHABLE
  int line_no1 = __LINE__; ks.async_put(1,"One",successfullCallBack,&line_no1);
  // sleep(1); /*wait for callbacks to complete*/

  /* Check successfull get */
  IS_REACHABLE
  // int line_no2 = __LINE__; ks.async_get(1,successfullGetCallBack);
  int line_no2 = __LINE__; ks.async_get(1,successfullGetCallBack,&line_no2);
  // sleep(1); /*wait for callbacks to complete*/

  /* Check successfull del */
  IS_REACHABLE
  int line_no3 = __LINE__; ks.async_del(1,successfullCallBack,&line_no3);
  // sleep(1); /*wait for callbacks to complete*/

  /* Check unsuccessfull get */
  IS_REACHABLE
  int line_no4 = __LINE__; ks.async_get(1,unsuccessfullGetCallBack,&line_no4);
  // sleep(1); /*wait for callbacks to complete*/

  /* Check unsuccessfull del */
  IS_REACHABLE
  int line_no5 = __LINE__; ks.async_del(1,unsuccessfullCallBack,&line_no5);

  sleep(5); /*wait for callbacks to complete*/
  cout<<"All testcases passed successfully for "<<__FILE__<<"."<<endl;
  return 0;
}
