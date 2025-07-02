
# ============================================
# Makefile for UMDF (Unified Medical Data Format)
#
# Usage:
#   make           - builds in release mode
#   make release   - same as above
#   make debug     - builds with debug flags
#   make clean     - removes build artifacts
#
# File structure:
#   src/     - source files (.cpp)
#   include/ - header files (.h)
#   build/   - auto-generated object and dependency files
#
# Author: Rob Lovegrove
# ============================================


# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -Iinclude -Wall -Wextra -MMD -MP

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

TARGET = umdf_tool

# Default target
all: release

# Debug and Release configurations
debug: CXXFLAGS += -g -O0
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Create build directory if needed
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile .cpp in src/ to .o in build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Automatically include dependency files
-include $(DEPS)

# Clean all build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean debug release