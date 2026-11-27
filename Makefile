
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude

TARGET = watchdog
WORKER = worker

SRCS = src/main.c \
       src/signal_manager.c \
       src/process_manager.c \
       src/logger.c

OBJS = $(SRCS:.c=.o)

# build both programs
all: $(TARGET) $(WORKER)

# build the watchdog
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# build the worker
$(WORKER): workers/worker.c
	$(CC) $(CFLAGS) -o $@ $<

# compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# remove compiled files
clean:
	rm -f $(OBJS) $(TARGET) $(WORKER)

.PHONY: all clean