CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0 json-glib-1.0`
LIBS = `pkg-config --libs gtk+-3.0 json-glib-1.0`
LDFLAGS = -lcurl


SRC_DIR = src
BIN_DIR = bin

OUTPUT = $(BIN_DIR)/stations

SRC = $(SRC_DIR)/main.c $(SRC_DIR)/stations.c $(SRC_DIR)/widgets.c $(SRC_DIR)/wan.c $(SRC_DIR)/api.c $(SRC_DIR)/utils.c

$(shell mkdir -p $(BIN_DIR))

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT) $(LIBS) $(LDFLAGS) 

.PHONY: clean