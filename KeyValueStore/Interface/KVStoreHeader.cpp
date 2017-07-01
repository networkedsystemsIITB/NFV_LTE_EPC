/*
   This file contains the non-template implementation of KVStoreHeader
*/

#include "KVStoreHeader.h"

namespace kvstore {
/*
	KVResultSet non template implementation
*/
		KVResultSet::KVResultSet(vector<KVData<string>> r, vector<string> opr_type){
			res=r;
			operation_type=opr_type;
		}
		/* Return the size of ResultSet */
		int KVResultSet::size(){
			return res.size();
		}
		/* Returns the operation type */
		string KVResultSet::oprType(int idx){
			if(idx<0 || idx>=size()){
      if(size()==0)
	      return "Empty KVResultSet due to empty request queue, please ensure that your request queue is not empty.";
      else
	      return "KVResultSet index out of bound. Valid range is 0 to "+to_string(size()-1)+" found "+to_string(idx);
			}
			return operation_type[idx];
		}



	KVRequest::KVRequest(){}

	KVRequest::~KVRequest(){}

	bool KVRequest::bind(string connection){
		return kh.bind(connection,"randomtable_123321011422");
	}

	KVResultSet KVRequest::execute(){
		vector<KVData<string>> mput_res;
		vector<KVData<string>> mget_res;
		vector<KVData<string>> mdel_res;
		vector<KVData<string>> smput_res;
		vector<KVData<string>> smget_res;

		int mp = kh.mput(put_key, put_value, put_tablename, mput_res);
		int mg = kh.mget(get_key, get_tablename, mget_res);
		int md = kh.mdel(del_key, del_tablename, mdel_res);
		if(sget_key.size() > 0)
		int smg = kh.smget(sget_key, sget_tablename, smget_res);
		if(sput_key.size() > 0)
		int smp = kh.smput(sput_key, sput_value, sput_tablename, smput_res);

		vector<KVData<string>> combined_res;
		int sz=operation_type.size();
		int pi=0,gi=0,di=0,spi=0,sgi=0;
		for(int i=0;i<sz;i++){
			if(operation_type[i] == OPR_TYPE_PUT){
				combined_res.push_back(mput_res[pi]);
				pi++;
			} else if(operation_type[i] == OPR_TYPE_GET){
				combined_res.push_back(mget_res[gi]);
				gi++;
			} else if(operation_type[i] == OPR_TYPE_DEL){
				combined_res.push_back(mdel_res[di]);
				di++;
			} else if(operation_type[i] == OPR_TYPE_SPUT){
				combined_res.push_back(smput_res[spi]);
				spi++;
			} else if(operation_type[i] == OPR_TYPE_SGET){
				combined_res.push_back(smget_res[sgi]);
				sgi++;
			} else {
				cerr<<"Invalid operation type in "<<__FILE__<<", "<<__FUNCTION__<<endl;
			}
		}
		KVResultSet rs = KVResultSet(combined_res,operation_type);
		return rs;
	}

/*---------------Helper for async_execute --------------------*/
	async_execute_data::async_execute_data(void *d,vector<string> &ot){
		vfn=d;
		operation_type=ot;
	}

  void fun(KVData<string> kd, void *data){
		static vector<KVData<string>> combined_res;
		combined_res.push_back(kd);
		if(data != NULL){
			async_execute_data *ad = (async_execute_data*)data;
			KVResultSet rs = KVResultSet(combined_res,ad->operation_type);
			function<void(KVResultSet)> *fn = (function<void(KVResultSet)> *) ad->vfn;
			(*fn)(rs);
			delete(fn);
			delete(ad);
			combined_res.clear();
		}
	}

	void KVRequest::reset(){
		operation_type.clear();
		put_tablename.clear();
		put_key.clear();
		put_value.clear();
		get_tablename.clear();
		get_key.clear();
		del_tablename.clear();
		del_key.clear();

		sput_tablename.clear();
		sput_key.clear();
		sput_value.clear();
		sget_tablename.clear();
		sget_key.clear();
	}
}
