package main

import (
  "os"
  "fmt"
  "net"
  "strconv"
  kvstore "levelmemdb/lmemdb_kvstore"
)

/*
type KVStoreClient struct {
var conn net.Conn
}
*/

var tm kvstore.TableManager

type Client struct {
  kvs kvstore.KVS_LMemDB
}


func itoba(i int) []byte {
    slen := i
    bytearr := make([]byte,4)
    idx:=0
    bytearr[idx] = byte((slen >> 24) & 255)
    idx++
    bytearr[idx] = byte((slen >> 16) & 255)
    idx++
    bytearr[idx] = byte((slen >> 8) & 255)
    idx++
    bytearr[idx] = byte((slen) & 255)
    idx++
    return bytearr
}

func batoi(bytearr []byte) int{
  idx:=4
  x:=int(bytearr[idx-4])<<24 + int(bytearr[idx-3])<<16 + int(bytearr[idx-2])<<8 + int(bytearr[idx-1])
  return x
}

func (cl *Client) performTask(strarr []string) []string{
  switch strarr[0] {
  case "CreateTable":
    cl.kvs.CreateTable(strarr[1],&tm)
    return []string{"true"}
  case "Get":
    val,serr,ierr := cl.kvs.Get(strarr[1])
    return []string{val,serr,strconv.Itoa(ierr)}
  case "Put":
    serr,ierr := cl.kvs.Put(strarr[1],strarr[2])
    return []string{"",serr,strconv.Itoa(ierr)}
  case "Del":
    serr,ierr := cl.kvs.Del(strarr[1])
    return []string{"",serr,strconv.Itoa(ierr)}
  case "Clear":
    cl.kvs.Clear()
    return []string{"true"}
  }
  return []string{""}
}


func (cl *Client) doTask(strarr []string,conn net.Conn){
  //fmt.Println("Strings\n",strarr)
  ret := cl.performTask(strarr)
  writeStrings(ret,conn)
  //time.Sleep(5000)
  //os.Exit(0)
}

func writeStrings(strarr []string,conn net.Conn){
  alen := len(strarr)
  sum := 1
  for i:=0;i<alen;i++ {
    sum += (len(strarr[i])+4)
  }
  //fmt.Println("Sum",sum)
  bytearr := make([]byte,sum)
  idx := 0
  bytearr[0] = byte(alen)
  idx++
  for i:=0;i<alen;i++ {
    slen := len(strarr[i])
    bytearr[idx] = byte((slen >> 24) & 255)
    idx++
    bytearr[idx] = byte((slen >> 16) & 255)
    idx++
    bytearr[idx] = byte((slen >> 8) & 255)
    idx++
    bytearr[idx] = byte((slen) & 255)
    idx++

    copy(bytearr[idx:],[]byte(strarr[i]))
    idx+=slen

  }
  conn.Write(bytearr) //? err
}

func readString(conn net.Conn) string{
  lenbuf := make([]byte,4)
  /*n,err := */conn.Read(lenbuf) //? err
  var slen int
  slen = int(lenbuf[0])<<24 + int(lenbuf[1])<<16 + int(lenbuf[2])<<8 + int(lenbuf[3])
  //fmt.Println("Slen:",slen)
  strbuf := make([]byte,slen)
  dsum:=0;
  for dsum!=slen{
    n,_ := conn.Read(strbuf[dsum:]) //? err
    dsum+=n
  }
  return string(strbuf)
}

func readStrings(conn net.Conn) ([]string, error) {
  no_of_string := make([]byte,1)
  _,err := conn.Read(no_of_string) //? err
  if(err!=nil){
    return nil,err
  }
  strarr := make([]string,no_of_string[0])
  for i:=byte(0);i<no_of_string[0];i++ {
    strarr[i] = readString(conn)
  }
  return strarr,nil
}

func handleClient(conn net.Conn) {
  var cl Client
  //i := 0
  for {
    strarr,err := readStrings(conn)
      //i++
      //fmt.Println("Iter:",i,"Strings",strarr)
    if err!=nil {
      //fmt.Println("returned")
      return
    }
    cl.doTask(strarr,conn)
  }
}

func main() {
  socket := os.Args[1] //Read form command line arugment
  ln, err := net.Listen("tcp",socket)
  if err != nil {
    panic(err)
    return
    } else {
      tm = *kvstore.NewTableManager()
      fmt.Println("Server started at",os.Args[1])
      for{
        conn, err := ln.Accept()
        if err != nil {
          continue
        }
        go handleClient(conn)
      }
    }
  }
