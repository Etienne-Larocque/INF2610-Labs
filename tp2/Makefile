CC = gcc
CFLAGS = -Wall -Wextra -O0 -Iutils
LDFLAGS = -pthread

BIN_DIR = bin
UTILS = utils/timer.c

LOG1 ?= utils/logs.txt
LOG2 ?= utils/logs_2.txt
N ?= 3
NTHREADS ?= 2

.PHONY: all run1 run2 clean

all: $(BIN_DIR)/section1 $(BIN_DIR)/section2

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/section1: section1/section1.c $(UTILS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(UTILS) -o $@ $(LDFLAGS)

$(BIN_DIR)/section2: section2/section2.c $(UTILS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(UTILS) -o $@ $(LDFLAGS)

run1: $(BIN_DIR)/section1
	./$(BIN_DIR)/section1 $(LOG1) $(LOG2) $(N)

run2: $(BIN_DIR)/section2
	./$(BIN_DIR)/section2 $(LOG1) $(LOG2) $(N) $(NTHREADS)

clean:
	rm -rf $(BIN_DIR)
