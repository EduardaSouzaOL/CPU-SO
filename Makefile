CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2

.PHONY: all
all: rate edf

.PHONY: rate
rate:
	$(CC) $(CFLAGS) -DRATE_MONOTONIC scheduler.c -o rate

.PHONY: edf
edf:
	$(CC) $(CFLAGS) -DEARLIEST_DEADLINE_FIRST scheduler.c -o edf

.PHONY: clean
clean:
	rm -f rate edf
