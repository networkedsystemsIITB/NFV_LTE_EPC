#include <kvstore/KVStoreHeader.h>
#include <hiredis-vip/hircluster.h>
#include <mutex>
#include <atomic>
#include <queue>
#include <chrono>
#include <iostream>
#include <thread>
using namespace std;
namespace kvstore {

  #define c_kvsclient ((KVStoreClient*)dataholder)

  struct async_data{
    void (*fn)(KVData<string>,void *);
    void *vfn;
    int type;
  };

  class KVStoreClient{
  public:
    redisClusterContext* rc=NULL;
    string tablename;
    string conn;
    std::mutex mtx;
    std::queue<struct async_data> q;
    bool keeprunning = true;
    thread td;
    KVStoreClient(){
    }
    ~KVStoreClient(){
      keeprunning = false;
      if(td.joinable()){
        td.join();
      }
      if(rc!=NULL)
      redisClusterFree(rc);
    }
    void eventLoop(){
      redisReply *reply;
      int rep;
      std::chrono::milliseconds waittime(5);

      while(keeprunning){
        while(true){
          mtx.lock();
          if(!q.empty())
          {
            mtx.unlock();
            break;
          };
          mtx.unlock();
          std::this_thread::sleep_for(waittime);
          if(!keeprunning)return;
        }
        mtx.lock();
        rep = redisClusterGetReply(rc, (void**)&reply);
        mtx.unlock();

        // #define REDIS_REPLY_STRING 1
        // #define REDIS_REPLY_ARRAY 2
        // #define REDIS_REPLY_INTEGER 3
        // #define REDIS_REPLY_NIL 4
        // #define REDIS_REPLY_STATUS 5
        // #define REDIS_REPLY_ERROR 6
        if(rep == REDIS_OK){
          if(reply == NULL){
            cerr<<"Reply Null :"<<rc->errstr<<endl;
          } else {
            KVData<string> ret = KVData<string>();
            mtx.lock();
            struct async_data ad = q.front(); q.pop();
            mtx.unlock();
            if(reply->type == REDIS_REPLY_STRING){
              ret.ierr = 0;
              ret.value = string(reply->str);
            } else if (reply->type == REDIS_REPLY_STATUS){
              ret.ierr = 0;
            } else if (reply->type == REDIS_REPLY_NIL){
              ret.ierr = -1;
              ret.serr = "Value doesn't exists.";
            } else if (reply->type == REDIS_REPLY_INTEGER){
              if(reply->integer <= 0){
                ret.ierr = -1;
                ret.serr = "Value doesn't exists.";
              } else {
                ret.ierr = 0;
              }
            } else {
              cerr<<"Reply type:"<<reply->type<<endl;
            }
            ad.fn(ret,ad.vfn);
            freeReplyObject(reply);
          }
        } else { //REDIS_ERR
          cerr<<"Error in return file:"<<__FILE__<<" line:"<<__LINE__<<" err:"<<rc->errstr<<endl;
          redisClusterReset(rc);
        }
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
    delete(c_kvsclient);
  }

  bool KVImplHelper::bind(string conn, string tablename){
    c_kvsclient->tablename = tablename;
    c_kvsclient->conn = conn;

    bool retry = true;
    int attempts = 0;
    int MAX_TRIES = 10;
    while(retry){
      c_kvsclient->rc = redisClusterConnect(conn.c_str(), HIRCLUSTER_FLAG_NULL);
      if(c_kvsclient->rc == NULL || c_kvsclient->rc->err)
      {
        attempts++;
        if(attempts == MAX_TRIES){
          return false;
        }
      } else {
        retry = false;
      }
    }
    c_kvsclient->startEventLoop();
    return true;
  }

  KVData<string> KVImplHelper::get(string const& key){
    KVData<string> ret = KVData<string>();
    redisReply *reply = (redisReply *)redisClusterCommand(c_kvsclient->rc, "get %s", (c_kvsclient->tablename+key).c_str());
    if(reply == NULL)
    {
      ret.ierr = -1;
      ret.serr = "Unknown error.";
    } else if(reply->str == NULL){
      ret.ierr = -1;
      ret.serr = "Value doesn't exists.";
      freeReplyObject(reply);
    } else {
      ret.ierr = 0;
      ret.value = string(reply->str);
      freeReplyObject(reply);
    }
    return ret;
  }

  KVData<string> KVImplHelper::put(string const& key,string const& val){
    KVData<string> ret = KVData<string>();
    redisReply *reply = (redisReply *)redisClusterCommand(c_kvsclient->rc, "set %s %s", (c_kvsclient->tablename+key).c_str(), val.c_str());
    if(reply == NULL)
    {
      ret.ierr = -1;
      ret.serr = "Unknown error.";
    } else {
      ret.ierr = 0;
      freeReplyObject(reply);
    }
    return ret;
  }

  KVData<string> KVImplHelper::del(string const& key){
    KVData<string> ret = KVData<string>();
    redisReply *reply = (redisReply *)redisClusterCommand(c_kvsclient->rc, "del %s", (c_kvsclient->tablename+key).c_str());
    if(reply == NULL)
    {
      ret.ierr = -1;
      ret.serr = "Unknown error.";
    } else {
      if(reply->integer <= 0){
        ret.ierr = -1;
        ret.serr = "Value doesn't exists.";
      } else {
        ret.ierr = 0;
      }
      freeReplyObject(reply);
    }
    return ret;
  }

  bool KVImplHelper::clear(){
    return false;
  };

  int KVImplHelper::mget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
    int sz = key.size();
    int rep;
    redisReply *reply;

    for(int i=0;i<sz;i++){
      rep = redisClusterAppendCommand(c_kvsclient->rc, "get %s",(tablename[i]+key[i]).c_str()) ;
      if(rep == REDIS_ERR){
        cerr<<"Get Append error"<<endl;
      }
    }
    for(int i=0;i<sz;i++){
      rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
      if(rep == REDIS_OK){
        if(reply == NULL){
          cerr<<"Reply Null"<<endl;
        } else {
          KVData<string> ret = KVData<string>();
          if(reply->type == REDIS_REPLY_STRING){
            ret.ierr = 0;
            ret.value = string(reply->str);
          } else if (reply->type == REDIS_REPLY_NIL){
            ret.ierr = -1;
            ret.serr = "Value doesn't exists.";
          } else {
            cerr<<"Reply type:"<<reply->type<<endl;
          }
          freeReplyObject(reply);
          vret.push_back(ret);
        }
      } else {
        cerr<<"Error in return file:"<<__FILE__<<" line:"<<__LINE__<<endl;
        // redisClusterReset(cc);
      }
    }
    return 0;
  }

