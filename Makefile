CC=g++
CFLAGS=-I
CFLAGS+=-Wall
# CFLAGS+=-DDEBUG

FILES1=networkMon.cpp
FILES2=interfaceMon.cpp

networkMon: $(FILES1)
	$(CC) $(CFLAGS) $^ -o $@

interfaceMon: $(FILES2)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f *.o networkMon interfaceMon

all: networkMon interfaceMon

