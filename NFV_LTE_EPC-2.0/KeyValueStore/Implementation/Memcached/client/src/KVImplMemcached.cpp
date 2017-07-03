#include <kvstore/KVStoreHeader.h>
#include <libmemcached/memcached.h>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>
using namespace std;
namespace kvstore {

  void replaceStrChar(string &str, char old, char new_) {
    for (int i = 0; i < str.length(); ++i) {
      if (str[i] == old)
      str[i] = new_;
    }
  }

  #define c_kvsclient ((KVStoreClient*)dataholder)

  struct async_data{
    void (*fn)(KVData<string>,void *);
    void *vfn;
    int type;
    string key;
    string value;
    string tablename;
    KVImplHelper *kh;
  };

  #define KVGET 0
  #define KVPUT 1
  #define KVDEL 2
  #define KVMGET 3

  class KVStoreClient{
  public:
    string conn;
    memcached_st *memc;
    string tablename;
    unordered_map<string,uint64_t> version_store;
    std::mutex mtx;
    std::queue<struct async_data> q;
    bool keeprunning = true;
    thread td;

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
        while(true){
          mtx.lock();
          if(!q.empty()){
            ad=q.front();
            q.pop();
            mtx.unlock();
            break;
          };
          mtx.unlock();
          std::this_thread::sleep_for(waittime);
          if(!keeprunning)return;
        }

