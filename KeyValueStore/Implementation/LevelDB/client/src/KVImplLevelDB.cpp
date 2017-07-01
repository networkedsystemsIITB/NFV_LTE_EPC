#include <kvstore/KVStoreHeader.h>

#include <vector>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <chrono>
#include "../../../../Helper/src/kvstore_client.cpp"
using namespace std;

#define c_kvsclient ((KVManager *)dataholder)


namespace kvstore {

  struct async_data{
    void (*fn)(KVData<string>,void *);
    void *vfn;
    int type;
  };

  class KVManager{
  public:
    KVStoreClient *kc=NULL;
    string conn;
    string tablename;
    std::mutex mtx;
    std::queue<struct async_data> q;
    bool keeprunning = true;
    thread td;

    ~KVManager(){
      keeprunning = false;
      if(td.joinable()){
        td.join();
      }
      if(kc!=NULL){
        delete(kc);
      }
    }

    void eventLoop(){
      std::chrono::milliseconds waittime(5);
      struct async_data ad;
      std::vector<string> v;
      while(keeprunning){
        while(true){
          mtx.lock();
          if(!q.empty())
          {
            ad=q.front();
            q.pop();
            mtx.unlock();
            break;
          }
          mtx.unlock();
          std::this_thread::sleep_for(waittime);
          if(!keeprunning)return;
        }
        KVData<string> ret = KVData<string>();

        v=kc->receive();
        ret.serr = v[2];
        ret.ierr = stoi(v[3]);
        //Get
        if(ret.ierr==0 && ad.type == 1){
          ret.value = v[1];
        }
        ad.fn(ret,ad.vfn);
      }
    }

    void startEventLoop(){
      td = thread([&]{eventLoop();});
    }
  };


  KVImplHelper::KVImplHelper(){
  }

  KVImplHelper::~KVImplHelper(){
    if(dataholder != NULL){
      delete((KVManager *)dataholder);
    }
  }

  KVImplHelper::KVImplHelper(const KVImplHelper& kh){
    if(dataholder != NULL){
      string conn = ((KVManager *)(kh.dataholder))->conn;
      string tablename = ((KVManager *)(kh.dataholder))->tablename;
      bind(conn,tablename);
    }
  }

  bool KVImplHelper::bind(string connection, string tablename){
    dataholder = (void *) new KVManager();
    ((KVManager *)dataholder)->conn = connection;
    ((KVManager *)dataholder)->tablename = tablename;

    int colon = connection.find(":");
    string ip = connection.substr(0,colon);
    string port = connection.substr(colon+1);
    c_kvsclient->kc = new KVStoreClient(ip,stoi(port));

    std::vector<string> v;
    v.push_back("CreateTable");
    v.push_back(tablename);
    c_kvsclient->kc->send(v);
    v=c_kvsclient->kc->receive();
    if(v[0].compare("true")==0)
    {
      c_kvsclient->startEventLoop();
      return true;
    }
    return false;
  }

  KVData<string> KVImplHelper::get(string const& key){
    KVData<string> ret = KVData<string>();
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(c_kvsclient->tablename);
    v.push_back("Get");
    v.push_back(key);
    c_kvsclient->kc->send(v);

    v=c_kvsclient->kc->receive();
    ret.serr = v[2];
    ret.ierr = stoi(v[3]);
    if(ret.ierr==0){
      ret.value = v[1];
    }
    return ret;
  }

  KVData<string> KVImplHelper::put(string const& key,string const& val){
    KVData<string> ret = KVData<string>();
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(c_kvsclient->tablename);
    v.push_back("Put");
    v.push_back(key);
    v.push_back(val);
    c_kvsclient->kc->send(v);

    v=c_kvsclient->kc->receive();
    ret.serr = v[2];
    ret.ierr = stoi(v[3]);
    return ret;
  }

  KVData<string> KVImplHelper::del(string const& key){
    KVData<string> ret = KVData<string>();
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(c_kvsclient->tablename);
    v.push_back("Del");
    v.push_back(key);
    c_kvsclient->kc->send(v);

    v=c_kvsclient->kc->receive();
    ret.serr = v[2];
    ret.ierr = stoi(v[3]);
    return ret;
  }

