
TARGETS = mme_kby sgw_kby pgw_kby sink_kby ran_kby
CC=g++ -fpermissive -std=c++11 -g -O3 -Wno-write-strings -Wall
CC1= g++ -std=c++11 -g
DPDK=0
PS=0
NETMAP=1
CFLAGS=-DMAX_CPUS=1

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

all: mme sgw pgw sink ran

#mme compile
mme_kby.o: kby_mme.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB} ${MTCP_SCR} ${LIBS} ${CFLAGS} ${INC} -o mme_kby.o

MME_C = mme_kby.o ${MTCP_FLD}/lib/libmtcp.a
MME_L = ${CC} -o mme_kby $(EPC_REQR) $(MME_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`


mme:	$(MME_C)
	$(MME_L)

#sgw compile
sgw_kby.o: kby_sgw.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB}  ${LIBS} ${CFLAGS} ${INC} -o sgw_kby.o

SGW_C = sgw_kby.o ${MTCP_FLD}/lib/libmtcp.a
SGW_L = ${CC} -o sgw_kby $(EPC_REQR) $(SGW_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

sgw: $(SGW_C)
	$(SGW_L)

#pgw compile
pgw_kby.o: kby_pgw.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB}  ${LIBS} ${CFLAGS} ${INC} -o pgw_kby.o

PGW_C = pgw_kby.o ${MTCP_FLD}/lib/libmtcp.a
PGW_L = ${CC} -o pgw_kby $(EPC_REQR) $(PGW_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

pgw: $(PGW_C)
	$(PGW_L)


#ran
ran_kby.o: kby_ran.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB}  ${LIBS} ${CFLAGS} ${INC} -o ran_kby.o
#b: 
#	g++ -std=c++11 -o b.o lib.o libpacket.o utils.o ${MTCP_FLD}/lib/libmtcp.a -pthread -lboost_system
#B_P = b.o lib.o libpacket.o utils.o ${MTCP_FLD}/lib/libmtcp.a
#B_R = ${CC} -o b $(B_P) ${LIBS}  ${UTIL_OBJ} -lboost_system
RAN_C = ran_kby.o ${MTCP_FLD}/lib/libmtcp.a
RAN_L = ${CC} -o ran_kby $(EPC_REQR) $(RAN_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

ran: $(RAN_C)
	 $(RAN_L)


#sink compile
sink_kby.o: kby_sink.cpp
	$(MSG) "   CC $<"
	$(HIDE) ${CC} -c $< -pthread ${EPC_LIB} ${MTCP_SCR} ${LIBS} ${CFLAGS} ${INC}  -o sink_kby.o

SINK_C = sink_kby.o ${MTCP_FLD}/lib/libmtcp.a
SINK_L = ${CC} -o sink_kby $(EPC_REQR) $(SINK_C) ${LIBS}  ${UTIL_OBJ}  -lcrypto -pthread `mysql_config --cflags --libs`

sink:	$(SINK_C)
	$(SINK_L)


clean:
	rm -f *~ *.o ${TARGETS} log_*

distclean: clean
	rm -rf Makefile
