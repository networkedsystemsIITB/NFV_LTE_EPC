#!/bin/bash

mkdir -p /usr/local/include/kvscommon
cp ../../../Interface/KVStoreHeader.h /usr/local/include/kvscommon/

mkdir -p deps
cd deps

git clone https://github.com/redis/hiredis.git
cd hiredis
make
cd ..

git clone https://github.com/shawn246/redis_client.git
cd redis_client

mkdir -p /usr/local/include/
cp -rp test/redis /usr/local/include/
cp -rp src/RedisClient.hpp /usr/local/include/redis

cd src
g++ -std=c++11 -c RedisClient.cpp
#ar -cq libredisclient.a RedisClient.o

cd ../../

cp -p ../src/* /usr/local/include/redis

rm -r tmp
mkdir tmp
cd tmp
cp ../redis_client/src/RedisClient.o .
ar x ../hiredis/libhiredis.a
#pwd
g++ -std=c++11 -c ../../src/KVStoreNonTemplateImplRedis.cpp -Wno-deprecated
ar cq libkvs_redis.a *.o
#ar crsT libkvs_redis.a /usr/lib/libredisclient.a /usr/local/lib/libhiredis.a
#ar crsT libkvs_redis.a redis_client/src/libredisclient.a hiredis/libhiredis.a
cp -p libkvs_redis.a /usr/lib


