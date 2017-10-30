CC=g++
CFLAGS= -std=c++11 -Wall -pedantic -ggdb
LOADERS=
EXE=APIThreads

UTIL_CFILES=util/RodenThreadLib.cpp
UTIL_HFILES=util/RodenThreadLib.h util/RodenLockedQueue.h
UTIL_OFILES=util/RodenThreadLib.o

P1_CFILES=$(UTIL_CFILES) src/APIThreads.cpp
P1_HFILES=$(UTIL_HILES)
P1_OFILES=$(UTIL_OFILES) src/APIThreads.o


OFILES=$(P1_OFILES)
HFILES=$(P1_HFILES)
CFILES=$(P1_CFILES)

all:	$(EXE)

%.o:	%.cpp $(HFILES)
	$(CC) -c $(CFLAGS) $< -o $@ $(LOADERS)

APIThreads:	$(P1_OFILES) $(P1_HFILES)
	$(CC) $(CFLAGS) $(P1_OFILES) -o $@ $(LOADERS)

clean:
	rm -f *~ .*.swp util/.*.swp core $(OFILES) $(EXE)