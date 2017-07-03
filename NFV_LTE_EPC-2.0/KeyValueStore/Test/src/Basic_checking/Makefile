
G++ = g++ -std=c++11 -g

ifeq ($(kvs),leveldb)
	KVSTORE = -lkvs_leveldb
	DEFINES = -DCONF=string\(\"10.129.26.154:8090\"\)
	#DEFINES = -DCONF=string\(\"127.1.1.1:8090\"\)
	#DEFINES = -DCONF=string\(\"10.129.28.44:8090\"\)
else
        KVSTORE = -lkvs_redis
	DEFINES = -DCONF=string\(\"10.129.28.141:7003\"\)
endif


ifeq ($(kvs),memcached)
	KVSTORE = -lmemcached -lkvs_memcached -lmemcached 
	#DEFINES = -DCONF=string\(\"--SERVER=10.129.26.154:11211\"\)
	#DEFINES = -DCONF=string\(\"--SERVER=10.129.28.207:12000\"\)
	DEFINES = -DCONF=string\(\"--SERVER=10.129.28.207:12000\ --SERVER=10.129.28.207:12000\"\)
endif


ifeq ($(kvs),ramcloud)
	KVSTORE = -L/home/jash/KeyValueStore/Implementation/RAMCloud/RAMCloud/obj.master/  -lkvs_ramcloud -lramcloud -lprotobuf -lpcrecpp -lboost_system -lzookeeper_mt
	#DEFINES = -DCONF=string\(\"tcp:host=10.129.26.81,port=11100\"\)
	DEFINES = -DCONF=string\(\"tcp:host=10.129.28.186,port=11100\"\)
endif


LIBS = -lkvstore -lboost_serialization -pthread $(KVSTORE)

all:	test

test:
	$(G++) TestKVStoreImpl_Basic.cpp $(LIBS) $(DEFINES) -o Basic_KVStore.out 
	$(G++) TestKVRequestImpl_Basic.cpp $(LIBS) $(DEFINES) -o Basic_KVRequest.out
	$(G++) TestKVRequestImpl_SgetSput.cpp $(LIBS) $(DEFINES) -o Basic_KVRequestSgetSput.out
	$(G++) TestKVStoreImpl_Async_Basic.cpp $(LIBS) $(DEFINES) -o Async_KVStore.out
	$(G++) TestKVStore_AsyncVsBlocking_SingleUserThread.cpp $(LIBS) $(DEFINES) -o AsyncVsBlocking_KVStore.out
	$(G++) TestKVRequestImpl_Async_Basic.cpp $(LIBS) $(DEFINES) -o Async_KVRequest.out
	$(G++) TestKVRequest_AsyncVsBlocking_SingleUserThread.cpp $(LIBS) $(DEFINES) -o AsyncVsBlocking_KVRequest.out

run:
	./Basic_KVStore.out
	./Basic_KVRequest.out
	./Basic_KVRequestSgetSput.out
	./Async_KVStore.out
	./Async_KVRequest.out
	./AsyncVsBlocking_KVStore.out
	./AsyncVsBlocking_KVRequest.out
