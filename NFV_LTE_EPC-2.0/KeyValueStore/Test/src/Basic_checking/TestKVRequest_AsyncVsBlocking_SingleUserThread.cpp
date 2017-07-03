/*
  g++ -std=c++11 TestKVRequest_AsyncVsBlocking_SingleUserThread.cpp -lkvstore_v2 -lboost_serialization -pthread -lkvs_redis_v2
  NOTE :
  Note that here we are using single user thread, but async implementation may
  (may not) use multiple threads inside.
*/


// #define CONF string("127.0.0.1:8091")
// #define CONF string("10.129.28.44:8091")
// #define CONF string("10.129.28.141:7003")
#define TABLE string("TestTable123")
#define OPERATION_COUNT 10000
#define LOCAL_OPERATION_COUNT 3
#define READ_PROBABILITY 0.5
#define DATA_SET_SIZE 10000
#define KEY_SIZE 30
#define VALUE_SIZE 2000

// #define JDEBUG

#include "../jutils.h"
#include "../TestUtils.h"
#include "../AutomatedBenchmarking/DataSetGenerator.cpp"
#include "../AutomatedBenchmarking/RandomNumberGenerator.cpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <kvstore/KVStoreHeader.h>

using namespace std;
using namespace kvstore;

struct data_holder {
  long long opr_count;
  long long failure_count;
};

void callback_handler(void *data,KVData<string> kd){
  struct data_holder *dh = (struct data_holder *)data;
  dh->opr_count++;
  // cout<<dh->opr_count<<endl;
  if(kd.ierr != 0){
    dh->failure_count++;
  } else {
    // cout<<"Val:"<<kd.value<<endl;
  }
}

void async_execute_handler(void *data,KVResultSet rs){
  struct data_holder *dh = (struct data_holder *)data;
  dh->opr_count++;
  for(int i=0;i<LOCAL_OPERATION_COUNT;i++){
    if(rs.get<string>(i).ierr != 0){
      dh->failure_count++;
    }
  }
  // cout<<"Done:"<<dh->opr_count<<endl;
  TRACE(if(dh->opr_count%1000==0){cout<<"Done:"<<dh->opr_count<<endl;})
}


int main(){
  RandomNumberGenerator rng(0,DATA_SET_SIZE-1);
  cout<<"DATA_SET_SIZE:"<<DATA_SET_SIZE<<endl;
  shared_ptr<KVData<string>> kd;
  vector<string> keys = DataSetGenerator::getRandomStrings(DATA_SET_SIZE,KEY_SIZE);
  vector<string> vals = DataSetGenerator::getRandomStrings(DATA_SET_SIZE,VALUE_SIZE);
  // cout<<vals[0].size()<<endl;
  TRACE(cout<<"Initialized"<<endl);


  /* Load Data */
  KVStore<string,string> ks;
  bool succ = ks.bind(CONF,TABLE);
  jAssert(!succ,cout<<"Connection error for KVStore (loading data)."<<endl;);

  struct data_holder dh = {0,0};
  for(long long i = 0; i<DATA_SET_SIZE; i++){
    ks.async_put(keys[i],vals[i],callback_handler,&dh);
    // cout<<"In"<<i<<endl;
  }
  while(dh.opr_count!=DATA_SET_SIZE){usleep(10000);}
  jAssert(dh.failure_count!=0, cout<<"Failure loading data"<<endl;)
  TRACE(cout<<"Loading successfull"<<endl);
  cout<<"Loading successfull"<<endl;

  // sleep(2);
  /* Create connection */
  KVRequest kr;
  succ = kr.bind(CONF);
  jAssert(!succ,cout<<"Connection error"<<endl;);
  TRACE(cout<<"Connection successfull"<<endl);

  /* Test Blocking call */
  // if(1==0) //------------------------------
  {
    int local_opr_count = LOCAL_OPERATION_COUNT;
    long long opr_count = OPERATION_COUNT;
    int r1;
    int r2;
    long long failure_count = 0;
    double read_prob = READ_PROBABILITY * RAND_MAX;
    long long start_time = currentMicros();
    while(opr_count){
      for(int i=0;i<local_opr_count;i++){
        r1 = rand();
        r2 = rng.zipf();
        if(r1<read_prob){
          kr.get<string,string>(keys[r2],TABLE);
        } else {
          kr.put<string,string>(keys[r2],vals[r2],TABLE);
        }
      }
      KVResultSet rs = kr.execute();
      kr.reset();
      for(int i=0;i<local_opr_count;i++){
        if(rs.get<string>(i).ierr != 0){
          failure_count++;
        }
      }
      opr_count--;
      // cout<<"Opr :"<<opr_count<<" FC:"<<failure_count<<endl;
      TRACE(if(opr_count%500 == 0){
        cout<<opr_count<<endl;
      })
    }
    long long end_time = currentMicros();
    long long total_time = end_time - start_time;
    cout<<"Blocking Call Stats:"<<endl;
    cout<<"Total time taken :"<<total_time/(1e6)<<" seconds"<<endl;
    cout<<"Throughput :"<< OPERATION_COUNT / (total_time/(1e6)) << " ops"<<endl;
    cout<<"Operation count:"<<(long long) (OPERATION_COUNT)<<endl;
    cout<<"Failures :"<<failure_count<<endl;
  }


  /* Test Async call */
  // if(1==0)
  {
    struct data_holder dh = {0,0};

    int local_opr_count = LOCAL_OPERATION_COUNT;
    long long opr_count = OPERATION_COUNT;
    int r1;
    int r2;
    long long failure_count = 0;
    double read_prob = READ_PROBABILITY * RAND_MAX;
    long long start_time = currentMicros();
    while(opr_count){
      for(int i=0;i<local_opr_count;i++){
        r1 = rand();
        r2 = rng.zipf();
        if(r1<read_prob){
          kr.get<string,string>(keys[r2],TABLE);
        } else {
          kr.put<string,string>(keys[r2],vals[r2],TABLE);
        }
      }
      kr.async_execute(async_execute_handler,&dh);
      kr.reset();
      opr_count--;
      TRACE(if(opr_count%500 == 0){
        cout<<opr_count<<endl;
      })
    }
    /*wait while all callbacks are completed*/
    TRACE(cout<<"Waiting for callbacks"<<endl;)
    cout<<"Waiting for callbacks"<<endl;
    while(dh.opr_count!=OPERATION_COUNT){usleep(1000);}
    long long end_time = currentMicros();
    long long total_time = end_time - start_time;
    cout<<"Async Call Stats:"<<endl;
    cout<<"Total time taken :"<<total_time/(1e6)<<" seconds"<<endl;
    cout<<"Throughput :"<< OPERATION_COUNT / (total_time/(1e6)) << " ops"<<endl;
    cout<<"Operation count:"<<(long long) (OPERATION_COUNT)<<endl;
    cout<<"Failures :"<<dh.failure_count<<endl;
  }

  return 0;
}
