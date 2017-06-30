package lmemedb_kvstore

import (
	"github.com/syndtr/goleveldb/leveldb/memdb"
	"github.com/syndtr/goleveldb/leveldb/comparer"
	//"fmt"
	//"strconv"
	//"time"
  //kvstore "go_workspace/src/leveldb_test/lmemedb_kvstore"//Import path
	)



  type TableManager struct {
    tablemap map[string]*memdb.DB
  }

  type KVS_LMemDB struct {
    db *memdb.DB
  }

	func NewTableManager() *TableManager{
		tm := new(TableManager)
		tm.tablemap = make(map[string]*memdb.DB)
		return tm
	}

    func (kvs *KVS_LMemDB) CreateTable(tablename string, tm *TableManager) bool{
    db,ok := tm.tablemap[tablename]
    if ok {
      kvs.db = db
    } else {
      	db = memdb.New(comparer.DefaultComparer,100000)
        tm.tablemap[tablename] = db
        kvs.db = db
    }
    return true
  }

  func (kvs *KVS_LMemDB) Get(key string) (string,string,int) {
    data,err := kvs.db.Get([]byte(key))
    if err!=nil {
      return "",err.Error(),-1
    } else {
      return  string(data) , "", 0
    }
  }

  func (kvs *KVS_LMemDB) Put(key string,val string) (string,int) {
    err := kvs.db.Put([]byte(key),[]byte(val))
    if err!=nil {
      return err.Error(),-1
    } else {
      return "",0
    }
  }

  func (kvs *KVS_LMemDB) Del(key string) (string,int) {
    err := kvs.db.Delete([]byte(key))
    if err!=nil {
      return err.Error(),-1
    } else {
      return "",0
    }
  }

  func (kvs *KVS_LMemDB) Clear() (bool) {
    kvs.db.Reset()
    return true
  }
