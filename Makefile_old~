# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude -g 
LDFLAGS = 
LDLIBS = -lm -lgsl -lgslcblas  -lreadline

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
OBJ_DIR = build

# Executable
TARGET = $(BIN_DIR)/mm_rpn

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Default rule
all: $(TARGET)

# Link object files into binary
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile source to object
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean generated files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run Doxygen if Doxyfile exists
doc:
	doxygen Doxyfile

# Phony targets
.PHONY: all clean doc
