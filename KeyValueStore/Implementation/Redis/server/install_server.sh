#!/bin/bash
wget -nc "https://www.dropbox.com/s/47m53a4a77xp9pn/redis-3.2.4.tar.gz?dl=0" -O redis-3.2.4.tar.gz && tar -zxvf redis-3.2.4.tar.gz
cd redis-3.2.4
make -j12
make install
apt-get install ruby-full
gem install redis
