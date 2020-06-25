CC = gcc
CFLAGS = -Wall -pedantic --std=gnu99

.DEFAULT: all
.PHONY: all clean

all: 2310dealer 2310A 2310B

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c -o util.o

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c -o game.o

player.o: player.c player.h
	$(CC) $(CFLAGS) -c player.c -o player.o

error.o: error.c error.h
	$(CC) $(CFLAGS) -c error.c -o error.o

2310dealer: 2310dealer.c util.o game.o player.o error.o
	$(CC) $(CFLAGS) util.o game.o player.o error.o 2310dealer.c -o 2310dealer

2310A: 2310A.c util.o game.o player.o error.o
	$(CC) $(CFLAGS) util.o game.o player.o error.o 2310A.c -o 2310A

2310B: 2310B.c util.o game.o player.o error.o
	$(CC) $(CFLAGS) util.o game.o player.o error.o 2310B.c -o 2310B

clean:
	rm -f *.o
		
