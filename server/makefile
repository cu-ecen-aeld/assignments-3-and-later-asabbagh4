CC = gcc
CFLAGS = -g

ifeq ($(CROSS_COMPILE),)
 COMPILE_COMMAND = $(CC)
 $(info Native compilation)
else
 COMPILE_COMMAND = $(CROSS_COMPILE)$(CC)
 $(info Cross compilation: $(COMPILE_COMMAND))
endif

all: clean aesdsocket.o aesdsocket

aesdsocket: aesdsocket.o
	$(COMPILE_COMMAND) aesdsocket.o -o aesdsocket

aesdsocket.o: aesdsocket.c
	$(COMPILE_COMMAND) -c aesdsocket.c -o aesdsocket.o

clean:
	rm -f aesdsocket aesdsocket.o