#include "KVStore.h"

namespace kvstore {


  KVMgmt kvm;

  bool KVStoreClient::init(string ip, string port){
    bool b=true;
    kvm.mtx.lock();
    if(kvm.rc == NULL){
      kvm.rc = new CRedisClient();
      b=kvm.rc->Initialize(ip,stoi(port),10,100);
    }
    rc = kvm.rc;
    kvm.mtx.unlock();
    return b;
  }

  // class KVStoreClient{
  // public:
  //   CRedisClient rc;
  //   string tbname;
  //   ~KVStoreClient(){
  //     delete rc;
  //   }
  // };

  KVResultSet::KVResultSet(vector<string> r){
    count=r.size()/3;
    res=r;
  }

  int KVResultSet::size(){
    return count;
  }

  void KVRequest::bind(string connection){
    int colon = connection.find(":");
    string ip = connection.substr(0,colon);
    string port = connection.substr(colon+1);
    kvsclient = (void *) new KVStoreClient();
    if(c_kvsclient->init(ip,port)){
      cerr<<"connection error"<<endl;
    }
  }
  KVRequest::~KVRequest(){
    delete(c_kvsclient);
    //For distroying connection object
  }

  // void printvec(vector<string> &v){
  //   for(string s:v){
  //     cout<<s<<endl;
  //   }
  // }

  KVResultSet KVRequest::execute(){
    // cout<<"KVReq Execute"<<endl;
    vector<string> res;
    int pr=1,gr=1;
    if(vputk.size()!=0){
      pr = c_kvsclient->rc->Mset(vputk,vputv);
    }

    vector<string> getres;
    if(vget.size()!=0){
      gr = c_kvsclient->rc->Mget(vget,&getres);
    }

    vector<int> delres(vdel.size());
    for(int i=0;i<vdel.size();i++){
      delres[i] = c_kvsclient->rc->Del(vdel[i]);
    }

    int getidx=0;
    int delidx=0;
    for(int i=0;i<v.size();i++){
      switch(v[i][0]){
        case 'p' :
        if(pr == RC_SUCCESS){
          // cout<<"Put success"<<endl;
          res.push_back("0");
          res.push_back("");
          res.push_back("");
        } else {
          res.push_back("-1");
          res.push_back("connection error");
          res.push_back("");
        }
        break;

        case 'g' :
        if(gr == RC_SUCCESS){

          // cout<<"Get success :"<<getres[getidx]<<endl;
          if(getres[getidx].empty()){
            res.push_back("-1");
            res.push_back("Value not found");
            res.push_back("");
          }else {
            res.push_back("0");
            res.push_back("");
            res.push_back(getres[getidx]);
          }
        } else {
          res.push_back("-1");
          res.push_back("connection error");
          res.push_back("");
        }
        getidx++;
        break;

        case 'd' :
        if(delres[delidx] == RC_SUCCESS){
          //cout<<"Del success"<<endl;
          res.push_back("0");
          res.push_back("");
          res.push_back("");
        } else {
          //cout<<"Del unsuccess"<<endl;
          res.push_back("-1");
          res.push_back("connection error");
          res.push_back("");
        }
        delidx++;
        break;
      }
    }

    return KVResultSet(res);
  }

  void KVRequest::reset(){
    v.clear();
    vputk.clear();
    vputv.clear();
    vget.clear();
    vdel.clear();
  }

}