  int KVImplHelper::mput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& vret){
    int sz = key.size();
    int rep;
    redisReply *reply;

    for(int i=0;i<sz;i++){
      rep = redisClusterAppendCommand(c_kvsclient->rc, "set %s %s",(tablename[i]+key[i]).c_str(), val[i].c_str()) ;
      if(rep == REDIS_ERR){
        cerr<<"Set error in redis. Reason may be resource contention."<<endl;
      }
    }
    for(int i=0;i<sz;i++){
      rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
      if(rep == REDIS_OK){
        if(reply == NULL){
          cerr<<"Reply Null."<<endl;
        } else {
          KVData<string> ret = KVData<string>();
          if (reply->type == REDIS_REPLY_STATUS){
            ret.ierr = 0;
          } else {
            cerr<<"Reply type:"<<reply->type<<endl;
          }
          freeReplyObject(reply);
          vret.push_back(ret);
        }
      }
    }
    return 0;
  }

  int KVImplHelper::mdel(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
    int sz = key.size();
    int rep;
    redisReply *reply;

    for(int i=0;i<sz;i++){
      rep = redisClusterAppendCommand(c_kvsclient->rc, "del %s",(tablename[i]+key[i]).c_str());
      if(rep == REDIS_ERR){
        cerr<<"Del error in redis. Reason may be resource contention."<<endl;
      }
    }
    for(int i=0;i<sz;i++){
      rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
      if(rep == REDIS_OK){
        if(reply == NULL){
          cerr<<"Reply Null."<<endl;
        } else {
          KVData<string> ret = KVData<string>();
          if (reply->type == REDIS_REPLY_INTEGER){
            if(reply->integer <= 0){
              ret.ierr = -1; /*reply->integer;*/
              ret.serr = "Value doesn't exists.";
            } else {
              ret.ierr = 0;
            }
          } else {
            cerr<<"Reply type:"<<reply->type<<endl;
          }
          freeReplyObject(reply);
          vret.push_back(ret);
        }
      }
    }
    return 0;
  }

