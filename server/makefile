CC ?= gcc
CFLAGS ?= -g -pthread
LDFLAGS = ?= -lm

ifeq ($(CROSS_COMPILE),)
 COMPILE_COMMAND = $(CC)
 $(info Native compilation)
else
 COMPILE_COMMAND = $(CROSS_COMPILE)$(CC)
 $(info Cross compilation: $(COMPILE_COMMAND))
endif

all: clean aesdsocket.o aesdsocket

aesdsocket: aesdsocket.o
	$(COMPILE_COMMAND) $(CFLAGS) aesdsocket.o -o aesdsocket $(LDFLAGS)

aesdsocket.o: aesdsocket.c
	$(COMPILE_COMMAND) $(CFLAGS) -c aesdsocket.c -o aesdsocket.o $(LDFLAGS)

clean:
	rm -f aesdsocket aesdsocket.o