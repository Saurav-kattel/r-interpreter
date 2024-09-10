# Define the compiler
CC = gcc

# Define the directories
SRC_DIR = .
BUILD_DIR = ./build
TEST_DIR = ./test

# Define the sources (excluding main.c for tests)
SOURCES = $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/interpreter.c $(SRC_DIR)/symbol.c

# Main program source
MAIN_SOURCE = $(SRC_DIR)/main.c

# Define the test sources
TEST_SOURCES = $(TEST_DIR)/test.c 
# Define the object files for the main program
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
MAIN_OBJECT = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(MAIN_SOURCE))

# Define the test object files
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/test/%.o,$(TEST_SOURCES))

# Define the executable name
TARGET = main

# Define the test executable name
TEST_TARGET = test_main

# Define the flags
CFLAGS =  -Wextra -g
LDFLAGS = 

# Default target
all: $(TARGET)

# Build the main target
$(TARGET): $(OBJECTS) $(MAIN_OBJECT)
	$(CC) -o $@ $^ $(LDFLAGS)

# Build the object files for main
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Build the object files for test
$(BUILD_DIR)/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Test target (build and run tests, exclude main.c)
test: $(TEST_OBJECTS) $(OBJECTS)
	$(CC) -o $(TEST_TARGET) $(TEST_OBJECTS) $(OBJECTS) $(LDFLAGS)
	./$(TEST_TARGET)

# Install target
install: $(TARGET)
	sudo install -m 755 $(TARGET) /usr/local/bin/

# Clean up
clean:
	rm -rf $(TARGET) $(BUILD_DIR) $(TEST_TARGET)

# Uninstall target
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall test
