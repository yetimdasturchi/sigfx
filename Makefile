CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -Iinclude
LDFLAGS := -lm

SRC_DIR := src
OBJ_DIR := obj
BIN     := sigfx

SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/wav.c $(SRC_DIR)/dsp.c $(SRC_DIR)/presets.c

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

.PHONY: clean run

run: $(BIN)
	./$(BIN) input.wav

clean:
	rm -rf $(OBJ_DIR) $(BIN)