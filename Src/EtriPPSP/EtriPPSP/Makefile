BIN = EtriPPSP
CC = g++
CFLAGS = -c -Wall -std=c++11
OBJECTS = \
	./Common/Common.o\
	./Common/Util.o\
	./SHA/sha1.o\
	./SHA/sha256.o\
	./PP/PPMessage.o\
	./PP/Ack.o\
	./PP/Cancel.o\
	./PP/Choke.o\
	./PP/Data.o\
	./PP/HandShake.o\
	./PP/Have.o\
	./PP/Integrity.o\
	./PP/PEXReq.o\
	./PP/PEXRes.o\
	./PP/Request.o\
	./PP/Unchoke.o\
	./Binmap.o\
	./HashTree.o\
	./PPClient.o\
	./PPServer.o\
	./main.o
OUTS = \
	Common.o \
	Util.o \
	sha1.o \
	sha256.o \
	PPMessage.o \
	Ack.o \
	Cancel.o \
	Choke.o \
	Data.o \
	HandShake.o \
	Have.o \
	Integrity.o \
	PEXReq.o \
	PEXRes.o \
	Request.o \
	Unchoke.o \
	Binmap.o \
	HashTree.o \
	PPClient.o \
	PPServer.o \
	main.o 

all : ${OBJECTS}
	${CC} -o ${BIN} ${OUTS} -pthread
	
mac : CFLAGS += -DMACOS
mac : ${OBJECTS}
	${CC} -o ${BIN} ${OUTS}

clean : 
	rm -rf *.o -rf ${BIN}

%.o : %.cpp
	${CC} ${CFLAGS} $<


