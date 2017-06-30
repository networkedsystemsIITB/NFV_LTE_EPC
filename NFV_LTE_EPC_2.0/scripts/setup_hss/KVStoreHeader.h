// Why both implementation and declaration are in same file
// Refer : https://isocpp.org/wiki/faq/templates
//        http://stackoverflow.com/questions/3040480/c-template-function-compiles-in-header-but-not-implementation

#ifndef __KVSTORE_H__
#define __KVSTORE_H__

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>

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
