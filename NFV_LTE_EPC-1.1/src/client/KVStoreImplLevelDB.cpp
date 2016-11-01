// Why both implementation and declaration are in same file
// Refer : https://isocpp.org/wiki/faq/templates
//        http://stackoverflow.com/questions/3040480/c-template-function-compiles-in-header-but-not-implementation
//It only includes templates implementation
#include "KVStoreHeader.h"

#ifndef __KVSTORE_DEF__
#define __KVSTORE_DEF__

#include "kvstore_client.cpp"

#define c_kvsclient ((KVStoreClient *)kvsclient)

namespace kvstore {

    	template<typename ValType>
    	KVData<ValType> KVResultSet::get(int idx){
        KVData<ValType> ret;
        if(idx>=0 && idx<count){
          ret.serr = res[idx*4+2];
          ret.ierr = stoi(res[idx*4+3]);
          if(ret.ierr==0){
            ret.value = toBoostObject<ValType>(res[idx*4+1]);
          }
        } else {
          ret.ierr=-1;
          if(count==0)
          ret.serr="Empty KVResultSet due to empty request queue, please ensure that your request queue is not empty.";
          else
          ret.serr="KVResultSet index out of bound. Valid range is 0 to "+to_string(count-1)+" found "+to_string(idx);
        }
        return ret;
      }


    template<typename KeyType, typename ValType>
    void KVRequest::get(KeyType const& key,string tablename){
      v.push_back("CreateTable");
      v.push_back(tablename);
      string skey=toBoostString(key);
      v.push_back("Get");
      v.push_back(skey);
      //rep.push_back(new KVData<ValType>);
    }

    template<typename KeyType, typename ValType>
    void KVRequest::put(KeyType const& key,ValType const& val,string tablename){
      v.push_back("CreateTable");
      v.push_back(tablename);
      string skey=toBoostString(key);
      string sval=toBoostString(val);
      v.push_back("Put");
      v.push_back(skey);
      v.push_back(sval);
      //rep.push_back(new KVData<ValType>);
    }

    template<typename KeyType, typename ValType>
    void KVRequest::del(KeyType const& key,string tablename){
      v.push_back("CreateTable");
      v.push_back(tablename);
      string skey=toBoostString(key);
      v.push_back("Del");
      v.push_back(skey);
      //rep.push_back(new KVData<ValType>);
    }

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
