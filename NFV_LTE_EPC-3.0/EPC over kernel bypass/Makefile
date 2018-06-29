
TARGETS = mme_kby sgw_kby pgw_kby mme_mkb sgw_mkb pgw_mkb
CC=g++ -fpermissive -std=c++11 -g -O3 -Wno-write-strings -Wall
CC1= g++ -std=c++11 -g
DPDK=0
PS=0
NETMAP=1
CFLAGS=-DMAX_CPUS=2

EPC_LIB = -I include-epc

# DPDK LIBRARY and HEADER
DPDK_INC=/include
DPDK_LIB=/lib/

# mtcp library and header 
MTCP_FLD    =../../mtcp
MTCP_INC    =-I${MTCP_FLD}/include
MTCP_LIB    =-L${MTCP_FLD}/lib
MTCP_TARGET = ${MTCP_LIB}/libmtcp.a
MTCP_SCR = -I ${MTCP_FLD}/scr/include

UTIL_FLD = ../../util
UTIL_INC = -I${UTIL_FLD}/include
UTIL_OBJ = ${UTIL_FLD}/http_parsing.o ${UTIL_FLD}/tdate_parse.o ${UTIL_FLD}/netlib.o

$(info VAR="$(MTCP_SCR)")

PS_DIR = ../../io_engine/
PS_INC = ${PS_DIR}/include
INC = -I./include/ ${UTIL_INC} ${MTCP_INC} -I${UTIL_FLD}/include
LIBS = ${MTCP_LIB}
ifeq ($(PS),1)
INC += -I{PS_INC}
LIBS += -lmtcp -L${PS_DIR}/lib -lps -lpthread -lnuma -lrt
endif

ifeq ($(NETMAP),1)
LIBS += -lmtcp -lpthread -lnuma -lrt
endif

# CFLAGS for DPDK-related compilation
INC += ${MTCP_INC}
ifeq ($(DPDK),1)
DPDK_MACHINE_FLAGS = $(shell cat /include/cflags.txt)
INC += ${DPDK_MACHINE_FLAGS} -I${DPDK_INC} -include $(DPDK_INC)/rte_config.h
endif

ifeq ($(shell uname -m),x86_64)
LIBS += -m64
endif

ifeq ($(DPDK),1)
DPDK_LIB_FLAGS = $(shell cat /lib/ldflags.txt)
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}
else
#LIBS += -m64 -g -O3 -pthread -lrt -march=native -Wl,-export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -Wl,-lnuma -Wl,-lmtcp -Wl,-lpthread -Wl,-lrt -Wl,-ldl -Wl,${DPDK_LIB_FLAGS}
LIBS += -g -O3 -pthread -lrt -march=native -export-dynamic ${MTCP_FLD}/lib/libmtcp.a -L../../dpdk/lib -lnuma -lmtcp -lpthread -lrt -ldl ${DPDK_LIB_FLAGS}
endif

#LDFLAGS += -lboost_system

ifeq ($V,) # no echo
	export MSG=@echo
	export HIDE=@
else
	export MSG=@\#
	export HIDE=
endif

#EPC Files required
EPC_REQR = include-epc/packet.o include-epc/utils.o include-epc/s1ap.o include-epc/gtp.o include-epc/mysql.o include-epc/diameter.o include-epc/telecom.o  include-epc/security.o

all: mme sgw pgw 

#mme compile
mme_kby.o: kby_mme.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB} ${MTCP_SCR} ${LIBS} ${CFLAGS} ${INC} -o mme_kby.o

MME_C = mme_kby.o ${MTCP_FLD}/lib/libmtcp.a
MME_L = ${CC} -o mme_kby $(EPC_REQR) $(MME_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`


mme_:	$(MME_C)
	$(MME_L)

#mme compile
mme_mkb.o: mkb_mme.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB} ${MTCP_SCR} ${LIBS} ${CFLAGS} ${INC} -o mme_mkb.o

MME_MKB_C = mme_mkb.o ${MTCP_FLD}/lib/libmtcp.a
MME_MKB_L = ${CC} -o mme_mkb $(EPC_REQR) $(MME_MKB_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

mme_mkb:	$(MME_MKB_C)
	$(MME_MKB_L)

#sgw_mkb compile
sgw_mkb.o: mkb_sgw.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB} ${MTCP_SCR} ${LIBS} ${CFLAGS} ${INC} -o sgw_mkb.o

SGW_MKB_C = sgw_mkb.o ${MTCP_FLD}/lib/libmtcp.a
SGW_MKB_L = ${CC} -o sgw_mkb $(EPC_REQR) $(SGW_MKB_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

sgw_mkb:	$(SGW_MKB_C)
	$(SGW_MKB_L)

#sgw compile
sgw_kby.o: kby_sgw.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB}  ${LIBS} ${CFLAGS} ${INC} -o sgw_kby.o

SGW_C = sgw_kby.o ${MTCP_FLD}/lib/libmtcp.a
SGW_L = ${CC} -o sgw_kby $(EPC_REQR) $(SGW_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

sgw: $(SGW_C)
	$(SGW_L)

#pgw_mkb compile
pgw_mkb.o: mkb_pgw.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB} ${MTCP_SCR} ${LIBS} ${CFLAGS} ${INC} -o pgw_mkb.o

PGW_MKB_C = pgw_mkb.o ${MTCP_FLD}/lib/libmtcp.a
PGW_MKB_L = ${CC} -o pgw_mkb $(EPC_REQR) $(PGW_MKB_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

pgw_mkb:	$(PGW_MKB_C)
	$(PGW_MKB_L)


#pgw compile
pgw_kby.o: kby_pgw.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB}  ${LIBS} ${CFLAGS} ${INC} -o pgw_kby.o

PGW_C = pgw_kby.o ${MTCP_FLD}/lib/libmtcp.a
PGW_L = ${CC} -o pgw_kby $(EPC_REQR) $(PGW_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

pgw: $(PGW_C)
	$(PGW_L)


clean:
	rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile
