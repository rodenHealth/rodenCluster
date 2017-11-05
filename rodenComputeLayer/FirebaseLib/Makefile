CC=g++
CFLAGS= -std=c++11 -Wall -pedantic -pthread -L/usr/local/Cellar/boost/1.65.1/lib -lboost_regex-mt -lboost_filesystem-mt -lboost_thread-mt -lboost_system -lcurl -ggdb
LOADERS=
EXE=FirebaseLib

UTIL_CFILES=
UTIL_HFILES=./util/RodenLockedQueue.h
UTIL_OFILES=

P1_CFILES=$(UTIL_CFILES) ./firebase.cpp
P1_HFILES=$(UTIL_HILES)
P1_OFILES=$(UTIL_OFILES) ./firebase.o


OFILES=$(P1_OFILES)
HFILES=$(P1_HFILES)
CFILES=$(P1_CFILES)

all:	$(EXE)

%.o:	%.cpp $(HFILES)
	$(CC) -c $(CFLAGS) $< -o $@ $(LOADERS)

FirebaseLib:	$(P1_OFILES) $(P1_HFILES)
	$(CC) $(CFLAGS) $(P1_OFILES) -o $@ $(LOADERS)

clean:
	rm -f *~ .*.swp util/.*.swp core $(OFILES) $(EXE)