        KVData<string> ret = KVData<string>();
        ret.ierr = -1;
        if(ad.type == KVGET){
          int sz = 1;
          uint32_t flags;
          char return_key[MEMCACHED_MAX_KEY];
          size_t return_key_length;
          char *return_value;
          size_t return_value_length;
          memcached_return_t error;
          int itr=0;
          while((return_value = memcached_fetch(memc, return_key, &return_key_length, &return_value_length, &flags, &error)))
          {
            string rkey = string(return_key,return_key_length);
            while(itr<sz){
              if(ad.key == rkey){
                ret.ierr = 0;
                ret.serr = "";
                ret.value = string(return_value,return_value_length);
                itr++;
                break;
              } else {
                ret.ierr = -1;
                ret.serr = "Unknown error [error type cannot be identified].";
                ret.value = "";
                itr++;
              }
            }
            free(return_value);
            if(itr>1){
              cerr<<"Implementation problem -> Multiple fatches in one go."<<endl;
            }
          }
        } else if(ad.type == KVPUT){
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
        } else if(ad.type == KVMGET){
          vector<KVData<string>> vret;
          vector<string> k(1,ad.key);
          vector<string> t(1,ad.tablename);
          ad.kh->mget(k,t,vret);
          ret = vret[0];
        }
        ad.fn(ret,ad.vfn);
      }
    } //end of eventLoop() function
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
    delete(c_kvsclient);
  }

  bool KVImplHelper::bind(string conn, string tablename){
    c_kvsclient->tablename = tablename;
    c_kvsclient->conn = conn;
    c_kvsclient->memc = memcached(conn.c_str(), conn.size());
    if(c_kvsclient->memc == NULL){
      return false;
    }
    memcached_behavior_set(c_kvsclient->memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);
    c_kvsclient->startEventLoop();
    return true;
  }

  KVData<string> KVImplHelper::get(string const& skey){
    KVData<string> ret = KVData<string>();
    string key = c_kvsclient->tablename + skey;
    replaceStrChar(key, ' ', '_');
    replaceStrChar(key, '\t', '_');
    replaceStrChar(key, '\n', '_');
    replaceStrChar(key, '\r', '_');
    char *op;
    size_t value_length;
    uint32_t flags;
    memcached_return_t error;
    op = memcached_get(c_kvsclient->memc, key.c_str(), key.size(), &value_length, &flags, &error);
    if(error != MEMCACHED_SUCCESS){
      /* http://docs.libmemcached.org/libmemcached/memcached_return_t.html#memcached_return_t */
      ret.ierr = -1;
      ret.serr = string(memcached_strerror(NULL,error));
      return ret;
    }
    ret.ierr = 0;
    ret.value = string(op,value_length);
    free(op);
    return ret;
  }

  KVData<string> KVImplHelper::put(string const& skey,string const& val){
    KVData<string> ret = KVData<string>();
    string key = c_kvsclient->tablename + skey;
    replaceStrChar(key, ' ', '_');
    replaceStrChar(key, '\t', '_');
    replaceStrChar(key, '\n', '_');
    replaceStrChar(key, '\r', '_');
    memcached_return_t error = memcached_set(c_kvsclient->memc, key.c_str(), key.size(), val.c_str(), val.size(), (time_t)0, (uint32_t)0);
    if (error != MEMCACHED_SUCCESS)
    {
      ret.ierr = -1;
      ret.serr = string(memcached_strerror(NULL,error));
      return ret;
    }
    ret.ierr = 0;
    return ret;
  }

  KVData<string> KVImplHelper::del(string const& skey){
    KVData<string> ret = KVData<string>();
    string key = c_kvsclient->tablename + skey;
    replaceStrChar(key, ' ', '_');
    replaceStrChar(key, '\t', '_');
    replaceStrChar(key, '\n', '_');
    replaceStrChar(key, '\r', '_');
    memcached_return_t error = memcached_delete(c_kvsclient->memc, key.c_str(), key.size(), (time_t)0);
    if (error != MEMCACHED_SUCCESS)
    {
      ret.ierr = -1;
      ret.serr = string(memcached_strerror(NULL,error));
      return ret;
    }
    ret.ierr = 0;
    return ret;
  }

  bool KVImplHelper::clear(){
    return false;
  };

  int KVImplHelper::mget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
    KVData<string> ret = KVData<string>();
    int sz = key.size();
    if(sz==0){
      return -1;
    }
    size_t key_length[sz];
    const char *keys[sz];
    string tbkey[sz];
    for(int i=0;i<sz;i++){
      tbkey[i] =  tablename[i] + key[i];
      replaceStrChar(tbkey[i], ' ', '_');
      replaceStrChar(tbkey[i], '\t', '_');
      replaceStrChar(tbkey[i], '\n', '_');
      replaceStrChar(tbkey[i], '\r', '_');
      key_length[i] = tbkey[i].size();
      keys[i] = tbkey[i].c_str();
    }
    uint32_t flags;
    char return_key[MEMCACHED_MAX_KEY];
    size_t return_key_length;
    char *return_value;
    size_t return_value_length;
    memcached_return_t error;

    error = memcached_mget(c_kvsclient->memc, keys, key_length, sz);
    if(error != MEMCACHED_SUCCESS){
      cerr<<""<<__FILE__<<" :"<<__LINE__<<" Error in memcached_mget :"<<string(memcached_strerror(NULL,error))<<endl;
      return -1;
    }
    int itr=0;
    while ((return_value = memcached_fetch(c_kvsclient->memc, return_key, &return_key_length, &return_value_length, &flags, &error)))
    {
      string rkey = string(return_key,return_key_length);
      while(itr<sz){
        if(string(keys[itr]) == rkey){
          ret.ierr = 0;
          ret.serr = "";
          ret.value = string(return_value,return_value_length);
          vret.push_back(ret);
          itr++;
          break;
        } else {
          ret.ierr = -1;
          ret.serr = "Unknown error [error type cannot be identified].";
          ret.value = "";
          vret.push_back(ret);
        }
        itr++;
      }
      free(return_value);
    }
    for(;itr<sz;itr++){
      ret.ierr = -1;
      ret.serr = "Value not found or Unknown error [error type cannot be identified].";
      ret.value = "";
      vret.push_back(ret);
    }
    return 0;
  }

  int KVImplHelper::mput(vector<string>& skey, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& vret){
    KVData<string> ret = KVData<string>();
    memcached_return_t error;
    int sz = skey.size();
    for(int i=0; i<sz; i++){
      string key = tablename[i] + skey[i];
      replaceStrChar(key, ' ', '_');
      replaceStrChar(key, '\t', '_');
      replaceStrChar(key, '\n', '_');
      replaceStrChar(key, '\r', '_');
      error = memcached_set(c_kvsclient->memc, key.c_str(), key.size(), val[i].c_str(), val[i].size(), (time_t)0, (uint32_t)0);
      if (error != MEMCACHED_SUCCESS)
      {
        ret.ierr = -1;
        ret.serr = string(memcached_strerror(NULL,error));
      } else {
        ret.ierr = 0;
        ret.serr = "";
      }
      vret.push_back(ret);
    }
    return 0;
  }

  int KVImplHelper::mdel(vector<string>& skey, vector<string>& tablename, vector<KVData<string>>& vret){
    KVData<string> ret = KVData<string>();
    memcached_return_t error;
    int sz = skey.size();
    for(int i=0; i<sz; i++){
      string key = tablename[i] + skey[i];
      replaceStrChar(key, ' ', '_');
      replaceStrChar(key, '\t', '_');
      replaceStrChar(key, '\n', '_');
      replaceStrChar(key, '\r', '_');
      error = memcached_delete(c_kvsclient->memc, key.c_str(), key.size(), (time_t)0);
      if (error != MEMCACHED_SUCCESS)
      {
        ret.ierr = -1;
        ret.serr = string(memcached_strerror(NULL,error));
      } else {
        ret.ierr = 0;
        ret.serr = "";
      }
      vret.push_back(ret);
    }
    return 0;
  }

  void KVImplHelper::async_get(string key, void (*fn)(KVData<string>,void *), void *vfn){
    struct async_data ad = {fn, vfn, KVMGET, key, "",  c_kvsclient->tablename, this};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_put(string key,string val, void (*fn)(KVData<string>,void *), void *vfn){
    struct async_data ad = {fn, vfn, KVPUT, key, val, c_kvsclient->tablename, this};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_del(string key, void (*fn)(KVData<string>,void *), void *vfn){
    struct async_data ad = {fn, vfn, KVDEL, key, "",  c_kvsclient->tablename, this};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }


  void KVImplHelper::async_get(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    struct async_data ad = {fn, vfn, KVMGET, key, "", tablename, this};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  void KVImplHelper::async_put(string key,string val, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    struct async_data ad = {fn, vfn, KVPUT, key, val, tablename, this};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }
  void KVImplHelper::async_del(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    struct async_data ad = {fn, vfn, KVDEL, key, "", tablename, this};
    c_kvsclient->mtx.lock();
    c_kvsclient->q.push(ad);
    c_kvsclient->mtx.unlock();
  }

  int KVImplHelper::smget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
    KVData<string> ret = KVData<string>();
    int sz = key.size();
    if(sz==0){
      return -1;
    }

    memcached_return_t rc;
    char *keys[sz];
    size_t key_length[sz];
    string tbkey[sz];
    for(int i=0;i<sz;i++){
      tbkey[i] =  tablename[i] + key[i];
      replaceStrChar(tbkey[i], ' ', '_');
      replaceStrChar(tbkey[i], '\t', '_');
      replaceStrChar(tbkey[i], '\n', '_');
      replaceStrChar(tbkey[i], '\r', '_');
      key_length[i] = tbkey[i].size();
      keys[i] = (char*)tbkey[i].c_str();
    }

    memcached_return_t error;
    error = memcached_mget(c_kvsclient->memc, keys, key_length, sz);
    if(error != MEMCACHED_SUCCESS){
      cerr<<""<<__FILE__<<" :"<<__LINE__<<" Error in memcached_mget :"<<string(memcached_strerror(NULL,error))<<endl;
      return -1;
    }

    const char* keyval;
    size_t keylen;
    const char* resval;
    size_t reslen;
    uint64_t version;
    string retkey;
    string retval;
    memcached_result_st *rs =  memcached_result_create(c_kvsclient->memc, NULL);
    int i;
    for(i=0;i<sz;i++){
      memcached_fetch_result(c_kvsclient->memc, rs, &error);
      if (error == MEMCACHED_SUCCESS){
        keyval = memcached_result_key_value(rs);
        keylen = memcached_result_key_length(rs);
        resval = memcached_result_value(rs);
        reslen = memcached_result_length(rs);
        version = memcached_result_cas(rs);
        retkey = string(keyval,keylen);
        retval = string(resval,reslen);
        c_kvsclient->version_store[retkey] = version;
        while(i<sz){
          if(string(tbkey[i]) == retkey){
            ret.ierr = 0;
            ret.serr = "";
            ret.value = retval;
            vret.push_back(ret);
            break;
          } else {
            ret.ierr = -1;
            ret.serr = "Value not found or Unknown error [error type cannot be identified].";
            ret.value = "";
            vret.push_back(ret);
          }
          i++;
        }//end while
      } else {
        break; //MEMCACHED_END
      }
    }
    ret.ierr = -1;
    ret.serr = "Value not found or Unknown error [error type cannot be identified].";
    ret.value = "";
    for(;i<sz;i++){
      vret.push_back(ret);
    }
    return 0;
  }

  int KVImplHelper::smput(vector<string>& skey, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& vret){
    KVData<string> ret = KVData<string>();
    memcached_return_t error;
    int sz = skey.size();
    string key;
    uint64_t version;
    for(int i=0; i<sz; i++){
      key = tablename[i] + skey[i];
      replaceStrChar(key, ' ', '_');
      replaceStrChar(key, '\t', '_');
      replaceStrChar(key, '\n', '_');
      replaceStrChar(key, '\r', '_');
      if(c_kvsclient->version_store.find(key) != c_kvsclient->version_store.end()){
        version = c_kvsclient->version_store[key];
        c_kvsclient->version_store.erase(key);
      } else {
        version = 0;
        ret.ierr = -1;
        ret.serr = "Version error: version not acquired yet (please always do SGET for the given key before doing SPUT OR use PUT operation)";
        vret.push_back(ret);
        continue;
      }
      error = memcached_cas(c_kvsclient->memc, key.c_str(), key.size(), val[i].c_str(), val[i].size(), (time_t)0, (uint32_t)0, version);
      if (error != MEMCACHED_SUCCESS)
      {
        ret.ierr = -1;
        ret.serr = string(memcached_strerror(NULL,error));
      } else {
        ret.ierr = 0;
        ret.serr = "";
      }
      vret.push_back(ret);
    }
    return 0;
  }
}
