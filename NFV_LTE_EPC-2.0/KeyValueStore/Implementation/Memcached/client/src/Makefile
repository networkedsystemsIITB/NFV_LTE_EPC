INSTALL_PATH = /usr/local
LIB_NAME = libkvs_memcached.a
G++ = g++ -O3 -std=c++11

all: install_all	KVImplMemcached.o $(LIB_NAME)

install_all: install_deps install

install:	install_common KVImplMemcached.o $(LIB_NAME)
	cp -f bin/$(LIB_NAME) $(INSTALL_PATH)/lib

install_deps:
	-wget -nc "https://www.dropbox.com/s/79mnqsex41a9de9/libmemcached-1.0.18.tar.gz?dl=0" -O "libmemcached-1.0.18.tar.gz" &&	tar -xzf libmemcached-1.0.18.tar.gz && cd "libmemcached-1.0.18" && ./configure
	cd "libmemcached-1.0.18" && sudo make -j16
	cd "libmemcached-1.0.18" && sudo make install


install_common:
	cd ../../../../Interface/ && make


KVImplMemcached.o:	KVImplMemcached.cpp
	$(G++) -c KVImplMemcached.cpp -o KVImplMemcached.o

$(LIB_NAME):	KVImplMemcached.o
	rm -f -r bin
	mkdir -p bin
	mv -f *.o bin/
	#cp ../hiredis-vip/libhiredis_vip.a bin/
	#cd bin && ar -x libhiredis_vip.a && rm libhiredis_vip.a && ar -cvq $(LIB_NAME) *
	cd bin && ar -cvq $(LIB_NAME) *

clean:
	rm -f -r bin
	rm -f *.o

uninstall:
	rm $(INSTALL_PATH)/lib/$(LIB_NAME)
