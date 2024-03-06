CC = gcc
CFLAGS = -Wall
TARGET = writer
OBJ = writer.o

CROSS_COMPILER_PREFIX = aarch64-none-linux-gnu-
# Check if CROSS_COMPILE is set, otherwise use default compiler
ifdef CROSS_COMPILER_PREFIX
	CC = $(CROSS_COMPILER_PREFIX)gcc
endif

#all
all: $(TARGET)

# Default target
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean target
clean:
	rm -f $(TARGET) *.o

# Cross-compilation target
cross:
	make CROSS_COMPILER_PREFIX=aarch64-none-linux-gnu- CC=$(CC) $(TARGET) 

