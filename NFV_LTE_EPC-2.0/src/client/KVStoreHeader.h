// Why both implementation and declaration are in same file
// Refer : https://isocpp.org/wiki/faq/templates
//        http://stackoverflow.com/questions/3040480/c-template-function-compiles-in-header-but-not-implementation
// Note that only implementation of templates are here since their true implementation is defered till their use.

#ifndef __KVSTORE_H__
#define __KVSTORE_H__

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>

//For returning hetrogeneous array as return of execute()
//#include <tuple> //Not used

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/hash_map.hpp>
#include <boost/serialization/hash_set.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/slist.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/string.hpp>
#include <boost/shared_ptr.hpp>


using namespace std;
using namespace boost;

namespace kvstore {

	template<typename KeyType, typename ValType>
	class KVStore;
	template<typename ValType>
	class KVData;

	class KVResultSet {
	private:
		int count;
		vector<string> res;
	public:
		KVResultSet(vector<string> r);
		int size();
		template<typename ValType>
		KVData<ValType> get(int idx);
	};

	class KVRequest {
	private:
		int count=0;
		void *kvsclient;
		std::vector<string> v;
	public:
		void bind(string connection);
		~KVRequest();		//For distroying connection object
		template<typename KeyType, typename ValType>
		void get(KeyType const& key,string tablename);

		template<typename KeyType, typename ValType>
		void put(KeyType const& key,ValType const& val,string tablename);

		template<typename KeyType, typename ValType>
		void del(KeyType const& key,string tablename);

		KVResultSet execute();
		void reset();
	};


	template<typename KeyType, typename ValType>
	class KVStore {
	private:
		void *kvsclient;
	public:
		KVStore();
		~KVStore();
		bool bind(string connection,string tablename);
		KVData<ValType> get(KeyType const& key);
		KVData<ValType> put(KeyType const& key,ValType const& val);
		KVData<ValType> del(KeyType const& key);
		bool clear();
	};



	template<typename ValType>
	class KVData {
	public:
		string serr;
		int ierr;
		ValType value;

		//void set(string se,int ie,string val);
	};

	template<typename T>
	string toBoostString(T const &obj);

	template<typename T>
	T const & toBoostObject(string sobj);

	//------------Common implementation--------------------
	template<typename T>
	string toBoostString(T const &obj){
		stringstream ofs;
		boost::archive::text_oarchive oa(ofs);
		oa << obj;
		return ofs.str();
	}


	template<typename T>
	T const & toBoostObject(string sobj){
		T *obj = new(T);
		stringstream ifs;
		ifs<<sobj;
		boost::archive::text_iarchive ia(ifs);
		ia >> (*obj);
		return *obj;
	}
}
#endif
