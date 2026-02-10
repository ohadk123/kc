SRC_DIR=./src
BUILD_DIR=./build

CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Werror -g -I./include -lk

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

all:
	compiledb make compile

compile: build $(BUILD_DIR)/main

build:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
