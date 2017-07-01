# Dependencies
# boost serialization dev is required for use.


INSTALL_PATH = /usr/local
LIB_NAME = libkvs_leveldb.a
G++ = g++ -std=c++11


all: install_common	KVImplLevelDB.o $(LIB_NAME) install

install: KVImplLevelDB.o $(LIB_NAME)
	cp -f bin/$(LIB_NAME) $(INSTALL_PATH)/lib


install_common:
	cd ../../../../Interface/ && make


KVImplLevelDB.o:	KVImplLevelDB.cpp
	$(G++) -c KVImplLevelDB.cpp -o KVImplLevelDB.o

$(LIB_NAME):	KVImplLevelDB.o
	rm -f -r bin
	mkdir -p bin
	mv -f *.o bin/
	cd bin && ar -cvq $(LIB_NAME) *

clean:
	rm -f -r bin
	rm -f *.o

uninstall:
	rm $(INSTALL_PATH)/lib/$(LIB_NAME)
