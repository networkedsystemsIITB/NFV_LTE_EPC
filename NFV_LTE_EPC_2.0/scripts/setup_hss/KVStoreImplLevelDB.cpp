// Why both implementation and declaration are in same file
// Refer : https://isocpp.org/wiki/faq/templates
//        http://stackoverflow.com/questions/3040480/c-template-function-compiles-in-header-but-not-implementation
#include "KVStoreHeader.h"

#ifndef __KVSTORE_DEF__
#define __KVSTORE_DEF__

#include "kvstore_client.cpp"
#include <vector>

#define c_kvsclient ((KVStoreClient *)kvsclient)

namespace kvstore {

  template<typename KeyType, typename ValType>
  KVStore<KeyType,ValType>::KVStore(){
  }

  template<typename KeyType, typename ValType>
  bool KVStore<KeyType,ValType>::bind(string connection,string tablename){
    int colon = connection.find(":");
    string ip = connection.substr(0,colon);
    string port = connection.substr(colon+1);
    kvsclient = (void *) new KVStoreClient(ip,stoi(port));

    std::vector<string> v;
    v.push_back("CreateTable");
    v.push_back(tablename);
    c_kvsclient->send(v);
    v=c_kvsclient->receive();
    if(v[0].compare("true")==0)
    {
      return true;
    }
    return false;
  }

  template<typename KeyType, typename ValType>
  KVStore<KeyType,ValType>::~KVStore(){
    delete(c_kvsclient); //? close connection
  }

  template<typename KeyType, typename ValType>
  KVData<ValType>  KVStore<KeyType,ValType>::get(KeyType const& key){
    //val,serr,ierr
    string skey=toBoostString(key);
    std::vector<string> v;
    v.push_back("Get");
    v.push_back(skey);
    c_kvsclient->send(v);

    KVData<ValType> kvd;
    v=c_kvsclient->receive();
    kvd.serr = v[1];
    kvd.ierr = stoi(v[2]);
    if(kvd.ierr==0){
      kvd.value = toBoostObject<ValType>(v[0]);
    }
    return kvd;
  }

  template<typename KeyType, typename ValType>
  KVData<ValType> KVStore<KeyType,ValType>::put(KeyType const& key,ValType const& val){
    //cout<<"PUT"<<endl;
        string skey=toBoostString(key);
        string sval=toBoostString(val);
        std::vector<string> v;
        v.push_back("Put");
        v.push_back(skey);
        v.push_back(sval);
        c_kvsclient->send(v);

        KVData<ValType> kvd;
        v=c_kvsclient->receive();
        kvd.serr = v[1];
        kvd.ierr = stoi(v[2]);
        return kvd;
  }

  template<typename KeyType, typename ValType>
  KVData<ValType> KVStore<KeyType,ValType>::del(KeyType const& key){
      string skey=toBoostString(key);
      std::vector<string> v;
      v.push_back("Del");
      v.push_back(skey);
      c_kvsclient->send(v);

      KVData<ValType> kvd;
      v=c_kvsclient->receive();
      kvd.serr = v[1];
      kvd.ierr = stoi(v[2]);
      return kvd;
  }

  template<typename KeyType, typename ValType>
  bool KVStore<KeyType,ValType>::clear() {
      std::vector<string> v;
      v.push_back("Clear");
      c_kvsclient->send(v);
      v=c_kvsclient->receive();
      if(v[0].compare("true")==0)
      {
        return true;
      }
      return false;
  }

}
#endif