  void KVImplHelper::async_get(string key, void (*fn)(KVData<string>,void *), void *vfn){
    c_kvsclient->mtx.lock();
    int ret = redisClusterAppendCommand(c_kvsclient->rc, "get %s",(c_kvsclient->tablename+key).c_str()) ;
    c_kvsclient->mtx.unlock();
    if(ret!= REDIS_ERR){
      struct async_data ad{fn,vfn,1};
      c_kvsclient->mtx.lock();
      c_kvsclient->q.push(ad);
      c_kvsclient->mtx.unlock();
    }	else {
      cerr<<"\n\n\nget Append error\n\n\n"<<endl;
    }
  }

  void KVImplHelper::async_put(string key,string val, void (*fn)(KVData<string>,void *), void *vfn){
    c_kvsclient->mtx.lock();
    int ret = redisClusterAppendCommand(c_kvsclient->rc, "set %s %s",(c_kvsclient->tablename+key).c_str(),val.c_str()) ;
    c_kvsclient->mtx.unlock();
    if(ret!= REDIS_ERR){
      struct async_data ad{fn,vfn,2};
      c_kvsclient->mtx.lock();
      c_kvsclient->q.push(ad);
      c_kvsclient->mtx.unlock();
    }	else {
      cerr<<"Put error in Redis. Reason may be resource contention."<<endl;
    }
  }
  void KVImplHelper::async_del(string key, void (*fn)(KVData<string>,void *), void *vfn){
    c_kvsclient->mtx.lock();
    int ret = redisClusterAppendCommand(c_kvsclient->rc, "del %s",(c_kvsclient->tablename+key).c_str()) ;
    c_kvsclient->mtx.unlock();
    if(ret!= REDIS_ERR){
      struct async_data ad{fn,vfn,3};
      c_kvsclient->mtx.lock();
      c_kvsclient->q.push(ad);
      c_kvsclient->mtx.unlock();
    }	else {
      cerr<<"Del error in Redis. Reason may be resource contention."<<endl;
    }
  }


  void KVImplHelper::async_get(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    c_kvsclient->mtx.lock();
    int ret = redisClusterAppendCommand(c_kvsclient->rc, "get %s",(tablename+key).c_str()) ;
    c_kvsclient->mtx.unlock();
    if(ret!= REDIS_ERR){
      struct async_data ad{fn,vfn,4};
      c_kvsclient->mtx.lock();
      c_kvsclient->q.push(ad);
      c_kvsclient->mtx.unlock();
    }	else {
      cerr<<"Get error in Redis. Reason may be resource contention."<<endl;
    }
  }

  void KVImplHelper::async_put(string key,string val, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    c_kvsclient->mtx.lock();
    int ret = redisClusterAppendCommand(c_kvsclient->rc, "set %s %s",(tablename+key).c_str(),val.c_str()) ;
    c_kvsclient->mtx.unlock();
    if(ret!= REDIS_ERR){
      struct async_data ad{fn,vfn,2};
      c_kvsclient->mtx.lock();
      c_kvsclient->q.push(ad);
      c_kvsclient->mtx.unlock();
    }	else {
      cerr<<"Put error in Redis. Reason may be resource contention."<<endl;
    }
  }
  void KVImplHelper::async_del(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn){
    c_kvsclient->mtx.lock();
    int ret = redisClusterAppendCommand(c_kvsclient->rc, "del %s",(tablename+key).c_str()) ;
    c_kvsclient->mtx.unlock();
    if(ret!= REDIS_ERR){
      struct async_data ad{fn,vfn,3};
      c_kvsclient->mtx.lock();
      c_kvsclient->q.push(ad);
      c_kvsclient->mtx.unlock();
    }	else {
      cerr<<"Del error in Redis. Reason may be resource contention."<<endl;
    }
  }


