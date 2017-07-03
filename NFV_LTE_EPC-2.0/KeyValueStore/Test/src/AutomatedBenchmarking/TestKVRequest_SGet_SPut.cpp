/*
  g++ -std=c++11 TestKVStore_AsyncVsBlocking_SingleUserThread.cpp -lkvstore_v2 -lboost_serialization -pthread -lkvs_redis_v2
  NOTE :
  Note that here we are using single user thread, but async implementation may
  (may not) use multiple threads inside.
*/


// #define CONF string("127.0.0.1:8091")
// #define CONF string("10.129.28.44:8091")
// #define CONF string("10.129.28.141:7003")
// #define CONF string("127.1.1.1:8090")

#define TABLE string("TestTable123")
#define RUN_TIME 10
//#define THREAD_COUNT 50
#define CONTENTION false
#define DATA_SET_SIZE 500
#define KEY_SIZE 30
#define VALUE_SIZE 2000

bool run;

//#define JDEBUG

#include "../jutils.h"
#include "../TestUtils.h"
#include "../AutomatedBenchmarking/DataSetGenerator.cpp"
#include "../AutomatedBenchmarking/RandomNumberGenerator.cpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <kvstore/KVStoreHeader_v2.h>

using namespace std;
using namespace kvstore;

int main(){
  thread td[THREAD_COUNT];

  RandomNumberGenerator rng(0,DATA_SET_SIZE-1);
  cout<<"DATA_SET_SIZE:"<<DATA_SET_SIZE<<endl;
  KVData<string> kd;
  vector<string> keys = DataSetGenerator::getRandomStrings(DATA_SET_SIZE,KEY_SIZE);
  vector<string> vals = DataSetGenerator::getRandomStrings(DATA_SET_SIZE,VALUE_SIZE);
  // cout<<vals[0].size()<<endl;
  TRACE(cout<<"Initialized"<<endl);


  /* Create connection */
  KVRequest ks[THREAD_COUNT];
  bool succ;
  for(int i=0;i<THREAD_COUNT;i++){
    succ = ks[i].bind(CONF);
    jAssert(!succ,cout<<"Connection error"<<endl;);
  }
  TRACE(cout<<"Connection successfull"<<endl);

  /* Load Data */
  if(CONTENTION){
    KVStore<string,string> kss;
    succ = kss.bind(CONF,TABLE);
    jAssert(!succ,cout<<"Connection error"<<endl;);
    for(long long i = 0; i<DATA_SET_SIZE; i++){
      kd = kss.put(keys[i],vals[i]);
      if(kd.ierr != 0){
        cerr<<"Error loading data"<<endl;
      }
    }
  } else {
    KVStore<string,string> kss;
    succ = kss.bind(CONF,TABLE);
    jAssert(!succ,cout<<"Connection error"<<endl;);
    for(long long i = 0; i<DATA_SET_SIZE; i++){
      for(int ii=0; ii<THREAD_COUNT; ii++){
        kd = kss.put(keys[i]+to_string(ii),vals[i]);
        if(kd.ierr != 0){
          cerr<<"Error loading data"<<endl;
        }
      }
    }
  }
  TRACE(cout<<"Loading successfull"<<endl);
  cout<<"Loading successfull"<<endl;

//     /* Load Data */
// {
//     vector<string> keys = DataSetGenerator::getRandomStrings(DATA_SET_SIZE,KEY_SIZE);
//     vector<string> vals = DataSetGenerator::getRandomStrings(DATA_SET_SIZE,VALUE_SIZE);
//     for(long long i = 0; i<DATA_SET_SIZE; i++){
//       kd = ks.put(keys[i],vals[i]);
//       if(kd.ierr != 0){
//         cerr<<"Error2 loading data"<<endl;
//       }
//     }
//     cout<<"Loading2 successfull"<<endl;
// }

  /* Test Normal Get Set */
  // if(1==0) //------------------------------
  {
    vector<long long> opr_count(THREAD_COUNT,0);
    vector<long long> failure_count(THREAD_COUNT,0);
    run = false;
    for(int i=0; i<THREAD_COUNT; i++){
      td[i] = thread([&,i]{
        int r1;
        int r2;
        string append;
        if(CONTENTION){
          append = "";
        } else {
          append = to_string(i);
        }
        int id = i;
        KVData<string> kd;
        while(!run);
        while(run){
          // r1 = rand();
          r2 = rng.zipf();
          // r2 = rng.uniform();
          ks[id].get<string,string>(keys[r2]+append,TABLE);
          KVResultSet kr = ks[id].execute();
          ks[id].reset();
          kd = kr.get<string>(0);
          if(kd.ierr != 0){
            failure_count[id]++;
          }
          ks[id].put<string,string>(keys[r2]+append,vals[r2],TABLE);
          kr = ks[id].execute();
          ks[id].reset();
          kd = kr.get<string>(0);
          if(kd.ierr != 0){
            failure_count[id]++;
          }
          opr_count[id]++;
          TRACE(if(opr_count[id]%1000 == 0){
            cout<<opr_count[id]<<endl;
          })
        }
      });
    }
    run = true;
    sleep(RUN_TIME);
    run=false;
    long long tcount = 0, fcount = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
      if (td[i].joinable()) {
        td[i].join();
      }
      tcount += opr_count[i];
      fcount += failure_count[i];
    }
    cout<<"Normal Call Stats:"<<endl;
    cout<<"Throughput :"<< tcount / RUN_TIME << " ops"<<endl;
    cout<<"Operation count:"<<(long long) (tcount)<<endl;
    cout<<"Failures :"<<fcount<<endl;
  }


    /* Test Safe call */
    // if(1==0) //------------------------------
    {
      vector<long long> opr_count(THREAD_COUNT,0);
      vector<long long> failure_count(THREAD_COUNT,0);
      run = false;
      for(int i=0; i<THREAD_COUNT; i++){
        td[i] = thread([&,i]{
          int r1;
          int r2;
          // cout<<"Tid:"<<this_thread::get_id()<<" i:"<<i<<endl;
          string append;
          if(CONTENTION){
            append = "";
          } else {
            append = to_string(i);
          }
          // cout<<"ID:"<<i<<endl;
          int id = i;
          KVData<string> kd;
          // KVResultSet kr;
          while(!run);
          while(run){
            r2 = rng.zipf();
            // r2 = rng.uniform();
            ks[id].sget<string,string>(keys[r2]+append,TABLE);
            KVResultSet kr = ks[id].execute();
            ks[id].reset();
            kd = kr.get<string>(0);
            if(kd.ierr != 0){
              failure_count[id]++;
              cout<<"Err:"<<kd.serr<<endl;
            }
            ks[id].sput<string,string>(keys[r2]+append,vals[r2],TABLE);
            kr = ks[id].execute();
            ks[id].reset();
            kd = kr.get<string>(0);
            if(kd.ierr != 0){
              failure_count[id]++;
              cout<<"Key:"<<keys[r2]+append<<endl;
              cout<<"Err:"<<kd.serr<<endl;
            }
            opr_count[id]++;
            TRACE(if(opr_count[id]%1000 == 0){
              cout<<opr_count[id]<<endl;
            })
          }
        });
      }
      run = true;
      sleep(RUN_TIME);
      run=false;
      long long tcount = 0, fcount = 0;
      for (int i = 0; i < THREAD_COUNT; i++) {
        if (td[i].joinable()) {
          td[i].join();
        }
        tcount += opr_count[i];
        fcount += failure_count[i];
      }
      cout<<"Safe Call Stats:"<<endl;
      cout<<"Throughput :"<< tcount / RUN_TIME << " ops"<<endl;
      cout<<"Successfull Throughput :"<< (tcount-fcount) / RUN_TIME << " ops"<<endl;
      cout<<"Operation count:"<<(long long) (tcount)<<endl;
      cout<<"Failures :"<<fcount<<endl;
    }
  return 0;
}
