package main

import (
  "os"
  "fmt"
  "net"
  "strconv"
  "syscall"
  //"runtime"
  "os/signal"
  "time"
  _ "net/http/pprof"
  "runtime/pprof"
  //"net/http"
  kvstore "levelmemdb/lmemdb_kvstore"
)

func temp_includes(){
  time.Now();
}


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

func (cl *Client) performTask(strarr []string) ([]string,int){
  used:=1
  switch strarr[0] {
  case "CreateTable":
    cl.kvs.CreateTable(strarr[1],&tm)
    used++;
    return []string{"true"},used
  case "Get":
    val,serr,ierr := cl.kvs.Get(strarr[1])
    used++;
    return []string{val,serr,strconv.Itoa(ierr)},used
  case "Put":
    serr,ierr := cl.kvs.Put(strarr[1],strarr[2])
    used+=2;
    return []string{"",serr,strconv.Itoa(ierr)},used
  case "Del":
    serr,ierr := cl.kvs.Del(strarr[1])
    used++;
    return []string{"",serr,strconv.Itoa(ierr)},used
  case "Clear":
    cl.kvs.Clear(&tm)
    return []string{"true"},used
  }
  return []string{""},used
}

func (cl *Client) performMultipleTasks(strarr []string) []string{
  l := len(strarr)
  ret := make([]string,0)
  idx := 0
  for idx!=l {
    s,n := cl.performTask(strarr[idx:])
    idx+=n
    ret = append(ret,s...)
  }
  return ret
}

func (cl *Client) doTask(strarr []string,conn net.Conn){
  if strarr[0]=="Multiple" {
    ret := cl.performMultipleTasks(strarr[1:])
    writeStrings(ret,conn)
  }else{
    ret,_ := cl.performTask(strarr)
    writeStrings(ret,conn)
  }
}

func writeStrings(strarr []string,conn net.Conn){
  alen := len(strarr)
  sum := 1
  for i:=0;i<alen;i++ {
    sum += (len(strarr[i])+4)
  }
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
  n,err := conn.Write(bytearr) //? err
  if n<sum || err!=nil {
    fmt.Println("Write error",err);
  }
}

func readString(conn net.Conn) (string,error){
  lenbuf := make([]byte,4)
  n,err := conn.Read(lenbuf) //? err
  if(n!=4 || err!=nil){
    fmt.Println("Error(1): n=",n," Err:",err)
  }
  var slen int
  slen = int(lenbuf[0])<<24 + int(lenbuf[1])<<16 + int(lenbuf[2])<<8 + int(lenbuf[3])
  strbuf := make([]byte,slen)
  dsum:=0;
  for dsum!=slen{
    n,err := conn.Read(strbuf[dsum:]) //? err
    if(err!=nil){
      fmt.Println("Error(2): n=",n," Err:",err)
      return "",err
    }
    dsum+=n
  }
  return string(strbuf),nil
}

func readStrings(conn net.Conn) ([]string, error) {
  no_of_string := make([]byte,1)
  n,err := conn.Read(no_of_string) //? err
  if((n!=1 || err!=nil)){
    return nil,err
  }
  strarr := make([]string,no_of_string[0])
  for i:=byte(0);i<no_of_string[0];i++ {
    strarr[i],err = readString(conn)
    if(err!=nil){
      fmt.Println("Error(4) due to error 2: n=",n," Err:",err)
      return nil,err
    }
  }
  return strarr,nil
}

func handleClient(conn net.Conn) {
  var cl Client
  for {
    strarr,err := readStrings(conn)
    if err!=nil {
      return
    }
    cl.doTask(strarr,conn)
  }
}

func main() {

  go func() {
  signal_chan := make(chan os.Signal, 1)
          signal.Notify(signal_chan,
                  syscall.SIGHUP,
                  syscall.SIGINT,
                  syscall.SIGTERM,
                  syscall.SIGQUIT)

          exit_chan := make(chan int)
          go func() {
                  for {
                          s := <-signal_chan
                          switch s {
                          case syscall.SIGHUP:
                                  fmt.Println("SIGHUP received")

                          case syscall.SIGINT:
                                  pprof.StopCPUProfile()
                                  fmt.Println("Server Stoped by SIGINT")
                                  exit_chan<-0

                          case syscall.SIGTERM:
                                  fmt.Println("SIGTERM received")
                                  exit_chan <- 0

                          case syscall.SIGQUIT:
                                  fmt.Println("SIGQUIT received")
                                  exit_chan <- 0

                          default:
                                  fmt.Println("Unknown signal received.")
                                  exit_chan <- 1
                          }
                  }
          }()

          code := <-exit_chan
  pprof.StopCPUProfile()
  os.Exit(code)
  }()

  //Uncomment to Monitor with GProf
  //runtime.SetBlockProfileRate(1)
  //go http.ListenAndServe("10.129.26.154:8080", http.DefaultServeMux)
  // go http.ListenAndServe("10.129.28.141:8080", http.DefaultServeMux)


  socket := os.Args[1] //Read form command line arugment
  ln, err := net.Listen("tcp",socket)
  if err != nil {
    panic(err)
    return
    } else {
      tm = *kvstore.NewTableManager()
      fmt.Println("Server started at ",os.Args[1])
      for{
        conn, err := ln.Accept()
        if err != nil {
          continue
        }
        go handleClient(conn)
      }
    }
  }
