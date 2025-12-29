CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lpthread

# Default target
all: scheduler

# Build the scheduler
scheduler: main.c kernel.c process.c scheduler.c
	$(CC) $(CFLAGS) -o scheduler main.c kernel.c process.c scheduler.c $(LIBS)

# Clean build artifacts
clean:
	rm -f scheduler

.PHONY: all clean