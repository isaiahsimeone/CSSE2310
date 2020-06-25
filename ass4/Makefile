CC = gcc
CFLAGS = -Wall -pedantic --std=gnu99

.DEFAULT: all
.PHONY: all clean

all: mapper2310 control2310 roc2310

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c -o util.o

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c -o error.o

port.o: port.c port.h
	$(CC) $(CFLAGS) -c port.c -o port.o

mapper2310: mapper2310.c util.o port.o
	$(CC) $(CFLAGS) -pthread util.o port.o mapper2310.c -o mapper2310

control2310: control2310.c util.o port.o error.o
	$(CC) $(CFLAGS) -pthread util.o port.o error.o control2310.c -o control2310

roc2310: roc2310.c util.o port.o error.o
	$(CC) $(CFLAGS) -pthread util.o port.o error.o roc2310.c -o roc2310

clean:
	rm -f *.o
		