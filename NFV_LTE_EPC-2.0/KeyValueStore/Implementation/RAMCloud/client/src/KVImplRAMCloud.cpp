#include <kvstore/KVStoreHeader.h>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>
#include "ramcloud/RamCloud.h" //path to RAMCloud

using namespace std;
using namespace RAMCloud;

namespace kvstore {

  #define c_kvsclient ((KVStoreClient*)dataholder)

  class TableManager{
  public:
    unordered_map<string,uint64_t> um;
    mutex mtx;
  };
  TableManager tm;

  // uint64_t getTableId(string tablename, RamCloud *cluster, string opr);
  uint64_t getTableId(string tablename, RamCloud *cluster, string opr){
    uint64_t tableId = -1;
    tm.mtx.lock();
    if(tm.um.find(tablename) == tm.um.end()){
      try{
        if(opr=="put"){
          tableId = cluster->createTable(tablename.c_str());
          tm.um[tablename] = tableId;
        } else {
          tableId = cluster->getTableId(tablename.c_str());
        }
      } catch(...){//TableDoesntExistException
        tm.mtx.unlock();
        return tableId;
      }
    } else {
      tableId = tm.um[tablename];
    }
    tm.mtx.unlock();
    return tableId;
  }


  #define KVGET 0
  #define KVPUT 1
  #define KVDEL 2
  struct async_data{
    void (*fn)(KVData<string>,void *);
    void *vfn;
    int type;
    string key;
    string value;
    string tablename;
    KVImplHelper *kh;
  };


  class KVStoreClient{
  public:
    string conn;
    string tablename;
    uint64_t table;
    RamCloud *cluster;

    std::mutex mtx;
    std::queue<struct async_data> q;
    bool keeprunning = true;
    thread td;
    KVStoreClient(){
      // count=0;
    }
    ~KVStoreClient(){
      keeprunning = false;
      if(td.joinable()){
        td.join();
      }
    }

    void eventLoop(){
      std::chrono::milliseconds waittime(5);
      struct async_data ad;
      while(keeprunning){
        while(true){mtx.lock();if(!q.empty()){ad = q.front(); q.pop(); mtx.unlock(); break;}; mtx.unlock();std::this_thread::sleep_for(waittime);if(!keeprunning)return;}

        KVData<string> ret;
        if(ad.type == KVPUT){
          vector<KVData<string>> vret;
          vector<string> k(1,ad.key);
          vector<string> v(1,ad.value);
          vector<string> t(1,ad.tablename);
          ad.kh->mput(k,v,t,vret);
          ret = vret[0];
        } else if(ad.type == KVDEL){
          vector<KVData<string>> vret;
          vector<string> k(1,ad.key);
          vector<string> t(1,ad.tablename);
          ad.kh->mdel(k,t,vret);
          ret = vret[0];
        } else if(ad.type == KVGET){
          vector<KVData<string>> vret;
          vector<string> k(1,ad.key);
          vector<string> t(1,ad.tablename);
          ad.kh->mget(k,t,vret);
          ret = vret[0];
        }
        ad.fn(ret,ad.vfn);
      }
    }

    void startEventLoop(){
      td = thread([&]{eventLoop();});
    }
  };

  KVImplHelper::KVImplHelper(){
    dataholder = (void*) new KVStoreClient();
  }


  KVImplHelper::KVImplHelper(const KVImplHelper& kh){
    dataholder = (void*) new KVStoreClient();
    bool succ = bind(((KVStoreClient*)kh.dataholder)->conn,((KVStoreClient*)kh.dataholder)->tablename);
    if(!succ){
      std::cerr << "Error copying KVImplHelper object" << std::endl;
    }
  }

  KVImplHelper::~KVImplHelper(){
    delete(c_kvsclient->cluster);
    delete(c_kvsclient);
  }

  bool KVImplHelper::bind(string conn, string tablename){
    c_kvsclient->conn = conn;
    c_kvsclient->tablename = tablename;
    c_kvsclient->cluster = new RamCloud(conn.c_str(),"test_cluster");
    c_kvsclient->table = c_kvsclient->cluster->createTable(tablename.c_str());
    c_kvsclient->startEventLoop();
    return true;
  }

