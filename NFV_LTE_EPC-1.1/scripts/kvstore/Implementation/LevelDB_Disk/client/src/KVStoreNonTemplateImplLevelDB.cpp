#include "KVStore.h"

namespace kvstore {

  KVResultSet::KVResultSet(vector<string> r){
    count=r.size()/4;
    res=r;
  }

  int KVResultSet::size(){
    return count;
  }

  void KVRequest::bind(string connection){
    int colon = connection.find(":");
    string ip = connection.substr(0,colon);
    string port = connection.substr(colon+1);
    kvsclient = (void *) new KVStoreClient(ip,stoi(port));
    v.push_back("Multiple");
  }
  KVRequest::~KVRequest(){
    //For distroying connection object
  }


  KVResultSet KVRequest::execute(){
    c_kvsclient->send(v);
    vector<string> rcv=c_kvsclient->receive();
    //cout<<"Received size"<<rcv.size()<<"  "<<rcv[0]<<endl;
    return KVResultSet(rcv);
  }

  void KVRequest::reset(){
    v.clear();
    v.push_back("Multiple");
  }

}
