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
#   include/ - header files (.h, .hpp)
#   build/   - object and dependency files
#
# Author: Rob Lovegrove
# ============================================

CXX := g++
CXXFLAGS := -std=c++20 -Iinclude -Isrc -Wall -Wextra -MMD -MP
OPENJPEG_CFLAGS := -I/opt/homebrew/include/openjpeg-2.5
OPENJPEG_LIBS := -L/opt/homebrew/lib -lopenjp2
PNG_CFLAGS := -I/opt/homebrew/include
PNG_LIBS := -L/opt/homebrew/lib -lpng

SRC_DIR := src
BUILD_DIR := build
TARGET := umdf_tool

# Find all .cpp files recursively in src/
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')

# Map source files to corresponding object files in build/
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Dependency files (.d) for each object
DEPS := $(OBJS:.o=.d)

# Tell make where to look for prerequisites (source files)
VPATH := $(SRC_DIR)

.PHONY: all debug release clean

# Default target is release
all: release

debug: CXXFLAGS += -g -O0 $(OPENJPEG_CFLAGS) $(PNG_CFLAGS)
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG $(OPENJPEG_CFLAGS) $(PNG_CFLAGS)
release: $(TARGET)

# Ensure build directory exists before compiling objects
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile source files to object files, creating directories as needed
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Link all object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(OPENJPEG_LIBS) $(PNG_LIBS)

# Include dependency files to enable automatic rebuilding
-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)