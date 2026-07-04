SRC_DIR=./src
BUILD_DIR=./build

CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Werror -O2
DEBUG_FLAGS= -g

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

RELEASE_NAME := kc-linux-$(shell uname -m)

all:
	compiledb make compile -j

compile: build $(BUILD_DIR)/kc

build:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/kc: $(OBJS)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

release: clean compile
	mv $(BUILD_DIR)/kc $(BUILD_DIR)/$(RELEASE_NAME)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all compile build release clean
