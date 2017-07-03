#ifndef __KVSTORE_H__
#define __KVSTORE_H__

#include "KVImpl.h"

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <functional>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>


using namespace std;

namespace kvstore {

	#define OPR_TYPE_GET string("get")
	#define OPR_TYPE_PUT string("put")
	#define OPR_TYPE_DEL string("del")
	#define OPR_TYPE_SGET string("sget")
	#define OPR_TYPE_SPUT string("sput")

	/* Class declaration */
	template<typename KeyType, typename ValType>
	class KVStore;
	class KVResultSet;
	class KVRequest;
	template<typename ValType>
	class KVData;

	/*
	Helper declarations
	*/
	template<typename T>
	string toBoostString(T const &obj);

	template<typename T>
	T toBoostObject(string sobj);

	/*
	KVResultSet holds results of KVRequest.execute();
	*/
	class KVResultSet {
	private:
		KVData<string> extra;
		vector<KVData<string>> res;
		vector<string> operation_type;
	public:
		KVResultSet(vector<KVData<string>> r, vector<string> opr_type);
		int size();
		string oprType(int idx);

		template<typename ValType>
		KVData<ValType> get(int idx);
	};
	/*----------KVResultSet::get()-----------*/
	template<typename ValType>
	KVData<ValType> KVResultSet::get(int idx){
		KVData<ValType> ret = KVData<ValType>();
		int count = size();
		if(idx>=0 && idx<count){
			ret.ierr = res[idx].ierr;
			ret.serr = res[idx].serr;
			if(ret.ierr==0 && (operation_type[idx]==OPR_TYPE_GET || operation_type[idx]==OPR_TYPE_SGET)){
				ret.value = toBoostObject<ValType>(res[idx].value);
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


	/*
	This class mearges multiple operations into one request.
	*/
	class KVRequest {
	private:
		void *dataholder;
		KVImplHelper kh;

		std::vector<string> operation_type;

		std::vector<string> put_tablename;
		std::vector<string> put_key;
		std::vector<string> put_value;

		std::vector<string> get_tablename;
		std::vector<string> get_key;

		std::vector<string> del_tablename;
		std::vector<string> del_key;

		std::vector<string> sput_tablename;
		std::vector<string> sput_key;
		std::vector<string> sput_value;

		std::vector<string> sget_tablename;
		std::vector<string> sget_key;
	public:
		KVRequest();
		~KVRequest();

		bool bind(string connection);

		template<typename KeyType, typename ValType>
		void get(KeyType const& key,string tablename);

		template<typename KeyType, typename ValType>
		void put(KeyType const& key,ValType const& val,string tablename);

		template<typename KeyType, typename ValType>
		void sget(KeyType const& key,string tablename);

		template<typename KeyType, typename ValType>
		void sput(KeyType const& key,ValType const& val,string tablename);

		template<typename KeyType, typename ValType>
		void del(KeyType const& key,string tablename);

		KVResultSet execute();
		template<typename Fn, typename... Args>
		void async_execute(Fn&& f, Args&&... args); /* */
		void reset();
	};

	/*-------KVRequest::get()----------*/
	template<typename KeyType, typename ValType>
	void KVRequest::get(KeyType const& key,string tablename){
		string skey=toBoostString(key);
		operation_type.push_back(OPR_TYPE_GET);
		get_key.push_back(skey);
		get_tablename.push_back(tablename);
	}
	/*-------KVRequest::put()----------*/
	template<typename KeyType, typename ValType>
	void KVRequest::put(KeyType const& key,ValType const& val,string tablename){
		string skey=toBoostString(key);
		string sval=toBoostString(val);
		operation_type.push_back(OPR_TYPE_PUT);
		put_key.push_back(skey);
		put_value.push_back(sval);
		put_tablename.push_back(tablename);
	}

	/*-------KVRequest::sget()----------*/
	template<typename KeyType, typename ValType>
	void KVRequest::sget(KeyType const& key,string tablename){
		string skey=toBoostString(key);
		operation_type.push_back(OPR_TYPE_SGET);
		sget_key.push_back(skey);
		sget_tablename.push_back(tablename);
	}
	/*-------KVRequest::sput()----------*/
	template<typename KeyType, typename ValType>
	void KVRequest::sput(KeyType const& key,ValType const& val,string tablename){
		string skey=toBoostString(key);
		string sval=toBoostString(val);
		operation_type.push_back(OPR_TYPE_SPUT);
		sput_key.push_back(skey);
		sput_value.push_back(sval);
		sput_tablename.push_back(tablename);
	}
	/*-------KVRequest::del()----------*/
	template<typename KeyType, typename ValType>
	void KVRequest::del(KeyType const& key,string tablename){
		string skey=toBoostString(key);
		operation_type.push_back(OPR_TYPE_DEL);
		del_key.push_back(skey);
		del_tablename.push_back(tablename);
	}
	/*----------- KVRequest::async_execute() ---*/
	class async_execute_data{
		public:
			void *vfn;
			vector<string> operation_type;
			async_execute_data(void *d,vector<string> &ot);
	};
	void fun(KVData<string> kd, void *data);
	template<typename Fn, typename... Args>
	void KVRequest::async_execute(Fn&& f, Args&&... args){
		int sz=operation_type.size()-1;  /*NOTE -1 for last operation*/
		int gi=0,pi=0,di=0;
		for(int i=0;i<sz;i++){
			if(operation_type[i] == OPR_TYPE_GET){
				kh.async_get(get_key[gi], get_tablename[gi], fun, NULL);
				gi++;
			} else if(operation_type[i] == OPR_TYPE_PUT){
				kh.async_put(put_key[pi], put_value[pi], put_tablename[pi], fun, NULL);
				pi++;
			} else {
				kh.async_del(del_key[di], del_tablename[di], fun, NULL);
				di++;
			}
		}
		/*Last operation*/
		function<void(KVResultSet)> tfn = std::bind(f,args...,std::placeholders::_1);
		function<void(KVResultSet)> *fn = new function<void(KVResultSet)>(tfn);
		async_execute_data *ad = new async_execute_data((void*)fn,operation_type);
		if(operation_type[sz] == OPR_TYPE_GET){
			kh.async_get(get_key[gi], get_tablename[gi], fun, (void*)ad);
			gi++;
		} else if(operation_type[sz] == OPR_TYPE_PUT){
			kh.async_put(put_key[pi], put_value[pi], put_tablename[pi], fun, (void*)ad);
			pi++;
		} else {
			kh.async_del(del_key[di], del_tablename[di], fun, (void*)ad);
			di++;
		}
	}


	/*
	Interface to Key-Value Store
	*/
	template<typename KeyType, typename ValType>
	class KVStore {
	private:
		KVImplHelper kh;
		void *kvsclient;
	public:
		KVStore();
		~KVStore();
		bool bind(string connection,string tablename);
		KVData<ValType> get(KeyType const& key);
		KVData<ValType> put(KeyType const& key,ValType const& val);
		KVData<ValType> del(KeyType const& key);
		template<typename Fn, typename... Args>
		void async_get(KeyType const& key, Fn&& f, Args&&... args);
		template<typename Fn, typename... Args>
		void async_put(KeyType const& key,ValType const& val, Fn&& f, Args&&... args);
		template<typename Fn, typename... Args>
		void async_del(KeyType const& key, Fn&& f, Args&&... args);
		bool clear();
	};
	/*-------KVStore::KVStore()----------*/
	template<typename KeyType, typename ValType>
	KVStore<KeyType,ValType>::KVStore(){
	}
	/*-------KVStore::~KVStore()----------*/
	template<typename KeyType, typename ValType>
	KVStore<KeyType,ValType>::~KVStore(){
	}
	/*---------KVStore::bind()--------*/
	template<typename KeyType, typename ValType>
	bool KVStore<KeyType,ValType>::bind(string connection,string tablename){
		return kh.bind(connection,tablename);
	}
	/*-------KVStore::get()----------*/
	template<typename KeyType, typename ValType>
	KVData<ValType>  KVStore<KeyType,ValType>::get(KeyType const& key){
		string skey=toBoostString(key);
		KVData<ValType> kvd = KVData<ValType>();
		KVData<string> res = kh.get(skey);
		kvd.ierr = res.ierr;
		kvd.serr = res.serr;
		if(kvd.ierr==0){
			kvd.value = toBoostObject<ValType>(res.value);
		}
		return kvd;
	}
	/*-------KVStore::put()----------*/
	template<typename KeyType, typename ValType>
	KVData<ValType>  KVStore<KeyType,ValType>::put(KeyType const& key,ValType const& val){
		string skey=toBoostString(key);
		string sval=toBoostString(val);
		KVData<ValType> kvd = KVData<ValType>();
		KVData<string> res = kh.put(skey,sval);
		kvd.ierr = res.ierr;
		kvd.serr = res.serr;
		return kvd;
	}
	/*-------KVStore::del()----------*/
	template<typename KeyType, typename ValType>
	KVData<ValType>  KVStore<KeyType,ValType>::del(KeyType const& key){
		string skey=toBoostString(key);
		KVData<ValType> kvd = KVData<ValType>();
		KVData<string> res = kh.del(skey);
		kvd.ierr = res.ierr;
		kvd.serr = res.serr;
		return kvd;
	}
	/*-------KVRequest::async_get()--------*/
	template<typename KeyType, typename ValType>
	template<typename Fn, typename... Args>
	void KVStore<KeyType,ValType>::async_get(KeyType const& key, Fn&& f, Args&&... args){
		// cout<<"DP:"<<" file:"<<__FILE__<<" line:"<<__LINE__<<endl;
		string skey=toBoostString(key);
		function<void(KVData<ValType>)> tfn = std::bind(f,args...,std::placeholders::_1);
		function<void(KVData<ValType>)> *fn = new function<void(KVData<ValType>)>(tfn);
		auto lambda_fn = [](KVData<string> res, void *vfn)->void{
			KVData<ValType> kvd = KVData<ValType>();
			kvd.ierr = res.ierr;
			kvd.serr = res.serr;
			if(kvd.ierr==0){
				kvd.value = toBoostObject<ValType>(res.value);
			}
			function<void(KVData<ValType>)> *fn = (function<void(KVData<ValType>)> *)vfn;
			(*fn)(kvd);
			delete(fn);
		};
		kh.async_get(skey,lambda_fn,(void *)fn);
	}
	/*-------KVRequest::async_put()--------*/
	template<typename KeyType, typename ValType>
	template<typename Fn, typename... Args>
	void KVStore<KeyType,ValType>::async_put(KeyType const& key,ValType const& val, Fn&& f, Args&&... args){
		string skey=toBoostString(key);
		string sval=toBoostString(val);
		function<void(KVData<ValType>)> tfn = std::bind(f,args...,std::placeholders::_1);
		function<void(KVData<ValType>)> *fn = new function<void(KVData<ValType>)>(tfn);
		auto lambda_fn = [](KVData<string> res, void *vfn)->void{
			KVData<ValType> kvd = KVData<ValType>();
			kvd.ierr = res.ierr;
			kvd.serr = res.serr;
			function<void(KVData<ValType>)> *fn = (function<void(KVData<ValType>)> *)vfn;
			(*fn)(kvd);
			delete(fn);
		};
		kh.async_put(skey,sval,lambda_fn,(void *)fn);
	}
	/*-------KVRequest::async_del()--------*/
	template<typename KeyType, typename ValType>
	template<typename Fn, typename... Args>
	void KVStore<KeyType,ValType>::async_del(KeyType const& key, Fn&& f, Args&&... args){
		string skey=toBoostString(key);
		function<void(KVData<ValType>)> tfn = std::bind(f,args...,std::placeholders::_1);
		function<void(KVData<ValType>)> *fn = new function<void(KVData<ValType>)>(tfn);
		auto lambda_fn = [](KVData<string> res, void *vfn)->void{
			KVData<ValType> kvd = KVData<ValType>();
			kvd.ierr = res.ierr;
			kvd.serr = res.serr;
			function<void(KVData<ValType>)> *fn = (function<void(KVData<ValType>)> *)vfn;
			(*fn)(kvd);
			delete(fn);
		};
		kh.async_del(skey,lambda_fn,(void*)fn);
	}
	/*-------KVStore::clear()----------*/
	template<typename KeyType, typename ValType>
	bool KVStore<KeyType,ValType>::clear(){
		return kh.clear();
	}

	/*
	------------Common implementation--------------------
	*/
	template<typename T>
	string toBoostString(T const &obj){
		stringstream ofs;
		boost::archive::text_oarchive oa(ofs);
		oa << obj;
		return ofs.str();
	}

	template<typename T>
	T toBoostObject(string sobj){
		T obj;
		stringstream ifs;
		ifs<<sobj;
		boost::archive::text_iarchive ia(ifs);
		ia >> obj;
		return obj;
	}
}
#endif
