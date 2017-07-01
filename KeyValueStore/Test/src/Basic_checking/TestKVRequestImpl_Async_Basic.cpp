/*
  g++ -std=c++11 TestKVRequestImpl_Async_Basic.cpp -lkvstore_v2 -lboost_serialization -pthread -lkvs_redis_v2
*/

// #define CONF string("10.129.28.44:8091")
// #define CONF string("10.129.28.141:7003")
#define TABLE string("TestTable123")

// #define JDEBUG

#include "../jutils.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <kvstore/KVStoreHeader.h>

using namespace std;
using namespace kvstore;

#define KEYS vector<int> keys = {1,2,3};
#define VALUES vector<string> vals = {"One","Two","Three"};
// #define KEYS vector<int> keys = {4,5};
// #define VALUES vector<string> vals = {"Four","Five"};

int main(){
  KEYS
  VALUES
  // int sz = keys.size();

  /* Create connection */
  KVRequest kr;
  IS_REACHABLE
  bool succ = kr.bind(CONF);
  jAssert(!succ,cout<<"Connection error"<<endl;);

  /* Check successfull put */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.put<int,string>(keys[i],vals[i],TABLE);
  }
  IS_REACHABLE
  auto lambda_fn = [](KVResultSet rs){
    KEYS
    VALUES
    cout<<"Put lambda called"<<endl;
    KVData<string> kd;
    jAssert(rs.size()!=keys.size(),cout<<"Put size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
    for(int i=0;i<rs.size();i++){
      kd = rs.get<string>(i);
      jAssert(rs.oprType(i)!=OPR_TYPE_PUT, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_GET<<" got:"<<rs.oprType(i)<<endl;)
      jAssert(kd.ierr!=0, cout<<"Error in put serr:"<<kd.serr<<" for index="<<i<<endl;)
    }
  };
  IS_REACHABLE
  kr.async_execute(lambda_fn);
  IS_REACHABLE
  kr.reset();
  IS_REACHABLE

  sleep(1);


// {
//
//   KVStore<int,string> kr;
//   IS_REACHABLE
//   bool succ = kr.bind(CONF,TABLE);
//   jAssert(!succ,cout<<"Connection error"<<endl;);
//   auto res=kr.get(keys[0]);
//   if(res.ierr != 0){
//     cout<<"Err:"<<res.serr<<endl;
//   } else {
//     cout<<"Value:"<<res.value<<endl;
//   }
// }
  /* Check successfull get */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.get<int,string>(keys[i],TABLE);
  }
  kr.async_execute([](KVResultSet rs){
    KEYS
    VALUES
    KVData<string> kd;
    jAssert(rs.size()!=keys.size(),cout<<"Get size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
    for(int i=0;i<rs.size();i++){
      kd = rs.get<string>(i);
      jAssert(rs.oprType(i)!=OPR_TYPE_GET, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_GET<<" got:"<<rs.oprType(i)<<endl;)
      jAssert(kd.ierr!=0, cout<<" Error in get serr:"<<kd.serr<<" for index="<<i<<endl;)
      jAssert(kd.value!=vals[i], cout<<"Incorrect value for get("<<keys[i]<<") expected:"<<vals[i]<<" got:"<<kd.value<<endl;)
    }
  });
  kr.reset();
  sleep(1);

  /* Check successfull del */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.del<int,string>(keys[i],TABLE);
  }
  kr.async_execute([&](KVResultSet rs){
    KEYS
    VALUES
    KVData<string> kd;
    jAssert(rs.size()!=keys.size(),cout<<"Del size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
    for(int i=0;i<rs.size();i++){
      kd = rs.get<string>(i);
      jAssert(rs.oprType(i)!=OPR_TYPE_DEL, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_DEL<<" got:"<<rs.oprType(i)<<endl;)
      jAssert(kd.ierr!=0, cout<<"Error in del serr:"<<kd.serr<<" for index="<<i<<endl;)
    }
  });
  kr.reset();
  sleep(1);

  /* Check unsuccessfull get */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.get<int,string>(keys[i],TABLE);
  }
  kr.async_execute([&](KVResultSet rs){
    KEYS
    VALUES
    KVData<string> kd;
    jAssert(rs.size()!=keys.size(),cout<<"Unsuccessfull get size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
    for(int i=0;i<rs.size();i++){
      kd = rs.get<string>(i);
      jAssert(rs.oprType(i)!=OPR_TYPE_GET, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_GET<<" got:"<<rs.oprType(i)<<endl;)
      jAssert(kd.ierr==0, cout<<"Error in unsuccessfull get got:"<<kd.value<<" for index="<<i<<endl;)
    }
  });
  kr.reset();
  sleep(1);

  /* Check unsuccessfull del */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.del<int,string>(keys[i],TABLE);
  }
  kr.async_execute([&](void *p, KVResultSet rs){
    KEYS
    VALUES
    KVData<string> kd;
    jAssert(rs.size()!=keys.size(),cout<<"Unsuccessfull del size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
    for(int i=0;i<rs.size();i++){
      kd = rs.get<string>(i);
      jAssert(rs.oprType(i)!=OPR_TYPE_DEL, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_DEL<<" got:"<<rs.oprType(i)<<endl;)
      jAssert(kd.ierr==0, cout<<"Error in unsuccessfull del for index="<<i<<endl;)
    }
  },(void*)NULL);
  kr.reset();
  sleep(1);

  // sleep(5); /*Wait for async to complete;*/
  cout<<"All testcases passed successfully for "<<__FILE__<<"."<<endl;
  return 0;
}