  void KVImplHelper::async_get(string key, void (*fn)(KVData<string>,void *), void *vfn){
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(c_kvsclient->tablename);
    v.push_back("Get");
    v.push_back(key);
    c_kvsclient->kc->send(v);
    struct async_data ad{fn,vfn,1};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_put(string key,string val, void (*fn)(KVData<string>, void *), void *vfn){
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(c_kvsclient->tablename);
    v.push_back("Put");
    v.push_back(key);
    v.push_back(val);
    c_kvsclient->kc->send(v);
    struct async_data ad{fn,vfn,2};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_del(string key, void (*fn)(KVData<string>,void *), void *vfn){
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(c_kvsclient->tablename);
    v.push_back("Del");
    v.push_back(key);
    c_kvsclient->kc->send(v);
    struct async_data ad{fn,vfn,3};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  bool KVImplHelper::clear(){
    std::vector<string> v;
    v.push_back("Clear");
    c_kvsclient->kc->send(v);
    v=c_kvsclient->kc->receive();
    if(v[0].compare("true")==0)
    {
      return true;
    }
    return false;
  }

  std::vector<KVData<string>> parseResults(vector<string> &res){
    int count = res.size()/4;
    std::vector<KVData<string>> fret(count);
    for(int idx=0;idx<count;idx++){
      KVData<string> ret = KVData<string>();
      ret.serr = res[idx*4+2];
      ret.ierr = stoi(res[idx*4+3]);
      if(ret.ierr==0){
        ret.value = res[idx*4+1];
      }
      fret[idx]=ret;
    }
    return fret;
  }

  int KVImplHelper::mget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret){
    /* Do multiget and send the response via 'ret' vector */
    int sz = key.size();
    std::vector<string> v;
    v.push_back("Multiple");
    for(int i=0;i<sz;i++){
      v.push_back("CreateTable");
      v.push_back(tablename[i]);
      v.push_back("Get");
      v.push_back(key[i]);
    }
    c_kvsclient->kc->send(v);
    std::vector<string> rcv=c_kvsclient->kc->receive();
    std::vector<KVData<string>> res = parseResults(rcv);
    for(int i=0;i<sz;i++){
      ret.push_back(res[i]);
    }
    return 0;
  }

  int KVImplHelper::mput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& ret){
    /* Do multiput and send the response via 'ret' vector */
    int sz = key.size();
    std::vector<string> v;
    v.push_back("Multiple");
    for(int i=0;i<sz;i++){
      v.push_back("CreateTable");
      v.push_back(tablename[i]);
      v.push_back("Put");
      v.push_back(key[i]);
      v.push_back(val[i]);
    }
    c_kvsclient->kc->send(v);
    std::vector<string> rcv=c_kvsclient->kc->receive();
    std::vector<KVData<string>> res = parseResults(rcv);
    for(KVData<string> kd: res){
      ret.push_back(kd);
    }
    return 0;
  }

  int KVImplHelper::mdel(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret){
    /* Do multidel and send the response via 'ret' vector */
    int sz = key.size();
    std::vector<string> v;
    v.push_back("Multiple");
    for(int i=0;i<sz;i++){
      v.push_back("CreateTable");
      v.push_back(tablename[i]);
      v.push_back("Del");
      v.push_back(key[i]);
    }
    c_kvsclient->kc->send(v);
    std::vector<string> rcv=c_kvsclient->kc->receive();
    std::vector<KVData<string>> res = parseResults(rcv);
    for(KVData<string> kd: res){
      ret.push_back(kd);
    }
    return 0;
  }


  void KVImplHelper::async_get(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(tablename);
    v.push_back("Get");
    v.push_back(key);
    c_kvsclient->kc->send(v);
    struct async_data ad{fn,vfn,1};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_put(string key,string val, string tablename, void (*fn)(KVData<string>, void *), void *vfn){
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(tablename);
    v.push_back("Put");
    v.push_back(key);
    v.push_back(val);
    c_kvsclient->kc->send(v);
    struct async_data ad{fn,vfn,2};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_del(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    std::vector<string> v;
    v.push_back("Multiple");
    v.push_back("CreateTable");
    v.push_back(tablename);
    v.push_back("Del");
    v.push_back(key);
    c_kvsclient->kc->send(v);
    struct async_data ad{fn,vfn,3};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  int KVImplHelper::smget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret){
    int sz = key.size();
    KVData<string> kd;
    kd.ierr = -1;
    kd.serr = "SGET not yet implemeted for LevelDB.";
    for(int i=0;i<sz;i++){
      ret.push_back(kd);
    }
    return -1;
  }

  int KVImplHelper::smput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& ret){
    int sz = key.size();
    KVData<string> kd;
    kd.ierr = -1;
    kd.serr = "SPUT not yet implemeted for LevelDB.";
    for(int i=0;i<sz;i++){
      ret.push_back(kd);
    }
    return -1;
  }
}
