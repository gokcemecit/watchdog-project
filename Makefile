
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude

TARGET = watchdog
WORKER = worker

SRCS = src/main.c \
       src/signal_manager.c \
       src/process_manager.c \
       src/logger.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET) $(WORKER)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(WORKER): workers/worker.c
	$(CC) $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(WORKER)

.PHONY: all clean
