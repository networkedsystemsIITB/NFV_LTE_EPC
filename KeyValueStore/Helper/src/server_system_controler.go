package main

import (
  "os"
  "os/exec"
  "fmt"
  "net"
  "time"
)

func temp_includes(){
  time.Now();
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
  if n<sum {
    fmt.Println("Write error",err);
  }
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


func syncSystem(cmd string) ([]byte,error) {
    return exec.Command("sh","-c",cmd).Output()
}

func asyncSystem(cmd string){
  go func(cmd string){
    _, err := exec.Command("sh","-c",cmd).Output()
    if err!=nil{
      fmt.Println("Error executing :",cmd,"\n",err)
    }
  }(cmd)
}

func handleClient(conn net.Conn) {
  for {
    strarr,err := readStrings(conn)
    if err!=nil {
      fmt.Println("Error:",err)
      return
    }
    for _,cmd := range strarr {
          asyncSystem(cmd)
    }
  }
}


func main() {
  socket := os.Args[1] //Read form command line arugment
  ln, err := net.Listen("tcp",socket)

  if err != nil {
      panic(err)
      return
    } else {
      fmt.Println("System Controller Started @ ",os.Args[1])
      for{
        conn, err := ln.Accept()
        if err != nil {
          continue
        }
        go handleClient(conn)
      }
    }
  }
