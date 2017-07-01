#!/bin/bash
sudo mkdir -p /home/NFV_LTE_EPC
sudo chmod u+rwx /home/NFV_LTE_EPC
mkdir -p deps
cd deps
wget -nc "https://storage.googleapis.com/golang/go1.6.2.linux-amd64.tar.gz"
cd ..
tar -C /home/NFV_LTE_EPC -xzf deps/go1.6.2.linux-amd64.tar.gz

mkdir -p /home/NFV_LTE_EPC/go_workspace/src/levelmemdb
mkdir -p /home/NFV_LTE_EPC/go_workspace/pkg/levelmemdb
mkdir -p /home/NFV_LTE_EPC/go_workspace/bin/levelmemdb

export GOROOT=/home/NFV_LTE_EPC/go
export PATH=$PATH:$GOROOT/bin
export GOPATH=/home/NFV_LTE_EPC/go_workspace

cp -rp src/* /home/NFV_LTE_EPC/go_workspace/src/levelmemdb
# cp -rp deps/github.com /home/NFV_LTE_EPC/go_workspace/src  #for offline use
go get -d github.com/syndtr/goleveldb/leveldb
go get -d github.com/syndtr/goleveldb/leveldb/memdb
echo "Done"
