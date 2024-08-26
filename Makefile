# Define the compiler
CC = gcc

# Define the directories
SRC_DIR = .
BUILD_DIR = ./build

# Define the sources
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/lexer.c $(SRC_DIR)/parser.c $(SRC_DIR)/interpreter.c $(SRC_DIR)/symbol.c 



# Define the object files
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

# Define the executable name
TARGET = main

# Define the flags

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Build the object files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)  # Create the directory if it doesn't exist
	$(CC) $(CFLAGS) -c -o $@ $<

# Install target
install: $(TARGET)
	sudo install -m 755 $(TARGET) /usr/local/bin/

# Clean up
clean:
	rm -rf $(TARGET) $(BUILD_DIR)

# Uninstall target
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall
