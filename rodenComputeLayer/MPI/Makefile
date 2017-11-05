CC=g++
CFLAGS= -std=c++11 -Wall -pedantic -pthread -L/usr/local/Cellar/boost/1.65.1/lib -lboost_regex-mt -lboost_filesystem-mt -lboost_thread-mt -lboost_system -lboost_mpi-mt -lboost_mpi -lboost_serialization -lcurl -lmpi -I/opt/local/include/openmpi -ggdb
LOADERS=
EXE=ProcessorPool

UTIL_CFILES= ../FirebaseLib/firebase.cpp ../ThreadPool/ThreadPool.cpp
UTIL_HFILES=./util/RodenLockedQueue.h ../FirebaseLib/firebase.h ../ThreadPool/ThreadPool.h
UTIL_OFILES= ../FirebaseLib/firebase.o ../ThreadPool/ThreadPool.o

P1_CFILES=$(UTIL_CFILES) ./ProcessorPool.cpp
P1_HFILES=$(UTIL_HILES)
P1_OFILES=$(UTIL_OFILES) ./ProcessorPool.o


OFILES=$(P1_OFILES)
HFILES=$(P1_HFILES)
CFILES=$(P1_CFILES)

all:	$(EXE)

%.o:	%.cpp $(HFILES)
	$(CC) -c $(CFLAGS) $< -o $@ $(LOADERS)

ProcessorPool:	$(P1_OFILES) $(P1_HFILES)
	$(CC) $(CFLAGS) $(P1_OFILES) -o $@ $(LOADERS)

clean:
	rm -f *~ .*.swp util/.*.swp core $(OFILES) $(EXE)
