#!/bin/bash
export GOROOT=/home/NFV_LTE_EPC/go
export PATH=$PATH:$GOROOT/bin
export GOPATH=/home/NFV_LTE_EPC/go_workspace
cd /home/NFV_LTE_EPC/go_workspace/src/levelmemdb/
#go run lmemdb_performance.go

if [ $# -eq 0 ]
then
	echo "Starting server at 127.1.1.1:8090"
	echo "You can also pass socket address as argument to this script."
	go run server.go 127.1.1.1:8090
else
	echo "Starting server at $1"
	go run server.go $1
fi
