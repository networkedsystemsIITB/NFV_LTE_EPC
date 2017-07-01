/*

 To compile with:
 //leveldb
 g++ -std=c++11 -O3 KVRequest.cpp -lkvstore -lboost_serialization -pthread -lkvs_leveldb

 //Redis
 g++ -std=c++11 -O3 KVRequest.cpp -lkvstore -lboost_serialization -pthread -lkvs_redis

 //Memcached
 g++ -std=c++11 -O3 KVRequest.cpp -lkvstore -lboost_serialization -pthread -lkvs_memcached -lmemcached
*/

#include <iostream>
#include <kvstore/KVStoreHeader.h>
using namespace std;
using namespace kvstore;

int main(){
  KVRequest kr;
  KVData<string> ret;

  /* Establish connection to key-value store*/
  kr.bind("127.1.1.1:8090");

  /* Register few requests to be executed */
  kr.put<int,string>(1, "One", "MyTable1");
  kr.put<int,string>(2, "Two", "MyTable1");
  kr.put<int,string>(3, "Three", "MyTable2");

  /* Execute the requests */
  KVResultSet rs = kr.execute();

  /* Clear the request queue */
  kr.reset();

  /* Parse the result */
  for(int i = 0; i < rs.size() ; i++){
    ret = rs.get<string>(i);
    if(ret.ierr != 0) {
      cout<<"Operation "<<i<<" failed. Error:"<<ret.serr<<endl;
    }
  }

  /* Reuse the KVRequest object */
  /* Register few requests to be executed */
  kr.get<int,string>(1, "MyTable1");
  kr.get<int,string>(2, "MyTable1");
  kr.get<int,string>(3, "MyTable2");

  /* Execute the requests */
  rs = kr.execute();

  /* Clear the request queue */
  kr.reset();

  /* Parse the result */
  for(int i = 0; i < rs.size() ; i++){
    ret = rs.get<string>(i);
    if(ret.ierr != 0) {
      cout<<"Operation "<<i<<" failed. Error:"<<ret.serr<<endl;
    } else {
      cout<<"Value for operation "<<i<<" is "<<ret.value<<endl;
    }
  }

  /* We can also club multiple type of operations. */
  /* But note that the order of execution is not ensured. */
  kr.put<int,string>(10, "Ten", "MyTable1");
  kr.get<int,string>(10, "MyTable1");
  kr.del<int,string>(10, "MyTable1");

  /* Execute the requests */
  rs = kr.execute();

  /* Clear the request queue */
  kr.reset();

  /* Parse the result */
  for(int i = 0; i < rs.size() ; i++){
    ret = rs.get<string>(i);
    if(ret.ierr != 0) {
      cout<<"Operation "<<i<<" failed. Error:"<<ret.serr<<endl;
    }
  }
}
