/*
 This file contains the declaration of helper class KVImplHelper.
 This file is only to be used by developer and is not usefull to Key-Value store
 users directly.
*/

#ifndef __KVIMPL_H__
#define __KVIMPL_H__

#include <iostream>
#include <vector>
#include <memory>

using namespace std;

namespace kvstore {
	/*
	KVData :
	Data wrapper object for returning response of any key value operation.
	*/
	template<typename ValType>
	class KVData {
	public:
		/* String Error : It contains the error discription */
		string serr;
		/* Integer Error : It contains the integer id of an error and 0 (zero) if no error */
		int ierr;
		/* Value : It contains value for get operation */
		ValType value;
	};


	/*
	KVImplHelper :
	This is the helper Class to be implemented for specific key value stores.
	This calss is not directly used by user, but is used by internal operations
	of KVStoreHeader.h
	*/
	class KVImplHelper{
	private:
		/* Generic pointer to hold any data usefull for implementation. */
		void *dataholder=NULL;
	public:
		/* Default constructor */
		KVImplHelper();
		/* Distructor (Don't forget to delete/free dataholder ptr here, if used) */
		~KVImplHelper();
		/* Copy constructor */
		KVImplHelper(const KVImplHelper&);

		/*
		 Bind/connects to data store
		 Connection string : generaly it is socket address of data store server eg. "127.0.0.1:8080", but can be specific to data store.
		 Table Name : name of the table to which KVImplHelper is to be bound.
		*/
		bool bind(string connection,string tablename);


		KVData<string> get(string const& key);
		KVData<string> put(string const& key,string const& val);
		KVData<string> del(string const& key);
		void async_get(string key, void (*fn)(KVData<string>,void *), void *vfn);
		void async_put(string key, string val, void (*fn)(KVData<string>,void *), void *vfn);
		void async_del(string key, void (*fn)(KVData<string>,void *), void *vfn);
		bool clear();

		int mget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret);
		int mdel(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret);
		int mput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& ret);

		void async_get(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn);
		void async_put(string key, string val, string tablename, void (*fn)(KVData<string>, void *), void *vfn);
		void async_del(string key, string tablename, void (*fn)(KVData<string>,void *), void *vfn);

		int smget(vector<string>& key, vector<string>& tablename, vector<KVData<string>>& ret);
		int smput(vector<string>& key, vector<string>& val, vector<string>& tablename, vector<KVData<string>>& ret);
	};
}
#endif
