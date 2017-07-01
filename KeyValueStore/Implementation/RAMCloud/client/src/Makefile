#g++ -std=c++11 -g -c -I/home/$(USER)/RAMCloud/install/include KVImplRAMCloud.cpp -lboost_serialization -pthread -lramcloud -Wl,-rpath=/home/$(USER)/RAMCloud/install/bin

PATH_TO_RAMCLOUD = ../../RAMCloud

GCC = g++ -std=c++11 #-g #Remove -g later
#LIBS = -lboost_serialization -pthread -lramcloud -Wl,-rpath=$(PATH_TO_RAMCLOUD)/install/bin #-Wl,linker options = ld linker options
INCLUDES = -I$(PATH_TO_RAMCLOUD)/install/include

OPLEVEL = 0
WARN = 0
SHOWWARN = #-Wno-deprecated

LIB_NAME = libkvs_ramcloud.a
IMPLPATH = .
BINPATH = bin

ifeq ($(WARN),1)
SHOWWARN =
endif

ifneq ($(OPLEVEL),0)
OPL = -O$(OPLEVEL)
endif

#install_all: all install

all:	init $(BINPATH)/KVImplRAMCloud.o

init:
	#bash ../../install_RAMCloud.sh
	mkdir -p bin

$(BINPATH)/KVImplRAMCloud.o:	$(IMPLPATH)/KVImplRAMCloud.cpp
	$(GCC) $(SHOWWARN) $(OPL) -c $(INCLUDES) $(IMPLPATH)/KVImplRAMCloud.cpp -o $(BINPATH)/KVImplRAMCloud.o

install:	$(BINPATH)/KVImplRAMCloud.o
	rm -rf $(BINPATH)/$(LIB_NAME)
	ar -cq $(BINPATH)/$(LIB_NAME) $(BINPATH)/KVImplRAMCloud.o
	cp -p $(BINPATH)/$(LIB_NAME) /usr/local/lib

uninstall:
	echo "Uninstall!!! Yet to be implemented."

clean:
	rm -f *~ bin/*
