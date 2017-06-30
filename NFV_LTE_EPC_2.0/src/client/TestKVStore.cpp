#include "KVStore.h"
#include<iostream>
#include<thread>

using namespace kvstore;
using namespace std;

void doSomthing(){
  KVStore<int,string> k;
  //SocketAddress,TableName
  //k.bind("10.129.26.154:8090","ShreeGanesh"); //Exactly once for each KVStore object
  k.bind("127.1.1.1:8090","ShreeGanesh"); //Exactly once for each KVStore object

  cout<<"First get()"<<endl;
  auto kvd = k.get(11); //You can also use KVData<string> insted of auto
  if(kvd.ierr<0){
    cout<<"Error in getting data"<<endl<<kvd.serr<<endl;
  } else {
    cout<<"Got Data : "<<kvd.value<<endl<<"Size:"<<kvd.value.size()<<endl;
  }

  cout<<endl<<"First put()"<<endl;
  kvd = k.put(10,"Om Nama Shivay");
  if(kvd.ierr<0){
    cout<<"Error in putting data"<<endl<<kvd.serr<<endl;
  } else {
    cout<<"Data written successfully"<<endl;
  }

  cout<<endl<<"get() after put"<<endl;
  kvd = k.get(10);
  if(kvd.ierr<0){
    cout<<"Error in getting data"<<endl<<kvd.serr<<endl;
  } else {
    cout<<"Got Data : "<<kvd.value<<endl;
  }

  // cout<<endl<<"get() after clear"<<endl;
  //k.clear();
  cout<<endl<<"get() after del"<<endl;
  k.del(10);
  kvd = k.get(10);
  if(kvd.ierr<0){
    cout<<"Error in getting data"<<endl<<kvd.serr<<endl;
  } else {
    cout<<"Data : "<<kvd.value<<endl;
  }

  cout<<endl<<"Second put() for next run"<<endl;
  string str="Some data from old writes in this table.";
  for(int i=0;i<3000;i++){
    str+="1234567890";
  }
  str+="abcd";
  kvd = k.put(11,str);
  if(kvd.ierr<0){
    cout<<"Error in putting data"<<endl<<kvd.serr<<endl;
  } else {
    cout<<"Data written successfully"<<endl;
  }
}

int main(){
  const int thread_count = 1;
  int i;
  thread threads[thread_count];
  for (i = 0; i < thread_count; i++) {
  		threads[i] = thread(doSomthing);
  	}
  	for (i = 0; i < thread_count; i++) {
  		if (threads[i].joinable()) {
  			threads[i].join();
  		}
  }
  return 0;
}
