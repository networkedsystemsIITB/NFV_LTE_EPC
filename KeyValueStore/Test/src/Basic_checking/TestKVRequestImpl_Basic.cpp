/*
  g++ -std=c++11 TestKVRequestImpl_Basic.cpp -lkvstore_v2 -lboost_serialization -pthread -lkvs_redis_v2
*/

// #define CONF string("10.129.28.44:8091")
// #define CONF string("10.129.28.141:7003")
#define TABLE string("TestTable1")

// #define JDEBUG

#include "../jutils.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <kvstore/KVStoreHeader.h>

using namespace std;
using namespace kvstore;

int main(){
  KVData<string> kd;
  vector<int> keys = {1,2,3};
  vector<string> vals = {"One","Two","Three"};
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
  KVResultSet rs = kr.execute();
  jAssert(rs.size()!=keys.size(),cout<<"Put size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
  for(int i=0;i<rs.size();i++){
    kd = rs.get<string>(i);
    jAssert(rs.oprType(i)!=OPR_TYPE_PUT, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_GET<<" got:"<<rs.oprType(i)<<endl;)
    jAssert(kd.ierr!=0, cout<<"Error in put serr:"<<kd.serr<<" for index="<<i<<endl;)
  }
  kr.reset();

  if(1==0)
  {
  /* Check successfull get */
    KVStore<int,string> ks;
    IS_REACHABLE
    bool succ = ks.bind(CONF,TABLE);
    jAssert(!succ,cout<<"Connection error"<<endl;);
    IS_REACHABLE
    KVData<string> kd;
    for(int i=0;i<keys.size();i++){
      // ks.put(keys[i],vals[i]);
      kd = ks.get(keys[i]);
      jAssert(kd.ierr!=0, cout<<" Error in get serr:"<<kd.serr<<" for index="<<i<<endl;)
      jAssert(kd.value!=vals[i], cout<<"Incorrect value for get("<<keys[i]<<") expected:"<<vals[i]<<" got:"<<kd.value<<endl;)
      TRACE(cout<<"i:"<<i<<" val:"<<kd.value<<endl;)
    }
  }

  /* Check successfull get */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    // cout<<"Keyy:"<<keys[i]<<endl;
    kr.get<int,string>(keys[i],TABLE);
  }
  rs = kr.execute();
  // cout<<"Key_sz"<<keys.size()<<"  rs_sz:"<<rs.size()<<endl;
  jAssert(rs.size()!=keys.size(),cout<<"Get size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
  for(int i=0;i<rs.size();i++){
    kd = rs.get<string>(i);
    jAssert(rs.oprType(i)!=OPR_TYPE_GET, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_GET<<" got:"<<rs.oprType(i)<<endl;)
    jAssert(kd.ierr!=0, cout<<" Error in get serr:"<<kd.serr<<" for index="<<i<<endl;)
    jAssert(kd.value!=vals[i], cout<<"Incorrect value for get("<<keys[i]<<") expected:"<<vals[i]<<" got:"<<kd.value<<endl;)
  }
  kr.reset();

  /* Check successfull del */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.del<int,string>(keys[i],TABLE);
  }
  rs = kr.execute();
  jAssert(rs.size()!=keys.size(),cout<<"Del size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
  for(int i=0;i<rs.size();i++){
    kd = rs.get<string>(i);
    jAssert(rs.oprType(i)!=OPR_TYPE_DEL, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_DEL<<" got:"<<rs.oprType(i)<<endl;)
    jAssert(kd.ierr!=0, cout<<"Error in del serr:"<<kd.serr<<" for index="<<i<<endl;)
  }
  kr.reset();

  /* Check unsuccessfull get */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.get<int,string>(keys[i],TABLE);
  }
  rs = kr.execute();
  jAssert(rs.size()!=keys.size(),cout<<"Unsuccessfull get size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
  for(int i=0;i<rs.size();i++){
    kd = rs.get<string>(i);
    jAssert(rs.oprType(i)!=OPR_TYPE_GET, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_GET<<" got:"<<rs.oprType(i)<<endl;)
    jAssert(kd.ierr==0, cout<<"Error in unsuccessfull get got:"<<kd.value<<" for index="<<i<<endl;)
  }
  kr.reset();

  /* Check unsuccessfull del */
  IS_REACHABLE
  for(int i=0;i<keys.size();i++){
    kr.del<int,string>(keys[i],TABLE);
  }
  rs = kr.execute();
  jAssert(rs.size()!=keys.size(),cout<<"Unsuccessfull del size mismatch expected:"<<keys.size()<<" got:"<<rs.size()<<endl;)
  for(int i=0;i<rs.size();i++){
    kd = rs.get<string>(i);
    jAssert(rs.oprType(i)!=OPR_TYPE_DEL, cout<<"Incorrect operation type for index="<<i<<" expected:"<<OPR_TYPE_DEL<<" got:"<<rs.oprType(i)<<endl;)
    jAssert(kd.ierr==0, cout<<"Error in unsuccessfull del for index="<<i<<endl;)
  }
  kr.reset();

  cout<<"All testcases passed successfully for "<<__FILE__<<"."<<endl;
  return 0;
}
