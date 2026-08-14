# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g

# Directories
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

# Core source files (excluding main.c)
CORE_SRCS = $(SRC_DIR)/list.c $(SRC_DIR)/port.c $(SRC_DIR)/task.c $(SRC_DIR)/heap_4.c
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(CORE_SRCS))

# Main App Target
MAIN_SRC = $(SRC_DIR)/main.c
MAIN_OBJ = $(BUILD_DIR)/main.o
TARGET = $(BUILD_DIR)/minirtos_app

# Unity Test Target
UNITY_SRCS = $(TEST_DIR)/unity/unity.c $(TEST_DIR)/test_runner.c
UNITY_OBJS = $(BUILD_DIR)/unity.o $(BUILD_DIR)/test_runner.o
TEST_TARGET = $(BUILD_DIR)/unity_test_runner

# Default target
all: $(TARGET) $(TEST_TARGET)

# Link Main Binary
$(TARGET): $(CORE_OBJS) $(MAIN_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CORE_OBJS) $(MAIN_OBJ) -o $@

# Link Unity Test Binary
$(TEST_TARGET): $(CORE_OBJS) $(UNITY_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CORE_OBJS) $(UNITY_OBJS) -o $@

# Compile Core SRC files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Unity Test files
$(BUILD_DIR)/unity.o: $(TEST_DIR)/unity/unity.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Itests/unity -c $< -o $@

$(BUILD_DIR)/test_runner.o: $(TEST_DIR)/test_runner.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Itests/unity -c $< -o $@

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Run Main App
run: $(TARGET)
	./$(TARGET)

# Run Unity Unit Tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run test clean
