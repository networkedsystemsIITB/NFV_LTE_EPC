#!/bin/bash
sudo mkdir /home/NFV_LTE_EPC
sudo chmod u+rwx /home/NFV_LTE_EPC
tar -C /home/NFV_LTE_EPC -xzf go1.6.2.linux-amd64.tar.gz
mkdir  /home/NFV_LTE_EPC/go_workspace
mkdir /home/NFV_LTE_EPC/go_workspace/src
mkdir /home/NFV_LTE_EPC/go_workspace/pkg
mkdir /home/NFV_LTE_EPC/go_workspace/bin

export GOROOT=/home/NFV_LTE_EPC/go
export PATH=$PATH:$GOROOT/bin
export GOPATH=/home/NFV_LTE_EPC/go_workspace

cp -rp levelmemdb /home/NFV_LTE_EPC/go_workspace/src
cp -rp github.com /home/NFV_LTE_EPC/go_workspace/src  #for offline use
#go get -d github.com/syndtr/goleveldb/leveldb
#go get -d github.com/syndtr/goleveldb/leveldb/memdb
