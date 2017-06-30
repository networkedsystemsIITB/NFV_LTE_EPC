#include "../KVStore.h"
#include<iostream>
#include<thread>

#include<vector>

using namespace kvstore;
using namespace std;

int main(){


  KVRequest kr;
  kr.bind("127.1.1.1:8090");//Invoked exactly once
  // kr.put<int,string>(10,"OM","T1");
  // kr.put<int,double>(10,0.9876,"T2");
  // kr.get<int,string>(10,"T1");
  // kr.get<int,double>(10,"T2");
  // kr.del<int,double>(10,"T2");
  // kr.get<int,double>(10,"T2");

  auto rs = kr.execute();
  // cout<<"RS size"<<rs.size()<<endl;
  //     cout<<"Get 10 T1 :"<<rs.get<string>(2).value<<endl;
  //     cout<<"Get 10 T2 :"<<rs.get<double>(3).value<<endl;
  //     cout<<"Get 10 T2 :"<<rs.get<double>(6).serr<<endl;
  //
  //
  //      rs=kr.execute();
  //   cout<<"RS size"<<rs.size()<<endl;
  //       cout<<"Get 10 T1 :"<<rs.get<string>(2).value<<endl;
  //       cout<<"Get 10 T2 :"<<rs.get<double>(3).value<<endl;
  cout<<"Get 10 T2 :"<<rs.get<double>(5).serr<<endl;

  //cout<<"v[0].value:"<<v[0].value<<"\nv[1].value:"<<v[1].value<<"\nv[2].value:"<<v[2].value<<endl;
  return 0;
}
