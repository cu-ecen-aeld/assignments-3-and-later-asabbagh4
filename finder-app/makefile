CC = gcc
CFLAGS = -Wall
TARGET = writer
OBJ = writer.o

	CROSS_COMPILE = 
# Check if CROSS_COMPILE is set, otherwise use default compiler
ifdef CROSS_COMPILE
	CC = $(CROSS_COMPILE)gcc
endif

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
	make CROSS_COMPILE=aarch64-none-linux-gnu-
