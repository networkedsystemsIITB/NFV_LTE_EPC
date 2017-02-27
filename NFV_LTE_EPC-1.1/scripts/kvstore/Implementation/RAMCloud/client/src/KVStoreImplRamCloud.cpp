// Why both implementation and declaration are in same file
// Refer : https://isocpp.org/wiki/faq/templates
//        http://stackoverflow.com/questions/3040480/c-template-function-compiles-in-header-but-not-implementation
//It only includes templates implementation


/*
TODO
Handle Exceptions With --> http://stackoverflow.com/questions/21022436/how-to-get-the-file-name-and-a-line-number-of-a-function-call
void _B(const char* file, int line) { ... } 
#define B() _B(__FILE__, __LINE__)
*/
#include "../../../../Interface/KVStoreHeader.h"

#ifndef __KVSTORE_DEF__
#define __KVSTORE_DEF__

// #include "kvstore_client.cpp"
#include "ramcloud/RamCloud.h" //path to RAMCloud

#define c_kvsclient ((KVSClientData *)kvsclient)

using namespace RAMCloud;
namespace kvstore {

  class KVSClientData {
  public:
    string tablename;
    uint64_t table;
    RamCloud *cluster;
  };

  template<typename ValType>
  KVData<ValType> KVResultSet::get(int idx){
    KVData<ValType> ret;
    // if(idx>=0 && idx<count){
    //   ret.serr = res[idx*4+2];
    //   ret.ierr = stoi(res[idx*4+3]);
    //   if(ret.ierr==0){
    //     ret.value = toBoostObject<ValType>(res[idx*4+1]);
    //   }
    // } else {
    //   ret.ierr=-1;
    //   if(count==0)
    //   ret.serr="Empty KVResultSet due to empty request queue, please ensure that your request queue is not empty.";
    //   else
    //   ret.serr="KVResultSet index out of bound. Valid range is 0 to "+to_string(count-1)+" found "+to_string(idx);
    // }
    return ret;
  }


  template<typename KeyType, typename ValType>
  void KVRequest::get(KeyType const& key,string tablename){
    // v.push_back("CreateTable");
    // v.push_back(tablename);
    // string skey=toBoostString(key);
    // v.push_back("Get");
    // v.push_back(skey);
  }

  template<typename KeyType, typename ValType>
  void KVRequest::put(KeyType const& key,ValType const& val,string tablename){
    // v.push_back("CreateTable");
    // v.push_back(tablename);
    // string skey=toBoostString(key);
    // string sval=toBoostString(val);
    // v.push_back("Put");
    // v.push_back(skey);
    // v.push_back(sval);
  }

  template<typename KeyType, typename ValType>
  void KVRequest::del(KeyType const& key,string tablename){
    // v.push_back("CreateTable");
    // v.push_back(tablename);
    // string skey=toBoostString(key);
    // v.push_back("Del");
    // v.push_back(skey);
  }

  //------------------------------------------------------------------

  template<typename KeyType, typename ValType>
  KVStore<KeyType,ValType>::KVStore(){
  }

  template<typename KeyType, typename ValType>
  bool KVStore<KeyType,ValType>::bind(string connection,string tablename){
    kvsclient = (void *) new KVSClientData();
    c_kvsclient->tablename = tablename;
    c_kvsclient->cluster = new RamCloud(connection.c_str(),"test_cluster");
    c_kvsclient->table = c_kvsclient->cluster->createTable(tablename.c_str());
    return true;
  }

  template<typename KeyType, typename ValType>
  KVStore<KeyType,ValType>::~KVStore(){
    delete(c_kvsclient->cluster);
    delete(c_kvsclient);
  }

  template<typename KeyType, typename ValType>
  KVData<ValType>  KVStore<KeyType,ValType>::get(KeyType const& key){
    //val,serr,ierr
    KVData<ValType> kvd;
    try{
      string skey=toBoostString(key);
      Buffer buffer;
      c_kvsclient->cluster->read(c_kvsclient->table, skey.c_str(), skey.size(), &buffer);
      char *buf = new char[buffer.size()+1];
      buffer.copy(0,buffer.size(),buf);

      kvd.serr = "";
      kvd.ierr = 0;
      kvd.value = toBoostObject<ValType>(string(buf)); //?
      delete buf;
    }/*
    catch(RAMCloud::ObjectDoesntExistException& e){
      kvd.serr = e.str();
      kvd.ierr=-1;
    }*/
    catch (RAMCloud::Exception& e) {
      kvd.serr = e.str();
      kvd.ierr=-1;
    }
    catch (std::exception const& e) {
      kvd.serr = e.what();
      kvd.ierr=-1;
    }
    catch (...) {
      kvd.serr = "Unknown Exception!!!";
      kvd.ierr=-1;
    } 
    return kvd;
  }

  template<typename KeyType, typename ValType>
  KVData<ValType> KVStore<KeyType,ValType>::put(KeyType const& key,ValType const& val){
    //cout<<"PUT"<<endl;
    KVData<ValType> kvd;
    try{
      string skey=toBoostString(key);
      string sval=toBoostString(val);
      c_kvsclient->cluster->write(c_kvsclient->table, skey.c_str(), skey.size(), sval.c_str(), sval.size()+1);
      kvd.serr = "";
      kvd.ierr = 0;

    }
    catch (RAMCloud::Exception& e) {
      kvd.serr = e.str();
      kvd.ierr=-1;
    }
    catch (std::exception const& e) {
      kvd.serr = e.what();
      kvd.ierr=-1;
    }
    catch (...) {
      kvd.serr = "Unknown Exception!!!";
      kvd.ierr=-1;
    } 
    return kvd;
  }

  template<typename KeyType, typename ValType>
  KVData<ValType> KVStore<KeyType,ValType>::del(KeyType const& key){
    KVData<ValType> kvd;
    try{
      string skey=toBoostString(key);
      c_kvsclient->cluster->remove(c_kvsclient->table, skey.c_str(), skey.size());
      kvd.serr = "";
      kvd.ierr = 0;
    }
    catch (RAMCloud::Exception& e) {
      kvd.serr = e.str();
      kvd.ierr=-1;
    }
    catch (std::exception const& e) {
      kvd.serr = e.what();
      kvd.ierr=-1;
    }
    catch (...) {
      kvd.serr = "Unknown Exception!!!";
      kvd.ierr=-1;
    } 
    return kvd;
  }

  template<typename KeyType, typename ValType>
  bool KVStore<KeyType,ValType>::clear() {
    try{
      c_kvsclient->cluster->dropTable(c_kvsclient->tablename.c_str());
    }
    catch (...) {
      return false;
    } 
    return true;
  }

}
#endif
