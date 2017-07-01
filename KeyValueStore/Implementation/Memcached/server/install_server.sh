sudo apt-get install libevent-dev -y
wget -nc "https://www.dropbox.com/s/h1z7w1qm9m70i3i/latest?dl=0" -O memcached-1.4.35.tar.gz && tar -xzf memcached-1.4.35.tar.gz && cd "memcached-1.4.35" && ./configure && make -j16
