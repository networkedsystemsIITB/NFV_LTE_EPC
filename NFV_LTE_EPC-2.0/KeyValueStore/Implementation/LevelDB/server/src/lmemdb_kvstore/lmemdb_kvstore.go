package lmemedb_kvstore

import (
	"github.com/syndtr/goleveldb/leveldb"
	// "github.com/syndtr/goleveldb/leveldb/memdb"
	// "github.com/syndtr/goleveldb/leveldb/comparer"
	"sync"
	"fmt"
	"os"
	//"strconv"
	//"time"
	//kvstore "go_workspace/src/leveldb_test/lmemedb_kvstore"//Import path
)




type TableManager struct {
	tablemapMutex sync.Mutex;
	tablemap map[string]*leveldb.DB
}

type KVS_LMemDB struct {
	tablename string
	db *leveldb.DB
}

func NewTableManager() *TableManager{
	tm := new(TableManager)
	tm.tablemap = make(map[string]*leveldb.DB)
	return tm
}

//Locking required
func (kvs *KVS_LMemDB) CreateTable(tablename string, tm *TableManager) bool{
	tm.tablemapMutex.Lock();
	defer tm.tablemapMutex.Unlock();
	db,ok := tm.tablemap[tablename]
	if ok {
		kvs.tablename = tablename
		kvs.db = db
		} else {
			//fmt.Println("Created New Table!!!");
			db,err := leveldb.OpenFile(tablename,nil); //? err
			if err!= nil {
				fmt.Println("CN:",err)
				return false
			}
			tm.tablemap[tablename] = db
			kvs.tablename = tablename
			kvs.db = db
		}
		return true
	}

	func (kvs *KVS_LMemDB) Get(key string) (string,string,int) {
		if(kvs.db == nil){
				return "","Can not access table",-1
		}
		data,err := kvs.db.Get([]byte(key),nil)
		//fmt.Println(err)
		if err!=nil {
			return "",err.Error(),-1
			} else {
				return  string(data) , "", 0
			}
		}

		func (kvs *KVS_LMemDB) Put(key string,val string) (string,int) {
			if(kvs.db == nil){
					return "Can not access table",-1
			}
			err := kvs.db.Put([]byte(key),[]byte(val),nil)
			if err!=nil {
				return err.Error(),-1
				} else {
					return "",0
				}
			}

			func (kvs *KVS_LMemDB) Del(key string) (string,int) {
				if(kvs.db == nil){
						return "Can not access table",-1
				}
				err := kvs.db.Delete([]byte(key),nil)
				if err!=nil {
					return err.Error(),-1
					} else {
						return "",0
					}
				}

				func (kvs *KVS_LMemDB) Clear( tm *TableManager) (bool) {
					if(kvs.db == nil){
							return false
					}
					tm.tablemapMutex.Lock();
					defer tm.tablemapMutex.Unlock();
					kvs.db.Close();
					err := os.RemoveAll(kvs.tablename)
					if err!= nil {
						fmt.Println("Clear:",err)
						return false
					}
					db,err := leveldb.OpenFile(kvs.tablename,nil); //? err
					if err!= nil {
						fmt.Println("Clear2:",err)
						return false
					}
					tm.tablemap[kvs.tablename] = db
					kvs.db = db
					return true
				}
