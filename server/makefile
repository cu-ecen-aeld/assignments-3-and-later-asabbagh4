CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread -lrt

all: clean aesdsocket.o aesdsocket

aesdsocket: aesdsocket.o
	$(CC) aesdsocket.o -o aesdsocket $(LDFLAGS)

aesdsocket.o: aesdsocket.c
	$(CC) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o $(LDFLAGS)

clean:
	rm -f aesdsocket aesdsocket.o