  int KVImplHelper::smget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& vret){
    //---------Yet non functional.--------------
    int sz = key.size();
    int rep, rep2;
    redisReply *reply;

    rep = redisClusterAppendCommand(c_kvsclient->rc, "unwatch");
    if(rep == REDIS_ERR){
      cerr<<"Unwatch Append error"<<endl;
    }
    for(int i=0;i<sz;i++){
      rep = redisClusterAppendCommand(c_kvsclient->rc, "watch %s",(tablename[i]+key[i]).c_str()) ;
      rep2 = redisClusterAppendCommand(c_kvsclient->rc, "get %s",(tablename[i]+key[i]).c_str()) ;
      if(rep == REDIS_ERR || rep2 == REDIS_ERR){
        cerr<<"Get Append error"<<endl;
      }
    }

    rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
    if(rep == REDIS_ERR){
      cerr<<"Unwatch reply error"<<endl;
    }
    if(reply != NULL){
      freeReplyObject(reply);
    }
    for(int i=0;i<sz;i++){
      rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
      if(rep == REDIS_ERR){
        cerr<<"Watch reply error"<<endl;
      }
      if(reply != NULL){
        freeReplyObject(reply);
      }
      rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
      if(rep == REDIS_OK){
        if(reply == NULL){
          cerr<<"Reply Null"<<endl;
        } else {
          KVData<string> ret = KVData<string>();
          if(reply->type == REDIS_REPLY_STRING){
            ret.ierr = 0;
            ret.value = string(reply->str);
          } else if (reply->type == REDIS_REPLY_NIL){
            ret.ierr = -1;
            ret.serr = "Value doesn't exists.";
          } else {
            cerr<<"Reply type:"<<reply->type<<endl;
          }
          freeReplyObject(reply);
          vret.push_back(ret);
        }
      } else {
        cerr<<"Error in return file:"<<__FILE__<<" line:"<<__LINE__<<endl;
        // redisClusterReset(cc);
      }
    }
    return 0;
  }

  int KVImplHelper::smput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& vret){
    int sz = key.size();
    int rep;
    redisReply *reply;

    rep = redisClusterAppendCommand(c_kvsclient->rc, "multi");
    if(rep == REDIS_ERR){
      cerr<<"Multi Append error rep:"<<rep<<endl;
    }
    rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
    if(rep == REDIS_ERR){
      cerr<<"Multi reply error"<<endl;
    }
    if(reply != NULL){
      freeReplyObject(reply);
    }

    for(int i=0;i<sz;i++){
      rep = redisClusterAppendCommand(c_kvsclient->rc, "set %s %s",(tablename[i]+key[i]).c_str(), val[i].c_str()) ;
      if(rep == REDIS_ERR){
        cerr<<"Set Append error"<<endl;
      }
    }
    rep = redisClusterAppendCommand(c_kvsclient->rc, "exec");
    if(rep == REDIS_ERR){
      cerr<<"Exec Append error rep:"<<rep<<endl;
    }

    for(int i=0;i<sz;i++){
      rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
      if(rep == REDIS_OK){
        if(reply == NULL){
          cerr<<"Reply Null"<<endl;
        } else {
          KVData<string> ret = KVData<string>();
          if (reply->type == REDIS_REPLY_STATUS){
            ret.ierr = 0;
          } else {
            cerr<<"Reply type:"<<reply->type<<endl;
          }
          freeReplyObject(reply);
          vret.push_back(ret);
        }
      }
    }

    rep = redisClusterGetReply(c_kvsclient->rc, (void**)&reply);
    if(rep == REDIS_ERR){
      cerr<<"Exec reply error"<<endl;
    }
    if(reply != NULL){
      freeReplyObject(reply);
    } else {
      cerr<<"Exec reply null"<<endl;
      for(int i=0;i<sz;i++){
        vret[i].ierr = -1;
        vret[i].serr = "Safe put failed, values may have changed since last read.";
      }
      return -1;
    }
    return 0;
  }
}
