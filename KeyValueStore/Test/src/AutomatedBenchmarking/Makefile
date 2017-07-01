redis:
	g++ CommonNoReqPerSecTest_v2.cpp -std=c++11 -lkvstore_v2 -lkvs_redis_v2 -DDS=\"REDIS\" -pthread -lboost_serialization

leveldb:
	g++ CommonNoReqPerSecTest_v2.cpp -std=c++11 -lkvstore_v2 -lkvs_leveldb_v2 -DDS=\"LEVELDB\" -pthread -lboost_serialization

memc:
	g++ CommonNoReqPerSecTest_v2.cpp -std=c++11 -lkvstore_v2 -lmemcached -lkvs_memcached_v2 -DDS=\"MEMC\" -pthread -lboost_serialization -lmemcached

ramcloud:
	#g++ CommonNoReqPerSecTest_v2.cpp -std=c++11 -lkvstore_v2 -lkvs_ramcloud_v2 -DDS=\"RAMCLOUD\" -pthread -lboost_serialization -L/home/$(USER)/RAMCloud/install/bin -lramcloud
	g++ CommonNoReqPerSecTest_v2.cpp -std=c++11 -lkvstore_v2 -lkvs_ramcloud_v2 -DDS=\"RAMCLOUD\" -pthread -lboost_serialization -L/home/$(USER)/RAMCloud/install/bin -lramcloud -lprotobuf  -lzookeeper_mt -lboost_system -lpcrecpp
	# export LD_LIBRARY_PATH=/home/jash2/RAMCloud/install/bin
