CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -g
CPPFLAGS := -Iinclude
BUILD_DIR := build
TARGET := $(BUILD_DIR)/mini_scheduler

SRC := \
	src/compare.c \
	src/config.c \
	src/main.c \
	src/engine.c \
	src/runqueue.c \
	src/trace.c \
	src/metrics.c \
	src/policies/rr.c \
	src/policies/fair.c

OBJ := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

TEST_SRC := \
	tests/test_compare.c \
	tests/test_config.c \
	tests/test_engine.c \
	tests/test_rr.c \
	tests/test_fair.c \
	tests/test_metrics.c
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRC))
TEST_DEP := $(TEST_BINS:=.d)

.PHONY: all clean test dirs

all: $(TARGET)

dirs:
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/policies results

$(TARGET): dirs $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/test_%: tests/test_%.c $(filter-out $(BUILD_DIR)/main.o,$(OBJ))
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP $^ -o $@

test: $(TEST_BINS)
	@set -e; for test_bin in $(TEST_BINS); do ./$$test_bin; done

clean:
	rm -rf $(BUILD_DIR)

-include $(DEP) $(TEST_DEP)
