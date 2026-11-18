CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -pthread

BIN_DIR = bin

.PHONY: all run1 run2 run3 clean

all: $(BIN_DIR)/interblocage $(BIN_DIR)/memoire $(BIN_DIR)/ordonnancement

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/interblocage: section1/interblocage.c section1/interblocage.h | $(BIN_DIR)
	$(CC) $(CFLAGS) section1/interblocage.c -o $@

$(BIN_DIR)/memoire: section2/memoire.c section2/memoire.h | $(BIN_DIR)
	$(CC) $(CFLAGS) section2/memoire.c -o $@

$(BIN_DIR)/ordonnancement: section3/ordonnancement.c section3/ordonnancement.h | $(BIN_DIR)
	$(CC) $(CFLAGS) section3/ordonnancement.c -o $@

run1: $(BIN_DIR)/interblocage
	./$(BIN_DIR)/interblocage

run2: $(BIN_DIR)/memoire
	./$(BIN_DIR)/memoire

run3: $(BIN_DIR)/ordonnancement
	./$(BIN_DIR)/ordonnancement

clean:
	rm -rf $(BIN_DIR)
