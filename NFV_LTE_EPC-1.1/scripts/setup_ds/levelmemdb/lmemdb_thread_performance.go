package main

import (
	"bytes"
	"fmt"
	"github.com/syndtr/goleveldb/leveldb/comparer"
	"github.com/syndtr/goleveldb/leveldb/memdb"
	"strconv"
	"sync"
	"time"
)

var wg sync.WaitGroup
var runs int = 100000
var threads int = 100
var key [][]byte = make([][]byte, runs)
var data [][]byte = make([][]byte, runs)

func do_put(tid int, db *memdb.DB) {
	//fmt.Println("PUT",tid)
	defer wg.Done()
	starttime := time.Now().UnixNano()
	for i := 0; i < runs; i++ {
		err := db.Put(key[i], data[i])
		if err != nil {
			fmt.Println("TID:",tid,"Put error for key:", key[i], "\nError:", err)
		}
	}
	endtime := time.Now().UnixNano()
	diff := endtime - starttime
	fmt.Println("TID:", tid, "MemDB Duration  for", runs, " write operations in Nanoseconds:", diff, " Avg:", diff/int64(runs))
}

func do_get(tid int, db *memdb.DB) {
	//fmt.Println("GET",tid)
	defer wg.Done()
	starttime := time.Now().UnixNano()
	for i := 0; i < runs; i++ {
		val, err := db.Get(key[i]) //?data and error not handled
		if err != nil {
			fmt.Println("TID:",tid,"Get error for key:", key[i], "\nError:", err)
		} else {
			if bytes.Compare(val, data[i]) != 0 {
				fmt.Println("Invalid data at key:", key[i])
				fmt.Println("Required Data :", data[i])
				fmt.Println("Received Data :", val[i])
			}
		}
	}
	endtime := time.Now().UnixNano()
	diff := endtime - starttime
	fmt.Println("TID:", tid, "MemDB Duration  for", runs, " read + compare operations in Nanoseconds:", diff, " Avg:", diff/int64(runs))
}

func main() {
	for i := 0; i < runs; i++ {
		key[i] = []byte(strconv.Itoa(i))
		data[i] = []byte("012345678_012345678_012345678_012345678_012345678_012345678_012345678_012345678_012345678_987654321:" + strconv.Itoa(i))
	}
	/*
	   New creates a new initalized in-memory key/value DB. The capacity is the initial key/value buffer capacity. The capacity is advisory, not enforced.
	   The returned DB instance is goroutine-safe.
	*/
	db := memdb.New(comparer.DefaultComparer, runs)
	//Initial write to enture do_get doesnt throws error
	wg.Add(1)
	do_put(0, db)
	wg.Add(1)
	do_get(0, db)
	for i := 1; i <= threads; i++ {
		wg.Add(1)
		go do_put(i, db)

		wg.Add(1)
		go do_get(i, db)
	}
	wg.Wait()
}
