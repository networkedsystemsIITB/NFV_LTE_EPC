#g++ --std=c++0x -g -I../install/include -I. RMCLoadTestMultiThreaded.cpp  -o RMCLoadTestMultiThreaded.out -L../install/bin -lramcloud -Wl,-rpath=../install/bin -pthread


PATH_TO_RAMCLOUD = /home/$(USER)/RAMCloud

GCC = g++ -std=c++11 -g #Remove -g later
LIBS = -lboost_serialization -pthread -lramcloud -Wl,-rpath=$(PATH_TO_RAMCLOUD)/install/bin #-Wl,linker options = ld linker options
INCLUDES = -I$(PATH_TO_RAMCLOUD)/install/include

OPLEVEL = 0
WARN = 0
SHOWWARN = #-Wno-deprecated

IMPLPATH = src
BINPATH = bin

ifeq ($(WARN),1)
SHOWWARN =
endif

ifneq ($(OPLEVEL),0)
OPL = -O$(OPLEVEL)
endif

all:	$(BINPATH)/KVStoreNonTemplateImplRamCloud.o


$(BINPATH)/KVStoreNonTemplateImplRamCloud.o:	$(IMPLPATH)/KVStoreImplRamCloud.cpp $(IMPLPATH)/KVStoreNonTemplateImplRamCloud.cpp ../../../Interface/KVStoreHeader.h $(IMPLPATH)/KVStore.h
	$(GCC) $(SHOWWARN) $(OPL) -c $(INCLUDES) $(IMPLPATH)/KVStoreNonTemplateImplRamCloud.cpp -o $(BINPATH)/KVStoreNonTemplateImplRamCloud.o $(LIBS)

install:	$(BINPATH)/KVStoreNonTemplateImplRamCloud.o
	ar -cq $(BINPATH)/libkvstore.a $(BINPATH)/KVStoreNonTemplateImplRamCloud.o
	cp -p $(BINPATH)/libkvstore.a /usr/lib

uninstall:
	echo "Uninstall!!! Yet to be implemented."

clean:
	rm -f *~ bin/*
