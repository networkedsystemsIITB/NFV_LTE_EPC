GCC = g++ -std=c++11
LIBS = -lboost_serialization
WARN = 0
SHOWWARN = -Wno-deprecated
OPLEVEL = 0
OPL =
TC = 50
TESTPATH = Test

ifeq ($(WARN),1)
SHOWWARN =
endif

ifneq ($(OPLEVEL),0)
OPL = -O$(OPLEVEL)
endif

lib:	kvstore_client.cpp KVStoreImplLevelDB.cpp KVStoreNonTemplateImplLevelDB.cpp KVStoreHeader.h KVStore.h
	$(GCC) $(SHOWWARN) $(OPL) -c KVStoreNonTemplateImplLevelDB.cpp -o KVStoreNonTemplateImplLevelDB.o $(LIBS)

install:	KVStoreNonTemplateImplLevelDB.o
	ar -cq libkvstore.a KVStoreNonTemplateImplLevelDB.o
	cp -p libkvstore.a /usr/lib


test:	TestKVStore.o TestKVStorePerformance.o TestKVStoreNoReqPerSec.o


TestKVStore.o: kvstore_client.cpp KVStoreImplLevelDB.cpp KVStoreHeader.h KVStore.h $(TESTPATH)/TestKVStore.cpp
	$(GCC) $(SHOWWARN) $(OPL) $(TESTPATH)/TestKVStore.cpp  -o $(TESTPATH)/TestKVStore.o $(LIBS)


TestKVStorePerformance.o: kvstore_client.cpp KVStoreImplLevelDB.cpp KVStoreHeader.h KVStore.h $(TESTPATH)/TestKVStorePerformance.cpp
	$(GCC) $(SHOWWARN) $(OPL) $(TESTPATH)/TestKVStorePerformance.cpp  -o $(TESTPATH)/TestKVStorePerformance.o $(LIBS) -Dthread_count=$(TC)


TestKVStoreNoReqPerSec.o: kvstore_client.cpp KVStoreImplLevelDB.cpp KVStoreHeader.h KVStore.h $(TESTPATH)/TestKVStoreNoReqPerSec.cpp
	$(GCC) $(SHOWWARN) $(OPL) $(TESTPATH)/TestKVStoreNoReqPerSec.cpp  -o $(TESTPATH)/TestKVStoreNoReqPerSec.o $(LIBS) -Dthread_count=$(TC)

clean:
	rm -f *.o *~ $(TESTPATH)/*.o
