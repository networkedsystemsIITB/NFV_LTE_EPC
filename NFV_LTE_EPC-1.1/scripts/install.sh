#!/bin/bash

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install openvpn
sudo apt-get install libsctp-dev
sudo apt-get install openssl
sudo add-apt-repository "ppa:patrickdk/general-lucid"
sudo apt-get update
sudo apt-get install iperf3
sudo apt-get install iperf
sudo apt-get install htop
sudo apt-get install ipvsadm
