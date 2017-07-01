/*
 To compile with:
 //leveldb
 g++ -std=c++11 -O3 KVStore.cpp -lkvstore -lboost_serialization -pthread -lkvs_leveldb

 //Redis
 g++ -std=c++11 -O3 KVStore.cpp -lkvstore -lboost_serialization -pthread -lkvs_redis

 //Memcached
 g++ -std=c++11 -O3 KVStore.cpp -lkvstore -lboost_serialization -pthread -lkvs_memcached -lmemcached
*/

#include <iostream>
#include <kvstore/KVStoreHeader.h>
using namespace std;
using namespace kvstore;

int main(){
  /* Declare the KVStore object with KeyType and ValType */
  KVStore<int,string> ks;

  /* Establish connection to key-value store*/
  bool succ = ks.bind("tcp:host=10.129.28.186,port=11100","MyTable");
  if(succ){
    cout<<"Connection successful."<<endl;
  } else {
    cout<<"Problem connecting."<<endl;
    return -1;
  }

  /* Storage for return value */
  KVData<string> ret;

  /* Put a key in key-value store */
  ret = ks.put(1, "One");
  if(ret.ierr == 0){
    cout<<"Put successful."<<endl;
  } else {
    cout<<"Problem with put. Error Description:"<<ret.serr<<endl;
  }

  /* Get a value from key-value store */
  ret = ks.get(1);
  if(ret.ierr == 0){
    cout<<"Get successful. We got the value "<<ret.value<<endl;
  } else {
    cout<<"Problem with get. Error Description:"<<ret.serr<<endl;
  }

  /* Delete a value from key-value store */
  ret = ks.del(1);
  if(ret.ierr == 0){
    cout<<"Del successful."<<endl;
  } else {
    cout<<"Problem with del. Error Description:"<<ret.serr<<endl;
  }
}
