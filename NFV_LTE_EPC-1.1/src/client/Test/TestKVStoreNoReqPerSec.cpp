#include "../KVStore.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace kvstore;
using namespace std;
using namespace std::chrono;

void doSomething(int tid){
  const int iter = 5000;

  auto t1 = high_resolution_clock::now();
  KVStore<int,string> k;
  //SocketAddress,TableName
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object
  for(int i=0;i<iter;i++){
    k.put(tid*10000+i,"Om Nama Shivay "+to_string(i));
    auto kd = k.get(tid*10000+i);
    if(kd.ierr==-1){
      cout<<"Error in thread "<<tid<<" at i="<<i<<endl;//" Got data:"<<kd.value<<endl;
    } else {
      //cout<<"Got Data:"<<kd.value<<" for tid:"<<tid<<" i:"<<i<<endl;
    }
    k.del(tid*10000+i);
    kd = k.get(tid*10000+i);
    if(kd.ierr!=-1){
      cout<<"Error in thread "<<tid<<" at i="<<i<<" Got data:"<<kd.value<<endl;
    }
  }
  auto t2 = high_resolution_clock::now();
  double dur = duration_cast<microseconds>(t2 -t1).count();
  cout<<"TID:"<<tid<<" duration "<<dur<<" microsecond for "<<iter<<" iterrations."<<endl;
}

void doIndependentWrites(int tid){
  const int iter = 5000;

  auto t1 = high_resolution_clock::now();
  KVStore<int,string> k;
  //SocketAddress,TableName
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object
  for(int i=0;i<iter;i++){
    k.put(tid*10000+i,"Om Nama Shivay "+to_string(i));
  }
  auto t2 = high_resolution_clock::now();
  double dur = duration_cast<microseconds>(t2 -t1).count();
  cout<<"doIndependentWrites TID:"<<tid<<" duration "<<dur<<" microsecond for "<<iter<<" iterrations."<<endl;
}

void doWritesOnSameObjects(int tid){
  const int iter = 5000;

  auto t1 = high_resolution_clock::now();
  KVStore<int,string> k;
  //SocketAddress,TableName
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object
  for(int i=0;i<iter;i++){
    k.put(i,"Om Nama Shivay "+to_string(i));
  }
  auto t2 = high_resolution_clock::now();
  double dur = duration_cast<microseconds>(t2 -t1).count();
  cout<<"doWritesOnSameObjects TID:"<<tid<<" duration "<<dur<<" microsecond for "<<iter<<" iterrations."<<endl;
}


void doIndependentReads(int tid){
  const int iter = 5000;

  auto t1 = high_resolution_clock::now();
  KVStore<int,string> k;
  //SocketAddress,TableName
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object
  for(int i=0;i<iter;i++){
    auto kd = k.get(tid*10000+i);
    if(kd.ierr==-1){
      cout<<"doIndependentReads Error in thread "<<tid<<" at i="<<i<<endl;//" Got data:"<<kd.value<<endl;
    }
  }
  auto t2 = high_resolution_clock::now();
  double dur = duration_cast<microseconds>(t2 -t1).count();
  cout<<"doIndependentReads TID:"<<tid<<" duration "<<dur<<" microsecond for "<<iter<<" iterrations."<<endl;
}




void doReadsOnSameObject(int tid){
  const int iter = 5000;

  auto t1 = high_resolution_clock::now();
  KVStore<int,string> k;
  //SocketAddress,TableName
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object
  for(int i=0;i<iter;i++){
    auto kd = k.get(i);
    if(kd.ierr==-1){
      cout<<"doReadsOnSameObject Error in thread "<<tid<<" at i="<<i<<endl;//" Got data:"<<kd.value<<endl;
    }
  }
  auto t2 = high_resolution_clock::now();
  double dur = duration_cast<microseconds>(t2 -t1).count();
  cout<<"doReadsOnSameObject TID:"<<tid<<" duration "<<dur<<" microsecond for "<<iter<<" iterrations."<<endl;
}

int main(){
  //const int thread_count = 70; //to be passed during compile time with -D option -Dthread_count=70
  int i;
  thread threads[thread_count];

  {
    auto fp = doIndependentWrites;
    string fname = "doIndependentWrites";
    auto t1 = high_resolution_clock::now();
    for (i = 0; i < thread_count; i++) {
      threads[i] = thread(fp,i);
    }
    for (i = 0; i < thread_count; i++) {
      if (threads[i].joinable()) {
        threads[i].join();
      }
    }
    auto t2 = high_resolution_clock::now();
    double dur = duration_cast<microseconds>(t2 -t1).count();
    cout<<"Total time for "<<thread_count<<" "<<fname<<" threads is "<<dur<<" microsecond"<<endl<<endl;
  }

  {
    auto fp = doWritesOnSameObjects;
    string fname = "doWritesOnSameObjects";
    auto t1 = high_resolution_clock::now();
    for (i = 0; i < thread_count; i++) {
      threads[i] = thread(fp,i);
    }
    for (i = 0; i < thread_count; i++) {
      if (threads[i].joinable()) {
        threads[i].join();
      }
    }
    auto t2 = high_resolution_clock::now();
    double dur = duration_cast<microseconds>(t2 -t1).count();
    cout<<"Total time for "<<thread_count<<" "<<fname<<" threads is "<<dur<<" microsecond"<<endl<<endl;
  }


  {
    auto fp = doIndependentReads;
    string fname = "doIndependentReads";
    auto t1 = high_resolution_clock::now();
    for (i = 0; i < thread_count; i++) {
      threads[i] = thread(fp,i);
    }
    for (i = 0; i < thread_count; i++) {
      if (threads[i].joinable()) {
        threads[i].join();
      }
    }
    auto t2 = high_resolution_clock::now();
    double dur = duration_cast<microseconds>(t2 -t1).count();
    cout<<"Total time for "<<thread_count<<" "<<fname<<" threads is "<<dur<<" microsecond"<<endl<<endl;
  }

  {
    auto fp = doReadsOnSameObject;
    string fname = "doReadsOnSameObject";
    auto t1 = high_resolution_clock::now();
    for (i = 0; i < thread_count; i++) {
      threads[i] = thread(fp,i);
    }
    for (i = 0; i < thread_count; i++) {
      if (threads[i].joinable()) {
        threads[i].join();
      }
    }
    auto t2 = high_resolution_clock::now();
    double dur = duration_cast<microseconds>(t2 -t1).count();
    cout<<"Total time for "<<thread_count<<" "<<fname<<" threads is "<<dur<<" microsecond"<<endl<<endl;
  }

  return 0;
}
