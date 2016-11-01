#include "KVStore.h"
#include<iostream>
#include<thread>
#include <chrono>

using namespace kvstore;
using namespace std;
using namespace std::chrono;

void doSomthing(int tid){
  auto t1 = high_resolution_clock::now();
  KVStore<int,string> k;
  //SocketAddress,TableName
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object
  for(int i=0;i<5000;i++){
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
  cout<<"TID:"<<tid<<" duration "<<dur<<" microsecond"<<endl;
}


int main(){
  //const int thread_count = 70; //to be passed during compile time with -D option -Dthread_count=70
  int i;
  thread threads[thread_count];
  for (i = 0; i < thread_count; i++) {
  		threads[i] = thread(doSomthing,i);
  	}
  	for (i = 0; i < thread_count; i++) {
  		if (threads[i].joinable()) {
  			threads[i].join();
  		}
  }
  return 0;
}
