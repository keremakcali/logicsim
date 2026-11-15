CC       := gcc
CFLAGS   := -Wall -Wextra -std=c99 -Isrc -isystem ext -isystem ext/raylib/src -MMD -MP
LDFLAGS  := -Lext/raylib/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC_DIR    := src
BUILD_DIR  := build
OBJ_DIR    := $(BUILD_DIR)/obj
RAYLIB_DIR := ext/raylib/src
RAYLIB_LIB := $(RAYLIB_DIR)/libraylib.a
TARGET     := $(BUILD_DIR)/logicsim

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

.PHONY: all run debug clean clean-all

all: $(TARGET)

$(TARGET): $(RAYLIB_LIB) $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(RAYLIB_LIB):
	$(MAKE) -C $(RAYLIB_DIR) PLATFORM=PLATFORM_DESKTOP

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

debug: CFLAGS += -g -O0
debug: clean $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

clean-all: clean
	$(MAKE) -C $(RAYLIB_DIR) clean

-include $(DEPS)