  KVData<string> KVImplHelper::get(string const& skey){
    KVData<string> kvd;
    try{
      Buffer buffer;
      c_kvsclient->cluster->read(c_kvsclient->table, skey.c_str(), skey.size(), &buffer);
      char *buf = new char[buffer.size()+1];
      buffer.copy(0,buffer.size(),buf);

      kvd.serr = "";
      kvd.ierr = 0;
      kvd.value = string(buf);
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

KVData<string> KVImplHelper::put(string const& skey,string const& sval){
  KVData<string> kvd;
  try{
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

KVData<string> KVImplHelper::del(string const& skey){
  KVData<string> kvd;
  try{
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

bool KVImplHelper::clear(){
  try{
    c_kvsclient->cluster->dropTable(c_kvsclient->tablename.c_str());
  }
  catch (...) {
    return false;
  }
  return true;
};

int KVImplHelper::mget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
  int sz = key.size();
  uint64_t tid[sz];
  MultiReadObject *mro[sz];
  Tub<ObjectBuffer> retval[sz];
  for(int i=0;i<sz;i++) {
    tid[i] = getTableId(tablename[i],c_kvsclient->cluster,"get");
    // if(tid == -1){
    //   //? Error table not found
    //   continue;
    // }
    mro[i] = new MultiReadObject(tid[i], key[i].c_str(), key[i].size(), &retval[i]);
  }
  c_kvsclient->cluster->multiRead(mro,sz);

  for(int i=0;i<sz;i++){
    KVData<string> kvd;
    if(retval[i].get()==NULL){
      kvd.ierr = -1;
      kvd.serr = "Enrty doesn't exists";
    } else {
      kvd.ierr = 0;
      kvd.serr = "";
      kvd.value = string((char *)retval[i].get()->getValue());
    }
    vret.push_back(kvd);
  }
  return 0;
}

int KVImplHelper::mput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& vret){
  int sz = key.size();
  uint64_t tid[sz];
  MultiWriteObject *mwo[sz];
  // cout<<"DP11 called"<<endl;
  for(int i=0;i<sz;i++) {
    tid[i] = getTableId(tablename[i],c_kvsclient->cluster,"put");
    mwo[i] = new MultiWriteObject(tid[i], key[i].c_str(), key[i].size(), val[i].c_str(), val[i].size()); //? delete
    // cout<<"mput: key:"<<key[i]<<" val:"<<val[i]<<" tb:"<<tablename[i]<<" tid:"<<tid[i]<<endl;
  }
  c_kvsclient->cluster->multiWrite(mwo, sz);
  //? All successful?
  for(int i=0;i<sz;i++){
    KVData<string> kvd;
    kvd.ierr = 0;
    kvd.serr = "";
    vret.push_back(kvd);
  }
  return 0;
}

int KVImplHelper::mdel(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
  int sz = key.size();
  MultiRemoveObject *mdo[sz];
  uint64_t tid[sz];

  for(int i=0;i<sz;i++) {
    tid[i] = getTableId(tablename[i],c_kvsclient->cluster,"del");
    // if(tid == -1){
    //   //? Error table not found
    //   continue;
    // }
    mdo[i] = new MultiRemoveObject(tid[i], key[i].c_str(), key[i].size());
  }
  c_kvsclient->cluster->multiRemove(mdo,sz);
  //? All successful?
  for(int i=0;i<sz;i++){
    KVData<string> kvd;
    kvd.ierr = 0;
    kvd.serr = "";
    vret.push_back(kvd);
  }
  return 0;
}

void KVImplHelper::async_get(string key, void (*fn)(KVData<string>,void *), void *vfn){
  struct async_data ad{fn,vfn,KVGET,key,"",c_kvsclient->tablename,this};
  c_kvsclient->mtx.lock();
  c_kvsclient->q.push(ad);
  c_kvsclient->mtx.unlock();
}

void KVImplHelper::async_put(string key,string val, void (*fn)(KVData<string>, void *), void *vfn){
  struct async_data ad{fn,vfn,KVPUT,key,val,c_kvsclient->tablename,this};
  c_kvsclient->mtx.lock();
  c_kvsclient->q.push(ad);
  c_kvsclient->mtx.unlock();
}
void KVImplHelper::async_del(string key, void (*fn)(KVData<string>, void *), void *vfn){
  struct async_data ad{fn,vfn,KVDEL,key,"",c_kvsclient->tablename,this};
  c_kvsclient->mtx.lock();
  c_kvsclient->q.push(ad);
  c_kvsclient->mtx.unlock();
}


void KVImplHelper::async_get(string key, string tablename, void (*fn)(KVData<string>, void *), void *vfn){
  struct async_data ad{fn,vfn,KVGET,key,"",tablename,this};
  c_kvsclient->mtx.lock();
  c_kvsclient->q.push(ad);
  c_kvsclient->mtx.unlock();
}

void KVImplHelper::async_put(string key,string val, string tablename, void (*fn)(KVData<string>, void *), void *vfn){
  struct async_data ad{fn,vfn,KVPUT,key,val,tablename,this};
  c_kvsclient->mtx.lock();
  c_kvsclient->q.push(ad);
  c_kvsclient->mtx.unlock();
}
void KVImplHelper::async_del(string key, string tablename, void (*fn)(KVData<string>, void *), void *vfn){
  struct async_data ad{fn,vfn,KVDEL,key,"",tablename,this};
  c_kvsclient->mtx.lock();
  c_kvsclient->q.push(ad);
  c_kvsclient->mtx.unlock();
}
int KVImplHelper::smget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret){
  int sz = key.size();
  KVData<string> kd;
  kd.ierr = -1;
  kd.serr = "SGET not yet implemeted for RAMCloud.";
  for(int i=0;i<sz;i++){
    ret.push_back(kd);
  }
  return -1;
}

int KVImplHelper::smput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& ret){
  int sz = key.size();
  KVData<string> kd;
  kd.ierr = -1;
  kd.serr = "SPUT not yet implemeted for RAMCloud.";
  for(int i=0;i<sz;i++){
    ret.push_back(kd);
  }
  return -1;
}
}
