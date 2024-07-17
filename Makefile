# Compiler and flags
CC=gcc
CFLAGS=-fdiagnostics-color=always -g -Wall

# Source and build directories
SRC_DIR=./source
BUILD_DIR=./build

# Source files
SOURCES=$(wildcard $(SRC_DIR)/*.c)

# Object files
OBJECTS=$(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# Target executable
TARGET=$(BUILD_DIR)/compiler_count

# Default target
all: $(TARGET)

# Linking the target executable from the object files
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Compiling source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Cleaning up the compilation products
clean:
	rm -f $(BUILD_DIR)/*.o $(TARGET)
